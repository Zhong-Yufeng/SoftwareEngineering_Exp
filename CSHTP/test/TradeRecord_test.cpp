#include"test.h"
#include"Item.h"
#include"TradeRecord.h"
#include"InMemoryTradeRecordStore.h"

// ============================================================
// 成交记录与存储层的冒烟测试
//
// 覆盖：
//   TradeRecord::Create 的校验规则与快照装配
//   IsParticipant 的参与者判定
//   InMemoryTradeRecordStore 的编号分配、覆盖保存、按编号查
//   条件查询（买家/卖家/当事人/商品/关键词/价格区间/时间区间/组合）
//   四种排序
//   结果与库的隔离
//
// 【测试的写法约定】商品都走 Item::CreateDraft + 状态迁移造出来，
// 成交记录都走 TradeRecord::Create 造出来，不手摆字段——测的是真实
// 流程里能出现的数据。
// ============================================================

using Test::Check;

namespace
{
    // 造一件指定状态的商品。编号由测试显式给出，避免依赖存储层。
    Item MakeItem(const std::string& ItemID,
                  const std::string& Title,
                  long long PriceCents,
                  const std::string& SellerID,
                  const std::string& Contact,
                  const std::string& TradeLocation,
                  ItemStatus TargetStatus)
    {
        auto Draft = Item::CreateDraft(Title,
                                       "九成新，无笔记",
                                       Category::Textbook,
                                       PriceCents,
                                       Condition::LikeNew,
                                       TradeLocation,
                                       Contact,
                                       3,
                                       SellerID);
        Check(Draft.has_value(), "测试装置: 造商品应当成功");

        Item Goods = Draft.value();
        Check(Goods.AssignID(ItemID), "测试装置: 给商品分配编号应当成功");

        if (TargetStatus == ItemStatus::PendingReview || TargetStatus == ItemStatus::Published)
        {
            Check(Goods.SubmitForReview(), "测试装置: 提交审核应当成功");
        }
        if (TargetStatus == ItemStatus::Published)
        {
            Check(Goods.Approve(500), "测试装置: 审核通过应当成功");
        }
        return Goods;
    }

    // 造一条合法成交记录。Create 失败说明测试装置本身有问题，
    // 这里也记一条断言，免得后面拿空对象继续跑。
    TradeRecord MakeTrade(const Item& Goods,
                          const std::string& BuyerID,
                          long long TradedAtMs,
                          const std::string& BuyerContact = "13900000000")
    {
        auto R = TradeRecord::Create(Goods, BuyerID, BuyerContact, TradedAtMs);
        Check(R.has_value(), "测试装置: 造成交记录应当成功");
        return R.value();
    }

    Item MakePublished(const std::string& ItemID = "item-1",
                       const std::string& Title = "高等数学教材",
                       long long PriceCents = 1500,
                       const std::string& SellerID = "u-seller-1",
                       const std::string& Contact = "13800000001",
                       const std::string& TradeLocation = "图书馆门口")
    {
        return MakeItem(ItemID, Title, PriceCents, SellerID, Contact,
                        TradeLocation, ItemStatus::Published);
    }

    // 按顺序比对编号列表；顺序本身就是断言的一部分
    bool SameIDs(const std::vector<TradeRecord>& Records,
                 const std::vector<std::string>& Expected)
    {
        if (Records.size() != Expected.size())
        {
            return false;
        }
        for (std::size_t i = 0; i < Expected.size(); ++i)
        {
            if (Records[i].TradeID != Expected[i])
            {
                return false;
            }
        }
        return true;
    }
}

