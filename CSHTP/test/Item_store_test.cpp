#include"test.h"
#include"Item.h"
#include"ItemStore.h"
#include"InMemoryItemStore.h"
#include<string>
#include<vector>

// ============================================================
// ItemStore 的存取与查询
//
// 这一组测试只关心存储层自己的职责：编号分配、覆盖、条件筛选、排序、
// 删除。可见性属于 Item 自己的判定，在 Item_state_test.cpp 里验证。
// ============================================================

namespace
{
    // 造一件内容合法的商品。参数只留常改的那几个，其余给默认值，
    // 这样每条断言只变一个变量，失败时一眼能看出是哪个字段的问题。
    Item MakeItem(const std::string& Title = "二手教材",
                  const std::string& Description = "九成新，无笔记",
                  Category Cat = Category::Textbook,
                  long long PriceCents = 1000,
                  Condition Cond = Condition::LikeNew,
                  const std::string& SellerID = "seller-1",
                  const std::string& TradeLocation = "图书馆门口")
    {
        // 参数全部合法，所以一定有值；value() 取出的是拷贝。
        const auto Built = Item::CreateDraft(Title, Description, Cat, PriceCents,
                                             Cond, TradeLocation,
                                             "13800000000", 3, SellerID);
        return Built.value();
    }

    // 造一件已发布商品。只能走真实路径：提交审核 → 审核通过，
    // 不能手摆 Status（字段是私有的，这正是封装想要的效果）。
    Item MakePublished(const std::string& Title, long long PriceCents,
                       const std::string& SellerID, long long PublishedAtMs)
    {
        Item Goods = MakeItem(Title, "九成新，无笔记", Category::Textbook,
                              PriceCents, Condition::LikeNew, SellerID);
        Goods.SubmitForReview();
        Goods.Approve(PublishedAtMs);
        return Goods;
    }
}

