#include"test.h"
#include"User.h"
#include"Item.h"
#include"Reservation.h"
#include"SellerApplication.h"
#include"AuditRecord.h"
#include"OperationLog.h"
#include"InMemoryUserStore.h"
#include"InMemoryItemStore.h"
#include"InMemoryReservationStore.h"
#include"InMemorySellerApplicationStore.h"
#include"InMemoryAuditRecordStore.h"
#include"InMemoryOperationLogStore.h"

using Test::Check;

namespace
{
    ItemInfo MakeInfo(const std::string& Title,
                      long long PriceCents,
                      Category ItemCategory,
                      Condition ItemCondition,
                      const std::string& TradeLocation)
    {
        ItemInfo Info;
        Info.Title = Title;
        Info.Description = Title + " 的描述";
        Info.ItemCategory = ItemCategory;
        Info.PriceCents = PriceCents;
        Info.ItemCondition = ItemCondition;
        Info.TradeLocation = TradeLocation;
        Info.Contact = "QQ 10001";
        return Info;
    }

    Item MakeDraft(const std::string& Title,
                   const std::string& SellerID,
                   long long PriceCents,
                   Category ItemCategory = Category::Textbook,
                   Condition ItemCondition = Condition::LikeNew,
                   const std::string& TradeLocation = "一食堂")
    {
        const auto Created = Item::CreateDraft(
            MakeInfo(Title, PriceCents, ItemCategory, ItemCondition, TradeLocation), SellerID);
        return Created.value();
    }

    User MakeUser(const std::string& Phone, const std::string& Nickname)
    {
        return User::create(Phone, "", "123456", Nickname, "10001").value();
    }

    OperationLog MakeLog(LogType Type,
                         const std::string& OperatorID,
                         const std::string& ObjectType,
                         const std::string& ObjectID,
                         const std::string& Description,
                         long long AtMs)
    {
        return OperationLog::Create(Type, OperatorID, ObjectType, ObjectID, Description, AtMs).value();
    }
}