void RunTradeRecordTests()
{
    // ================= 1. Create 的状态校验 =================
    {
        Item Draft = MakeItem("item-draft", "草稿商品", 1000, "u-seller-1",
                              "13800000001", "图书馆", ItemStatus::Draft);
        Check(!TradeRecord::Create(Draft, "u-buyer-1", "13900000000", 1000),
              "Create: 草稿商品不能成交");

        Item Pending = MakeItem("item-pending", "待审核商品", 1000, "u-seller-1",
                                "13800000001", "图书馆", ItemStatus::PendingReview);
        Check(!TradeRecord::Create(Pending, "u-buyer-1", "13900000000", 1000),
              "Create: 待审核商品不能成交");

        // 没有编号 = 还没入库，不能拿来成交
        auto NoID = Item::CreateDraft("没入库", "描述", Category::Textbook, 1000,
                                      Condition::LikeNew, "图书馆", "13800000001", 3,
                                      "u-seller-1");
        Check(NoID.has_value(), "Create: 前置，无编号草稿应造得出来");
        if (NoID)
        {
            Check(!TradeRecord::Create(*NoID, "u-buyer-1", "13900000000", 1000),
                  "Create: 没有商品编号（未入库）不能成交");
        }
    }

    // ================= 2. Create 的入参校验 =================
    {
        Item Goods = MakePublished();

        Check(!TradeRecord::Create(Goods, "", "13900000000", 1000),
              "Create: 买家编号为空应被拒绝");
        Check(!TradeRecord::Create(Goods, "u-seller-1", "13900000000", 1000),
              "Create: 买家就是卖家（自己买自己）应被拒绝");
        Check(!TradeRecord::Create(Goods, "u-buyer-1", "", 1000),
              "Create: 买家联系方式为空应被拒绝");
        Check(!TradeRecord::Create(Goods, "u-buyer-1", "13900000000", 0),
              "Create: 成交时间为 0 应被拒绝");
        Check(!TradeRecord::Create(Goods, "u-buyer-1", "13900000000", -1),
              "Create: 成交时间为负应被拒绝");
        Check(TradeRecord::Create(Goods, "u-buyer-1", "13900000000", 1).has_value(),
              "Create: 成交时间为 1（边界值）应成功");
    }

    // ================= 3. 快照装配 =================
    {
        Item Goods = MakePublished("item-9", "C++ Primer", 8800,
                                   "u-seller-9", "13800000009", "三食堂门口");
        auto R = TradeRecord::Create(Goods, "u-buyer-9", "13900000009", 123456);
        Check(R.has_value(), "快照: 合法输入应成功");
        if (R)
        {
            Check(R->TradeID.empty(),                  "快照: 新记录的编号应留空，等存储层分配");
            Check(R->ItemID == "item-9",               "快照: 商品编号应来自商品");
            Check(R->BuyerID == "u-buyer-9",           "快照: 买家编号应来自入参");
            Check(R->SellerID == "u-seller-9",         "快照: 卖家编号应来自商品");
            Check(R->TradedAtMs == 123456,             "快照: 成交时间应来自入参");
            Check(R->ItemTitle == "C++ Primer",        "快照: 标题应来自商品");
            Check(R->PriceCents == 8800,               "快照: 价格应来自商品");
            Check(R->TradeLocation == "三食堂门口",     "快照: 交易地点应来自商品");
            Check(R->SellerContact == "13800000009",   "快照: 卖家联系方式应来自商品");
            Check(R->BuyerContact == "13900000009",    "快照: 买家联系方式应来自入参");
        }
    }

    // ================= 4. IsParticipant =================
    {
        Item Goods = MakePublished("item-1", "高等数学教材", 1500, "u-seller-1");
        TradeRecord R = MakeTrade(Goods, "u-buyer-1", 1000);

        Check(R.IsParticipant("u-buyer-1"),  "IsParticipant: 买家应判定为当事人");
        Check(R.IsParticipant("u-seller-1"), "IsParticipant: 卖家应判定为当事人");
        Check(!R.IsParticipant("u-other"),   "IsParticipant: 无关用户应判定为否");
        Check(!R.IsParticipant(""),          "IsParticipant: 空编号应判定为否");
        Check(!R.IsParticipant("u-buyer"),   "IsParticipant: 前缀相同的编号不应误判");
    }

    // ================= 5. 保存与编号分配 =================
    {
        InMemoryTradeRecordStore Store;

        TradeRecord A = MakeTrade(MakePublished(), "u-buyer-1", 1000);
        Check(A.TradeID.empty(), "Save 前: 新记录不应有编号");
        Check(Store.Save(A), "Save: 新记录应保存成功");
        Check(A.TradeID == "trade-1", "Save: 第一条应分配 trade-1");

        TradeRecord B = MakeTrade(MakePublished(), "u-buyer-2", 2000);
        Check(Store.Save(B), "Save: 第二条应保存成功");
        Check(B.TradeID == "trade-2", "Save: 第二条应分配 trade-2");

        Check(Store.Size() == 2, "Save: 库中应有 2 条记录");
        Check(Store.FindByID("").has_value() == false, "FindByID: 空编号应查不到");
    }

    // ================= 6. 编号不被复用 =================
    {
        InMemoryTradeRecordStore Store;

        // 手工占掉 trade-1，模拟「从文件读入已有数据」
        TradeRecord A = MakeTrade(MakePublished(), "u-buyer-1", 1000);
        A.TradeID = "trade-1";
        Check(Store.Save(A), "编号不复用: 手工编号的记录应保存成功");

        TradeRecord B = MakeTrade(MakePublished(), "u-buyer-2", 2000);
        Check(Store.Save(B), "编号不复用: 新记录应保存成功");
        Check(B.TradeID == "trade-2", "编号不复用: 已占用的 trade-1 应被跳过");
    }

    // ================= 7. 带编号保存与覆盖 =================
    {
        InMemoryTradeRecordStore Store;

        TradeRecord A = MakeTrade(MakePublished(), "u-buyer-1", 1000);
        Check(Store.Save(A), "覆盖: 第一次保存应成功");
        Check(Store.Size() == 1, "覆盖: 第一次保存后应只有 1 条");

        TradeRecord B = MakeTrade(MakePublished(), "u-buyer-2", 2000);
        B.TradeID = A.TradeID;
        Check(Store.Save(B), "覆盖: 同编号再存应成功");
        Check(Store.Size() == 1, "覆盖: 同编号两次保存仍应只有 1 条");

        auto Found = Store.FindByID(A.TradeID);
        Check(Found.has_value() && Found->BuyerID == "u-buyer-2",
              "覆盖: 查到的应是后写入的那条");
    }

    // ================= 8. 条件查询 =================
    {
        InMemoryTradeRecordStore Store;

        TradeRecord T1 = MakeTrade(MakePublished("item-1", "高等数学教材", 1500,
                                                 "u-seller-1", "13800000001", "图书馆门口"),
                                   "u-buyer-1", 1000);
        TradeRecord T2 = MakeTrade(MakePublished("item-2", "二手蓝牙耳机", 8000,
                                                 "u-seller-1", "13800000001", "一食堂"),
                                   "u-buyer-2", 2000);
        TradeRecord T3 = MakeTrade(MakePublished("item-3", "英语四级词汇", 1200,
                                                 "u-seller-2", "13800000002", "图书馆二楼"),
                                   "u-buyer-1", 3000);
        TradeRecord T4 = MakeTrade(MakePublished("item-4", "机械键盘", 15000,
                                                 "u-seller-2", "13800000002", "宿舍楼"),
                                   "u-buyer-3", 4000);

        // 手工给编号，让断言与分配规则解耦
        T1.TradeID = "t-1";
        T2.TradeID = "t-2";
        T3.TradeID = "t-3";
        T4.TradeID = "t-4";
        Store.Save(T1);
        Store.Save(T2);
        Store.Save(T3);
        Store.Save(T4);
        Check(Store.Size() == 4, "Query: 前置，库中应有 4 条记录");

        TradeQuery Cond;

        // 按买家
        Cond = TradeQuery{};
        Cond.BuyerID = "u-buyer-1";
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-3"}), "Query: 按买家应查出 2 条");
        Cond.BuyerID = "u-nobody";
        Check(Store.Query(Cond).empty(), "Query: 没有成交的买家应返回空");
        Cond.BuyerID = "";
        Check(Store.Query(Cond).empty(), "Query: 空买家应返回空，而不是全部");

        // 按卖家
        Cond = TradeQuery{};
        Cond.SellerID = "u-seller-2";
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-3", "t-4"}), "Query: 按卖家应查出 2 条");

        // 按当事人（买家或卖家）
        Cond = TradeQuery{};
        Cond.ParticipantID = "u-seller-1";
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-2"}), "Query: 按当事人应查出买卖双方的记录");
        Cond = TradeQuery{};
        Cond.ParticipantID = "u-buyer-3";
        Check(SameIDs(Store.Query(Cond), {"t-4"}), "Query: 当事人也可以是买家");
        Cond = TradeQuery{};
        Cond.ParticipantID = "";
        Check(Store.Query(Cond).empty(), "Query: 空当事人应返回空，而不是全部");

        // 按商品与成交编号
        Cond = TradeQuery{};
        Cond.ItemID = "item-2";
        Check(SameIDs(Store.Query(Cond), {"t-2"}), "Query: 按商品编号应查出 1 条");
        Cond = TradeQuery{};
        Cond.TradeID = "t-3";
        Check(SameIDs(Store.Query(Cond), {"t-3"}), "Query: 按成交编号应查出 1 条");

        // 模糊匹配
        Cond = TradeQuery{};
        Cond.Keyword = "教材";
        Check(SameIDs(Store.Query(Cond), {"t-1"}), "Query: 关键词应匹配标题子串");
        Cond = TradeQuery{};
        Cond.Keyword = "耳机";
        Check(SameIDs(Store.Query(Cond), {"t-2"}), "Query: 关键词应能查出商品名里的词");
        Cond = TradeQuery{};
        Cond.TradeLocation = "图书馆";
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-3"}), "Query: 交易地点应匹配子串");

        // 价格区间（两端包含）
        Cond = TradeQuery{};
        Cond.MinPriceCents = 8000;
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-2", "t-4"}), "Query: 价格下限应含边界");
        Cond = TradeQuery{};
        Cond.MaxPriceCents = 1500;
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-3"}), "Query: 价格上限应含边界");
        Cond = TradeQuery{};
        Cond.MinPriceCents = 1200;
        Cond.MaxPriceCents = 8000;
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-2", "t-3"}), "Query: 价格区间应两端都含");

        // 时间区间（两端包含）
        Cond = TradeQuery{};
        Cond.TradedAfterMs = 2000;
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-2", "t-3", "t-4"}), "Query: 成交时间下限应含边界");
        Cond = TradeQuery{};
        Cond.TradedBeforeMs = 2000;
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-2"}), "Query: 成交时间上限应含边界");

        // 多条件取「与」
        Cond = TradeQuery{};
        Cond.BuyerID = "u-buyer-1";
        Cond.MinPriceCents = 1300;
        Check(SameIDs(Store.Query(Cond), {"t-1"}), "Query: 多个条件应同时满足才命中");

        // 空条件 = 全部
        Cond = TradeQuery{};
        Check(Store.Query(Cond).size() == 4, "Query: 空条件应返回全部 4 条");
    }

    // ================= 9. 排序 =================
    {
        InMemoryTradeRecordStore Store;

        TradeRecord T1 = MakeTrade(MakePublished("item-1", "甲", 1500, "u-seller-1", "13800000001", "A"), "u-buyer-1", 1000);
        TradeRecord T2 = MakeTrade(MakePublished("item-2", "乙", 8000, "u-seller-1", "13800000001", "B"), "u-buyer-2", 2000);
        TradeRecord T3 = MakeTrade(MakePublished("item-3", "丙", 1200, "u-seller-2", "13800000002", "C"), "u-buyer-1", 3000);
        TradeRecord T4 = MakeTrade(MakePublished("item-4", "丁", 15000, "u-seller-2", "13800000002", "D"), "u-buyer-3", 4000);

        T1.TradeID = "t-1";
        T2.TradeID = "t-2";
        T3.TradeID = "t-3";
        T4.TradeID = "t-4";
        Store.Save(T1);
        Store.Save(T2);
        Store.Save(T3);
        Store.Save(T4);

        // 默认排序：成交时间从新到旧
        Check(SameIDs(Store.Query(TradeQuery{}), {"t-4", "t-3", "t-2", "t-1"}),
              "排序: 默认应按成交时间从新到旧");

        TradeQuery Cond;
        Cond.Sort = TradeSort::TradedOldest;
        Check(SameIDs(Store.Query(Cond), {"t-1", "t-2", "t-3", "t-4"}),
              "排序: 指定从旧到新应生效");

        Cond = TradeQuery{};
        Cond.Sort = TradeSort::PriceAsc;
        Check(SameIDs(Store.Query(Cond), {"t-3", "t-1", "t-2", "t-4"}),
              "排序: 价格从低到高");

        Cond = TradeQuery{};
        Cond.Sort = TradeSort::PriceDesc;
        Check(SameIDs(Store.Query(Cond), {"t-4", "t-2", "t-1", "t-3"}),
              "排序: 价格从高到低");

        Cond = TradeQuery{};
        Cond.Sort = TradeSort::None;
        Check(Store.Query(Cond).size() == 4, "排序: 不排序也应返回全部 4 条");

        // 排序只作用于结果，不改动库
        Check(SameIDs(Store.All(), {"t-4", "t-3", "t-2", "t-1"}),
              "All: 全部记录应按成交时间从新到旧");
    }

    // ================= 10. 结果与库的隔离 =================
    {
        InMemoryTradeRecordStore Store;

        TradeRecord A = MakeTrade(MakePublished(), "u-buyer-1", 1000);
        Store.Save(A);

        // 存进去之后再改原对象，库里的副本不应跟着变
        A.BuyerID = "u-buyer-999";
        auto Found = Store.FindByID(A.TradeID);
        Check(Found.has_value() && Found->BuyerID == "u-buyer-1",
              "隔离: 改原对象不应影响库里的记录");

        // 改查询结果，库里的记录也不应变
        auto Mine = Store.Query(TradeQuery{});
        if (!Mine.empty())
        {
            Mine[0].ItemTitle = "改过的标题";
        }
        auto Again = Store.FindByID(A.TradeID);
        Check(Again.has_value() && Again->ItemTitle != "改过的标题",
              "隔离: 改查询结果不应影响库里的记录");
    }

    // ================= 11. 初始状态 =================
    {
        InMemoryTradeRecordStore Store;
        Check(Store.Size() == 0, "初始: 新建的库应为空");
        Check(Store.All().empty(), "初始: 空库 All 应返回空");
        Check(!Store.FindByID("trade-1").has_value(), "初始: 空库按编号应查不到");
    }
}