void RunItemStoreTests()
{
    // ========================================================
    // 1. 保存与编号分配
    // ========================================================
    {
        InMemoryItemStore Store;

        Item A = MakeItem("甲");
        Item B = MakeItem("乙");

        Test::Check(Store.Save(A), "第一次保存应当成功");
        Test::Check(A.GetItemID() == "item-1", "第一件商品应当拿到 item-1");
        Test::Check(Store.Save(B), "第二次保存应当成功");
        Test::Check(B.GetItemID() == "item-2", "第二件商品应当拿到 item-2");
        Test::Check(Store.Size() == 2, "存了两件，Size 应当是 2");
    }

    // 同一件商品存两次，库里仍然只有一条（按编号覆盖，不是追加）
    {
        InMemoryItemStore Store;
        Item A = MakeItem("甲");

        Store.Save(A);
        Store.Save(A);

        Test::Check(Store.Size() == 1, "同一件商品存两次，库里仍只有一条");
        Test::Check(Store.FindByID(A.GetItemID()).has_value(), "存两次后仍能按编号查到");
    }

    // 改了状态再存一次，查到的是新状态（覆盖生效）
    {
        InMemoryItemStore Store;
        Item A = MakeItem("甲");

        Store.Save(A);
        A.SubmitForReview();

        Test::Check(Store.Save(A), "改状态后再次保存应当成功");

        const auto Found = Store.FindByID(A.GetItemID());
        Test::Check(Found.has_value() && Found->GetStatus() == ItemStatus::PendingReview,
                    "同一编号再次保存后，查到的是新状态");
        Test::Check(Store.Size() == 1, "改状态再存一次不该多出一条");
    }

    // ========================================================
    // 2. 按编号查（主键）
    // ========================================================
    {
        InMemoryItemStore Store;
        Item A = MakeItem("甲");
        Store.Save(A);

        const auto Found = Store.FindByID(A.GetItemID());
        Test::Check(Found.has_value(), "按编号能查到已保存的商品");
        Test::Check(Found->GetTitle() == "甲", "查到的就是存进去的那件");

        Test::Check(!Store.FindByID("item-999").has_value(), "编号不存在时返回空");
        Test::Check(!Store.FindByID("").has_value(), "空编号返回空，而不是命中别的商品");
    }

    // ========================================================
    // 3. 按条件查（单条件）
    //
    // 这一组故意放四种不同属性的商品，所有期望值都能手算出来：
    //   甲 教材 1000 分 LikeNew   seller-1 图书馆门口
    //   乙 教材 1500 分 BrandNew  seller-1 三食堂
    //   丙 数码 8000 分 Flawed    seller-2 宿舍楼下
    //   丁 服饰 3000 分 LikeNew   seller-2 图书馆门口
    // ========================================================
    InMemoryItemStore Shop;
    {
        Item A = MakeItem("高等数学", "九成新，无笔记", Category::Textbook, 1000,
                          Condition::LikeNew, "seller-1", "图书馆门口");
        Item B = MakeItem("线性代数", "全新未拆封", Category::Textbook, 1500,
                          Condition::BrandNew, "seller-1", "三食堂");
        Item C = MakeItem("机械键盘", "轴体完好", Category::Digital, 8000,
                          Condition::Flawed, "seller-2", "宿舍楼下");
        Item D = MakeItem("羽绒服", "只穿过两次", Category::Clothing, 3000,
                          Condition::LikeNew, "seller-2", "图书馆门口");

        Shop.Save(A);
        Shop.Save(B);
        Shop.Save(C);
        Shop.Save(D);

        Test::Check(Shop.Size() == 4, "四件商品都已入库");
    }

    // 空条件 = 不限
    {
        const auto All = Shop.Query(ItemQuery{});
        Test::Check(All.size() == 4, "没有任何条件时返回全部商品");
    }

    // 分类
    {
        ItemQuery Q;
        Q.ItemCategory = Category::Textbook;
        Test::Check(Shop.Query(Q).size() == 2, "按分类查教材应当有 2 件");

        ItemQuery Q2;
        Q2.ItemCategory = Category::Digital;
        const auto Found = Shop.Query(Q2);
        Test::Check(Found.size() == 1 && Found[0].GetTitle() == "机械键盘",
                    "按分类查数码应当只有机械键盘");
    }

    // 成色
    {
        ItemQuery Q;
        Q.ItemCondition = Condition::Flawed;
        const auto Found = Shop.Query(Q);
        Test::Check(Found.size() == 1 && Found[0].GetTitle() == "机械键盘",
                    "按成色查有瑕疵的应当只有机械键盘");
    }

    // 状态：全部都是草稿
    {
        ItemQuery Q;
        Q.Status = ItemStatus::Draft;
        Test::Check(Shop.Query(Q).size() == 4, "按草稿状态查应当是 4 件");

        ItemQuery Q2;
        Q2.Status = ItemStatus::Published;
        Test::Check(Shop.Query(Q2).empty(), "按已发布状态查应当是 0 件");
    }

    // 卖家
    {
        ItemQuery Q;
        Q.SellerID = "seller-2";
        Test::Check(Shop.Query(Q).size() == 2, "按卖家查 seller-2 应当有 2 件");

        ItemQuery Q2;
        Q2.SellerID = "seller-404";
        Test::Check(Shop.Query(Q2).empty(), "查不存在的卖家应当返回空");
    }

    // 关键词：标题命中
    {
        ItemQuery Q;
        Q.Keyword = "数学";
        const auto Found = Shop.Query(Q);
        Test::Check(Found.size() == 1 && Found[0].GetTitle() == "高等数学",
                    "关键词命中标题时只返回那一件");
    }

    // 关键词：描述命中
    {
        ItemQuery Q;
        Q.Keyword = "轴体";
        const auto Found = Shop.Query(Q);
        Test::Check(Found.size() == 1 && Found[0].GetTitle() == "机械键盘",
                    "关键词只在描述里出现时也要能查到");
    }

    // 关键词：不命中
    {
        ItemQuery Q;
        Q.Keyword = "不存在的词";
        Test::Check(Shop.Query(Q).empty(), "关键词谁都不命中时返回空");
    }

    // 交易地点：子串匹配（「图书馆」能同时命中「图书馆门口」）
    {
        ItemQuery Q;
        Q.TradeLocation = "图书馆";
        Test::Check(Shop.Query(Q).size() == 2, "按交易地点子串查应当有 2 件");
    }

    // ========================================================
    // 4. 价格区间（两端都含）
    // ========================================================
    {
        ItemQuery Q;
        Q.MinPriceCents = 1500;
        // 1500、3000、8000 三件
        Test::Check(Shop.Query(Q).size() == 3, "价格下限 1500 分应当剩 3 件");

        ItemQuery Q2;
        Q2.MaxPriceCents = 1500;
        // 1000、1500 两件
        Test::Check(Shop.Query(Q2).size() == 2, "价格上限 1500 分应当剩 2 件");

        ItemQuery Q3;
        Q3.MinPriceCents = 3000;
        Q3.MaxPriceCents = 8000;
        // 3000、8000 两件，边界值都算在内
        Test::Check(Shop.Query(Q3).size() == 2, "价格区间两端都包含边界值");

        ItemQuery Q4;
        Q4.MinPriceCents = 5000;
        Q4.MaxPriceCents = 6000;
        Test::Check(Shop.Query(Q4).empty(), "区间里没有商品时返回空");
    }

    // ========================================================
    // 5. 多条件组合：同时满足才算命中
    // ========================================================
    {
        ItemQuery Q;
        Q.SellerID = "seller-2";
        Q.ItemCategory = Category::Digital;
        Q.MinPriceCents = 5000;
        Q.Status = ItemStatus::Draft;

        const auto Found = Shop.Query(Q);
        Test::Check(Found.size() == 1 && Found[0].GetTitle() == "机械键盘",
                    "四个条件同时给定时取交集");
    }

    // ========================================================
    // 6. 排序
    // ========================================================
    {
        ItemQuery Q;
        Q.Sort = ItemSort::PriceAsc;
        const auto Asc = Shop.Query(Q);
        Test::Check(Asc.size() == 4, "排序不改变结果数量");
        Test::Check(Asc[0].GetPriceCents() == 1000 && Asc[3].GetPriceCents() == 8000,
                    "价格升序：最便宜的在最前，最贵的在最后");

        ItemQuery Q2;
        Q2.Sort = ItemSort::PriceDesc;
        const auto Desc = Shop.Query(Q2);
        Test::Check(Desc[0].GetPriceCents() == 8000 && Desc[3].GetPriceCents() == 1000,
                    "价格降序：最贵的在最前");
    }

    // 按发布时间排序
    {
        InMemoryItemStore Pub;
        Pub.Save(MakePublished("早发布的", 1000, "seller-1", 100));
        Pub.Save(MakePublished("中间的", 1000, "seller-1", 300));
        Pub.Save(MakePublished("晚发布的", 1000, "seller-1", 200));

        ItemQuery Q;
        Q.Sort = ItemSort::PublishedNewest;
        const auto Found = Pub.Query(Q);
        Test::Check(Found.size() == 3, "三件已发布商品都在");
        Test::Check(Found[0].GetPublishedAtMs() == 300
                    && Found[2].GetPublishedAtMs() == 100,
                    "发布时间从新到旧");
    }

    // 发布时间下限：只看最近发布的
    {
        InMemoryItemStore Pub;
        Pub.Save(MakePublished("早发布的", 1000, "seller-1", 100));
        Pub.Save(MakePublished("中间的", 1000, "seller-1", 300));
        Pub.Save(MakePublished("晚发布的", 1000, "seller-1", 200));

        ItemQuery Q;
        Q.PublishedAfterMs = 200;
        // 200 和 300 两件，边界值 200 本身也算
        Test::Check(Pub.Query(Q).size() == 2, "发布时间下限包含边界值");
    }

    // ========================================================
    // 7. 删除
    // ========================================================
    {
        InMemoryItemStore Store;
        Item A = MakeItem("甲");
        Item B = MakeItem("乙");
        Store.Save(A);
        Store.Save(B);

        Test::Check(Store.Remove(A.GetItemID()), "删除存在的商品应当成功");
        Test::Check(Store.Size() == 1, "删除后 Size 减一");
        Test::Check(!Store.FindByID(A.GetItemID()).has_value(), "删除后按编号查不到");
        Test::Check(!Store.Remove(A.GetItemID()), "再删一次应当返回 false");
        Test::Check(Store.FindByID(B.GetItemID()).has_value(), "删除一件不影响另一件");
    }

    // ========================================================
    // 8. 查询结果与库是两份数据
    // ========================================================
    {
        InMemoryItemStore Store;
        Item A = MakeItem("甲");
        Store.Save(A);

        std::vector<Item> Got = Store.Query(ItemQuery{});
        Test::Check(Got.size() == 1, "查询返回一件");
        Test::Check(Got[0].GetItemID() == A.GetItemID(), "查询结果保留原编号");

        // 改结果里的副本，库里的商品不能被带着变
        Got[0].SubmitForReview();
        const auto InStore = Store.FindByID(A.GetItemID());
        Test::Check(InStore.has_value() && InStore->GetStatus() == ItemStatus::Draft,
                    "改查询结果里的副本，库里的商品不受影响");
    }

    // ========================================================
    // 9. 可见性不在这里测
    //
    // 可见性是 Item::IsVisibleTo 的职责，与存储层无关；它的断言全部
    // 放在 Item_state_test.cpp 里，避免同一条规则在两处各写一遍、
    // 改规则时漏改一边。
    // ========================================================
}