void RunStoreTests()
{
    {
        InMemoryUserStore Store;
        User A = MakeUser("13800001001", "甲");
        User B = MakeUser("13800001002", "乙");
        Check(Store.Save(A) && A.UserID == "user-1", "UserStore: 首次保存分配编号");
        Check(Store.Save(B) && B.UserID == "user-2", "UserStore: 第二个编号递增");
        Check(Store.Size() == 2, "UserStore: Size 正确");
        Check(Store.FindByID("user-1").has_value(), "UserStore: 按编号查找");
        Check(Store.FindByPhone("13800001001").has_value(), "UserStore: 按手机号查找");
        Check(!Store.FindByPhone("").has_value(), "UserStore: 空手机号查不到");
        Check(Store.All().size() == 2, "UserStore: All 返回全部");
        Check(!Store.FindByStdNo("241220170").has_value(), "UserStore: 不存在学号返回空");
    }

    {
        InMemoryUserStore Store;
        User Admin = User::createAdmin("admin-1", "admin123", "管理员").value();
        Check(Store.Save(Admin) && Admin.UserID == "user-1", "UserStore: 管理员编号也由系统分配");
        Check(Store.FindByPhone("admin-1").has_value(), "UserStore: 管理员工号可按手机号查到");
        User SamePhone = MakeUser("admin-1", "撞号");
        Check(!Store.Save(SamePhone), "UserStore: 同一账号不能对应两个用户");
    }

    {
        InMemoryItemStore Store;
        Item A = MakeDraft("高数教材", "seller-1", 2000, Category::Textbook,
                           Condition::LikeNew, "一食堂");
        Item B = MakeDraft("旧手机", "seller-1", 50000, Category::Digital,
                           Condition::Flawed, "二食堂");
        Item C = MakeDraft("外套", "seller-2", 3000, Category::Clothing,
                           Condition::BrandNew, "图书馆");
        Check(Store.Save(A) && A.ItemID == "item-1", "ItemStore: 首次保存分配编号");
        Check(Store.Save(B) && B.ItemID == "item-2", "ItemStore: 第二个编号递增");
        Check(Store.Save(C) && C.ItemID == "item-3", "ItemStore: 第三个编号递增");
        Check(Store.Size() == 3, "ItemStore: Size 正确");

        Item Published = B;
        Published.SubmitForReview();
        Published.Approve(1000);
        Check(Store.Save(Published), "ItemStore: 保存已发布商品");

        ItemQuery Cond;
        Cond.Keyword = "教材";
        Check(Store.Query(Cond).size() == 1, "ItemStore: 标题关键词查询");

        Cond = ItemQuery();
        Cond.Keyword = "旧手机 的描述";
        Check(Store.Query(Cond).size() == 1, "ItemStore: 描述关键词查询");

        Cond = ItemQuery();
        Cond.ItemCategory = Category::Digital;
        Check(Store.Query(Cond).size() == 1, "ItemStore: 分类筛选");

        Cond = ItemQuery();
        Cond.ItemCondition = Condition::BrandNew;
        Check(Store.Query(Cond).size() == 1, "ItemStore: 成色筛选");

        Cond = ItemQuery();
        Cond.MinPriceCents = 3000;
        Cond.MaxPriceCents = 50000;
        Check(Store.Query(Cond).size() == 2, "ItemStore: 价格区间筛选");

        Cond = ItemQuery();
        Cond.Status = ItemStatus::Published;
        Check(Store.Query(Cond).size() == 1, "ItemStore: 状态筛选");

        Cond = ItemQuery();
        Cond.SellerID = "seller-1";
        Check(Store.Query(Cond).size() == 2, "ItemStore: 卖家筛选");

        Cond = ItemQuery();
        Cond.TradeLocation = "食堂";
        Check(Store.Query(Cond).size() == 2, "ItemStore: 交易地点模糊匹配");

        Cond = ItemQuery();
        Cond.PublishedAfterMs = 500;
        Check(Store.Query(Cond).size() == 1, "ItemStore: 发布时间筛选");

        Cond = ItemQuery();
        Cond.Sort = ItemSort::PriceAsc;
        const auto Asc = Store.Query(Cond);
        Check(Asc.size() == 3 && Asc.front().PriceCents == 2000, "ItemStore: 价格升序");

        Cond.Sort = ItemSort::PriceDesc;
        const auto Desc = Store.Query(Cond);
        Check(Desc.size() == 3 && Desc.front().PriceCents == 50000, "ItemStore: 价格降序");

        Cond = ItemQuery();
        Cond.Status = ItemStatus::Published;
        Cond.Sort = ItemSort::PublishedNewest;
        const auto Newest = Store.Query(Cond);
        Check(Newest.size() == 1 && Newest.front().ItemID == "item-2", "ItemStore: 最新发布排序");

        Check(Store.Remove("item-3"), "ItemStore: 删除存在商品");
        Check(!Store.Remove("item-3"), "ItemStore: 重复删除失败");
        Check(Store.Size() == 2, "ItemStore: 删除后 Size 正确");
    }

    {
        InMemoryReservationStore Store;
        auto A = Reservation::Create("buyer-1", "item-1", 1000).value();
        auto B = Reservation::Create("buyer-2", "item-1", 1500).value();
        auto C = Reservation::Create("buyer-1", "item-2", 2000).value();
        Check(Store.Save(A) && A.ReservationID == "res-1", "ReservationStore: 分配编号");
        Check(Store.Save(B) && B.ReservationID == "res-2", "ReservationStore: 编号递增");
        Check(Store.Save(C) && C.ReservationID == "res-3", "ReservationStore: 编号递增");
        Check(Store.Size() == 3, "ReservationStore: Size 正确");
        Check(Store.FindByID("res-2").has_value(), "ReservationStore: 按编号查找");
        Check(Store.FindByBuyerAndItem("buyer-1", "item-2").has_value(), "ReservationStore: 按买家与商品查找");
        Check(!Store.FindByBuyerAndItem("buyer-2", "item-2").has_value(), "ReservationStore: 无匹配返回空");
        Check(Store.ListByBuyer("buyer-1").size() == 2, "ReservationStore: 买家列表");
        Check(Store.ListByItem("item-1").size() == 2, "ReservationStore: 商品列表");

        Check(Store.Remove("res-3"), "ReservationStore: 删除预约");
        Check(Store.Size() == 2, "ReservationStore: 删除后 Size 正确");
        Check(Store.RemoveByItem("item-1") == 2, "ReservationStore: 按商品批量删除");
        Check(Store.Size() == 0, "ReservationStore: 批量删除后为空");
    }

    {
        InMemorySellerApplicationStore Store;
        auto A = SellerApplication::Create("buyer-1", "张三", 1000).value();
        auto B = SellerApplication::Create("buyer-1", "张三", 1100).value();
        auto C = SellerApplication::Create("buyer-2", "李四", 1200).value();
        Check(Store.Save(A) && A.ApplicationID == "app-1", "SellerApplicationStore: 分配编号");
        Check(!Store.Save(B), "SellerApplicationStore: 同一申请人不能重复保存");
        Check(Store.Save(C) && C.ApplicationID == "app-2", "SellerApplicationStore: 第二个申请人可保存");
        Check(Store.Size() == 2, "SellerApplicationStore: Size 正确");
        Check(Store.HasApplication("buyer-1"), "SellerApplicationStore: 有申请");
        Check(Store.FindPendingByApplicant("buyer-1").has_value(), "SellerApplicationStore: 待审核申请可查");
        Check(Store.ListByStatus(ApplicationStatus::PendingReview).size() == 2, "SellerApplicationStore: 按状态列表");
        Check(Store.All().size() == 2, "SellerApplicationStore: All 返回全部");
    }

    {
        InMemoryAuditRecordStore Store;
        auto A = AuditRecord::Create("item-1", "admin", AuditResult::Approved, 1000, "").value();
        auto B = AuditRecord::Create("item-1", "admin", AuditResult::Rejected, 2000, "违规").value();
        auto C = AuditRecord::Create("item-2", "admin-2", AuditResult::Approved, 1500, "").value();
        Check(Store.Save(A) && A.RecordID == "aud-1", "AuditRecordStore: 分配编号");
        Check(Store.Save(B), "AuditRecordStore: 保存第二条");
        Check(Store.Save(C), "AuditRecordStore: 保存另一商品记录");
        Check(Store.Size() == 3, "AuditRecordStore: Size 正确");
        Check(Store.ListByItem("item-1").size() == 2, "AuditRecordStore: 按商品列表");
        Check(Store.ListByReviewer("admin").size() == 2, "AuditRecordStore: 按审核人列表");
        Check(Store.FindLatestByItem("item-1").value().RecordID == "aud-2", "AuditRecordStore: 最新审核记录");
        Check(Store.All().size() == 3, "AuditRecordStore: All 返回全部");
    }

    {
        InMemoryOperationLogStore Store;
        auto A = MakeLog(LogType::Register, "user-1", "User", "user-1", "注册", 1000);
        auto B = MakeLog(LogType::CreateReservation, "buyer-1", "Reservation", "res-1", "预约", 1500);
        auto C = MakeLog(LogType::CompleteTrade, "seller-1", "Item", "item-1", "成交", 2000);
        Check(Store.Save(A) && A.LogID == "log-1", "OperationLogStore: 分配编号");
        Check(Store.Save(B), "OperationLogStore: 保存第二条");
        Check(Store.Save(C), "OperationLogStore: 保存第三条");

        OperationLog Duplicate = A;
        Check(!Store.Save(Duplicate), "OperationLogStore: 已有编号拒绝保存");

        Check(Store.Size() == 3, "OperationLogStore: Size 正确");
        Check(Store.CountByType(LogType::Register) == 1, "OperationLogStore: 按类型计数");

        const std::vector<OperationLog> AllLogs = Store.Query(LogQuery{});
        Check(AllLogs.size() == 3, "OperationLogStore: 默认条件返回全部");
        Check(AllLogs.front().LogID == "log-3" && AllLogs.back().LogID == "log-1",
              "OperationLogStore: 结果按时间倒序");

        LogQuery ByType;
        ByType.Type = LogType::Register;
        Check(Store.Query(ByType).size() == 1, "OperationLogStore: 按类型查询");

        LogQuery ByOperator;
        ByOperator.OperatorID = "buyer-1";
        Check(Store.Query(ByOperator).size() == 1, "OperationLogStore: 按操作人查询");

        LogQuery ByObject;
        ByObject.ObjectID = "res-1";
        Check(Store.Query(ByObject).size() == 1, "OperationLogStore: 按对象编号查询");
        ByObject.ObjectID.clear();
        ByObject.ObjectType = "Item";
        Check(Store.Query(ByObject).size() == 1, "OperationLogStore: 按对象类型查询");

        LogQuery ByTime;
        ByTime.FromMs = 1200;
        ByTime.ToMs = 1800;
        Check(Store.Query(ByTime).size() == 1, "OperationLogStore: 时间窗查询");
        ByTime.FromMs = 1800;
        ByTime.ToMs = 1200;
        Check(Store.Query(ByTime).empty(), "OperationLogStore: 反向时间窗为空");
        ByTime.FromMs = 0;
        ByTime.ToMs = 1000;
        Check(Store.Query(ByTime).size() == 1, "OperationLogStore: 只给上界也能查");

        LogQuery ByKeyword;
        ByKeyword.Keyword = "成交";
        Check(Store.Query(ByKeyword).size() == 1, "OperationLogStore: 按说明搜索");
        ByKeyword.Keyword = "log-2";
        Check(Store.Query(ByKeyword).empty(), "OperationLogStore: 关键词只查操作人、对象与说明");

        LogQuery Combined;
        Combined.Type = LogType::Register;
        Combined.OperatorID = "buyer-1";
        Check(Store.Query(Combined).empty(), "OperationLogStore: 多条件同时生效");
    }
}
