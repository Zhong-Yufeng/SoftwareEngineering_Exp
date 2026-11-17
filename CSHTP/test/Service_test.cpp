#include"test.h"
#include"User.h"
#include"Item.h"
#include"Reservation.h"
#include"ServiceCommon.h"
#include"ServiceRules.h"
#include"LogService.h"
#include"LoginService.h"
#include"BuyerService.h"
#include"SellerService.h"
#include"AdminService.h"
#include"InMemoryUserStore.h"
#include"InMemorySellerApplicationStore.h"
#include"InMemoryItemStore.h"
#include"InMemoryReservationStore.h"
#include"InMemoryTradeRecordStore.h"
#include"InMemoryAuditRecordStore.h"
#include"InMemoryOperationLogStore.h"

using Test::Check;

namespace
{
    ItemInfo MakeInfo(const std::string& Title, long long PriceCents)
    {
        ItemInfo Info;
        Info.Title = Title;
        Info.Description = Title + " 的描述";
        Info.ItemCategory = Category::Textbook;
        Info.PriceCents = PriceCents;
        Info.ItemCondition = Condition::LikeNew;
        Info.TradeLocation = "一食堂";
        Info.Contact = "QQ 10001";
        return Info;
    }

    struct Fixture
    {
        InMemoryUserStore Users;
        InMemorySellerApplicationStore Applications;
        InMemoryItemStore Items;
        InMemoryReservationStore Reservations;
        InMemoryTradeRecordStore Trades;
        InMemoryAuditRecordStore Audits;
        InMemoryOperationLogStore Logs;
        long long Now = 1000;
        std::string Error;
        NullTransaction Tx;

        LogService LogsSvc;
        LoginService Logins;
        BuyerService Buyers;
        SellerService Sellers;
        AdminService Admins;

        Fixture()
            : LogsSvc(Logs, [this]() { return Now; }),
              Logins(Users),
              Buyers(Users, Applications, Items, Reservations, Trades, LogsSvc, Tx, [this]() { return Now; }),
              Sellers(Users, Items, Reservations, Trades, LogsSvc, Tx, [this]() { return Now; }),
              Admins(Users, Applications, Items, Audits, Reservations, Trades,
                     LogsSvc, Tx, [this]() { return Now; })
        {
        }

        std::string MakeAdmin()
        {
            // 管理员编号由存储层分配，重复调用时复用已有管理员，避免账号重复
            for (const auto& Existing : Users.All())
            {
                if (Existing.UserRole == Role::Admin)
                {
                    return Existing.UserID;
                }
            }

            auto Admin = User::createAdmin("admin", "admin123", "管理员");
            Check(Admin.has_value(), "fixture: 管理员创建");
            if (!Admin)
            {
                return "";
            }
            Users.Save(Admin.value());
            return Admin->UserID;
        }

        std::string Register(const std::string& Phone, const std::string& StdNo, const std::string& Nickname)
        {
            auto Created = Buyers.Register(Phone, StdNo, "123456", Nickname, "10001", &Error);
            Check(Created.has_value(), "fixture: 注册 " + Nickname);
            if (!Created)
            {
                return "";
            }
            return Created->UserID;
        }

        std::string MakeSeller(const std::string& Phone, const std::string& Nickname)
        {
            const std::string SellerID = Register(Phone, "", Nickname);
            if (SellerID.empty())
            {
                return "";
            }

            auto Application = Buyers.ApplyForSeller(SellerID, Nickname + "真名", &Error);
            Check(Application.has_value(), "fixture: 卖家申请");
            if (!Application)
            {
                return "";
            }

            const std::string AdminID = MakeAdmin();
            Check(Admins.ApproveApplication(AdminID, Application->ApplicationID, &Error),
                  "fixture: 卖家申请通过");
            return SellerID;
        }

        std::optional<Item> PublishItem(const std::string& SellerID,
                                        const std::string& Title,
                                        long long PriceCents)
        {
            auto Draft = Sellers.CreateDraft(SellerID, MakeInfo(Title, PriceCents), &Error);
            Check(Draft.has_value(), "fixture: 创建商品 " + Title);
            if (!Draft)
            {
                return std::nullopt;
            }
            Check(Sellers.SubmitForReview(SellerID, Draft->ItemID, &Error), "fixture: 提交审核");
            ++Now;

            const std::string AdminID = MakeAdmin();
            Check(Admins.ApproveItem(AdminID, Draft->ItemID, &Error), "fixture: 审核通过");
            return Items.FindByID(Draft->ItemID);
        }
    };
}

void RunServiceTests()
{
    {
        Fixture F;
        const std::string AdminID = F.MakeAdmin();
        const std::string BuyerID = F.Register("13800002001", "", "买家");
        const std::string SellerID = F.MakeSeller("13900002001", "卖家");
        const std::string StdBuyerID = F.Register("", "241220170", "学号买家");

        Check(!BuyerID.empty() && !SellerID.empty() && !StdBuyerID.empty(), "Account: 注册与认证链路可用");
        Check(F.Users.FindByID(SellerID)->UserRole == Role::Seller, "Account: 审核通过后成为卖家");
        Check(!F.Admins.ApproveApplication(AdminID, "app-missing", &F.Error),
              "Account: 不存在的申请不能审核");
        Check(F.Error.find("申请不存在") != std::string::npos, "Account: 申请不存在提示");
        Check(F.Users.FindByID(StdBuyerID)->UserRole == Role::Buyer, "Account: 学号注册仍是买家");

        Check(F.Logins.Login("13800002001", "123456", &F.Error).has_value(), "Account: 手机号登录");
        Check(F.Logins.Login("241220170", "123456", &F.Error).has_value(), "Account: 学号登录");
        Check(!F.Logins.Login("13800002001", "wrong", &F.Error).has_value(), "Account: 密码错误登录失败");
        Check(!F.Logins.Login("13899999999", "123456", &F.Error).has_value(), "Account: 不存在账号登录失败");
        Check(!F.Buyers.Register("13800002001", "", "123456", "重复", "10001", &F.Error).has_value(),
              "Account: 手机号不能重复注册");
        Check(!F.Buyers.Register("", "241220170", "123456", "重复", "10001", &F.Error).has_value(),
              "Account: 学号不能重复注册");
        Check(!F.Buyers.Register("", "", "123456", "无账号", "10001", &F.Error).has_value(),
              "Account: 手机号与学号不能都为空");

        Check(F.Buyers.ApplyForSeller(BuyerID, "买家", &F.Error).has_value(),
              "Account: 买家可申请卖家");
        Check(!F.Buyers.ApplyForSeller(BuyerID, "重复申请", &F.Error).has_value(),
              "Account: 已有申请不能再申请");

        const auto PendingApplications = F.Admins.ListApplications(ApplicationStatus::PendingReview);
        Check(PendingApplications.size() == 1, "Account: 待审核申请列表");
        const std::string ApplicationID = PendingApplications.front().ApplicationID;
        Check(F.Admins.FindApplication(AdminID, ApplicationID, &F.Error).has_value(),
              "Account: 管理员可查看申请内容");
        Check(!F.Admins.FindApplication(AdminID, "app-missing", &F.Error).has_value(),
              "Account: 不存在的申请查不到");
        Check(!F.Admins.RejectApplication(AdminID, ApplicationID, "", &F.Error),
              "Account: 驳回理由不能为空");
        Check(F.Admins.RejectApplication(AdminID, ApplicationID, "信息有误", &F.Error),
              "Account: 管理员可按编号驳回申请");
        Check(F.Users.FindByID(BuyerID)->UserRole == Role::Buyer, "Account: 驳回后仍是买家");
        Check(!F.Admins.ApproveApplication(AdminID, ApplicationID, &F.Error),
              "Account: 已处理的申请不能再次审核");
        Check(F.Error.find("待审核") != std::string::npos, "Account: 已处理申请提示");

        Check(F.Admins.ListUsers(AdminID, &F.Error).size() == 4, "Account: 用户列表数量");
        Check(F.Admins.ListApplications().size() == 2, "Account: 申请列表数量");
        Check(F.Logs.CountByType(LogType::Register) == 3, "Account: 注册日志数量");

        Check(F.Logins.Login("admin", "admin123", &F.Error).has_value(), "Account: 统一登录入口可登录管理员");
        Check(!F.Logins.Login("admin", "wrong", &F.Error).has_value(), "Account: 管理员密码错误登录失败");
        Check(F.Logins.Login("13900002001", "123456", &F.Error).has_value(), "Account: 统一登录入口可登录卖家");
        Check(F.Admins.IssueAdminAccount("admin-2", "admin456", "二号管理员", &F.Error).has_value(),
              "Account: 发放管理员账号");
        Check(!F.Admins.IssueAdminAccount("admin-2", "admin456", "重复", &F.Error).has_value(),
              "Account: 管理员编号不能重复");
        Check(F.Admins.ListUsers(AdminID, &F.Error).size() == 5, "Account: 发放后用户列表数量");

        LogQuery AllLogs;
        std::vector<OperationLog> Found;
        Check(F.Admins.QueryLogs(AdminID, AllLogs, &Found, &F.Error), "Log: 管理员可查询日志");
        Check(Found.size() == F.Logs.Query(AllLogs).size(), "Log: 管理员查询能取到全部日志");
        Check(!Found.empty() && Found.front().OperatedAtMs >= Found.back().OperatedAtMs,
              "Log: 结果按时间倒序");
        Check(!F.Admins.QueryLogs(AdminID, AllLogs, nullptr, &F.Error), "Log: 结果指针为空时拒绝");
        Check(F.Error.find("不能为空") != std::string::npos, "Log: 空指针提示结果不能为空");

        LogQuery ByType;
        ByType.Type = LogType::Register;
        Check(F.Admins.QueryLogs(AdminID, ByType, &Found, &F.Error) && Found.size() == 3,
              "Log: 按类型查询");
        Check(!Found.empty() && Found.front().Type == LogType::Register, "Log: 类型过滤结果正确");

        LogQuery ByOperator;
        ByOperator.OperatorID = BuyerID;
        Check(F.Admins.QueryLogs(AdminID, ByOperator, &Found, &F.Error) && Found.size() == 1,
              "Log: 按操作人查询");
        Check(!Found.empty() && Found.front().ObjectID == BuyerID, "Log: 操作人过滤结果正确");

        LogQuery ByObject;
        ByObject.ObjectID = BuyerID;
        Check(F.Admins.QueryLogs(AdminID, ByObject, &Found, &F.Error) && Found.size() == 1,
              "Log: 按操作对象查询");

        LogQuery ByWindow;
        ByWindow.FromMs = F.Now + 1;
        ByWindow.ToMs = F.Now + 1000;
        Check(F.Admins.QueryLogs(AdminID, ByWindow, &Found, &F.Error) && Found.empty(),
              "Log: 时间窗外查不到日志");
        ByWindow.FromMs = 0;
        ByWindow.ToMs = F.Now;
        Check(F.Admins.QueryLogs(AdminID, ByWindow, &Found, &F.Error) && !Found.empty(),
              "Log: 时间窗内查得到日志");

        LogQuery ByKeyword;
        ByKeyword.Keyword = "驳回";
        Check(F.Admins.QueryLogs(AdminID, ByKeyword, &Found, &F.Error) && Found.size() == 1,
              "Log: 按关键字查询");
        Check(!Found.empty() && Found.front().Type == LogType::Audit, "Log: 关键字过滤结果正确");

        LogQuery Empty;
        Empty.Keyword = "不存在的关键字";
        Check(F.Admins.QueryLogs(AdminID, Empty, &Found, &F.Error) && Found.empty(),
              "Log: 无匹配时返回空结果");
    }

    {
        Fixture F;
        const std::string AdminID = F.MakeAdmin();
        const std::string SellerID = F.MakeSeller("13900003001", "卖家");
        const std::string BuyerID = F.Register("13800003001", "", "买家");

        auto ItemA = F.PublishItem(SellerID, "高数教材", 2000);
        Check(ItemA.has_value(), "Item: 发布商品");
        if (!ItemA)
        {
            return;
        }

        Check(ItemA->Status == ItemStatus::Published, "Item: 审核后已发布");
        Check(F.Sellers.FindItem(SellerID, ItemA->ItemID, &F.Error).has_value(),
              "Item: 卖家可查看自己的商品");
        Check(F.Buyers.FindItem(BuyerID, ItemA->ItemID, &F.Error).has_value(),
              "Item: 买家可查看已发布商品");
        Check(F.Admins.FindItem(AdminID, ItemA->ItemID, &F.Error).has_value(),
              "Item: 管理员可查看已发布商品");

        ItemQuery Cond;
        Cond.Keyword = "高数";
        Check(F.Buyers.QueryItems(Cond, BuyerID).size() == 1, "Item: 已发布商品可被买家查询");
        Check(F.Buyers.QueryItems(Cond, "other").size() == 1, "Item: 已发布商品公开可见");

        auto ItemB = F.Sellers.CreateDraft(SellerID, MakeInfo("旧手机", 50000), &F.Error);
        Check(ItemB.has_value(), "Item: 创建待审核商品");
        if (!ItemB)
        {
            return;
        }
        Check(F.Sellers.SubmitForReview(SellerID, ItemB->ItemID, &F.Error), "Item: 提交待审核商品");
        Check(!F.Admins.RejectItem(AdminID, ItemB->ItemID, "", &F.Error), "Item: 驳回理由不能为空");
        Check(F.Admins.RejectItem(AdminID, ItemB->ItemID, "描述不清", &F.Error), "Item: 管理员可驳回");
        Check(F.Items.FindByID(ItemB->ItemID)->Status == ItemStatus::Rejected, "Item: 驳回状态正确");
        Check(F.Sellers.SubmitForReview(SellerID, ItemB->ItemID, &F.Error), "Item: 驳回后可重新提交");
        Check(F.Admins.ApproveItem(AdminID, ItemB->ItemID, &F.Error), "Item: 重新审核通过");

        Check(F.Sellers.ListMyItems(SellerID).size() == 2, "Item: 卖家商品列表");
        Check(F.Admins.ListAuditRecords(AdminID, ItemB->ItemID, &F.Error).size() == 2,
              "Item: 审核记录数量");
        Check(F.Logs.CountByType(LogType::SubmitReview) == 3, "Item: 提交审核日志");
        LogQuery AuditLogs;
        AuditLogs.Type = LogType::Audit;
        int ItemAuditCount = 0;
        for (const OperationLog& Log : F.Logs.Query(AuditLogs))
        {
            if (Log.ObjectType == ServiceObject::Item)
            {
                ++ItemAuditCount;
            }
        }
        Check(ItemAuditCount == 3, "Item: 商品审核日志");
    }

    {
        Fixture F;
        const std::string AdminID = F.MakeAdmin();
        const std::string SellerID = F.MakeSeller("13900004001", "卖家");
        const std::string Buyer1 = F.Register("13800004001", "", "买家甲");
        const std::string Buyer2 = F.Register("13800004002", "", "买家乙");

        auto Goods = F.PublishItem(SellerID, "交易商品", 3000);
        Check(Goods.has_value(), "Trade: 发布交易商品");
        if (!Goods)
        {
            return;
        }

        auto R1 = F.Buyers.Reserve(Buyer1, Goods->ItemID, &F.Error);
        ++F.Now;
        auto R2 = F.Buyers.Reserve(Buyer2, Goods->ItemID, &F.Error);
        Check(R1.has_value() && R2.has_value(), "Trade: 两个买家可同时预约");
        Check(F.Reservations.ListByItem(Goods->ItemID).size() == 2, "Trade: 产生两条预约");
        Check(F.Buyers.Reserve(Buyer1, Goods->ItemID, &F.Error)->ReservationID == R1->ReservationID,
              "Trade: 重复预约返回原记录");
        Check(F.Reservations.Size() == 2, "Trade: 重复预约不新增记录");
        Check(!F.Buyers.Reserve(SellerID, Goods->ItemID, &F.Error).has_value(), "Trade: 不能预约自己的商品");

        Check(F.Sellers.ConfirmReservation(SellerID, Goods->ItemID, R1->ReservationID, &F.Error),
              "Trade: 卖家确认预约");
        Check(F.Items.FindByID(Goods->ItemID)->Status == ItemStatus::Confirmed, "Trade: 商品变为已确认");
        Check(F.Reservations.FindByID(R1->ReservationID)->Status == ReservationStatus::Succeeded,
              "Trade: 被选预约已成功");
        Check(!F.Reservations.FindByID(R2->ReservationID).has_value(), "Trade: 另一条预约被删除");
        Check(F.Reservations.ListByItem(Goods->ItemID).size() == 1, "Trade: 只保留成功预约");

        Check(F.Trades.Size() == 1, "Trade: 卖家确认后生成一条交易记录");
        const std::vector<TradeRecord> Initiated = F.Buyers.ListMyTrades(Buyer1);
        Check(Initiated.size() == 1, "Trade: 买家可按 BuyerID 查到进行中的交易");
        Check(F.Sellers.ListMyTrades(SellerID).size() == 1, "Trade: 卖家可按 SellerID 查到进行中的交易");
        Check(F.Sellers.ListItemTrades(SellerID, Goods->ItemID, &F.Error).size() == 1,
              "Trade: 可按商品查到交易记录");
        Check(F.Buyers.ListMyTrades(Buyer2).empty(), "Trade: 落选买家没有交易记录");
        if (!Initiated.empty())
        {
            const TradeRecord& Record = Initiated.front();
            Check(!Record.TradeID.empty(), "Trade: 交易记录由存储层分配编号");
            Check(Record.ReservationID == R1->ReservationID, "Trade: 记录挂在被确认的预约上");
            Check(Record.Status == TradeStatus::InProgress, "Trade: 新记录为进行中");
            Check(Record.CreatedAtMs == F.Now, "Trade: 记录生成时间取注入的时钟");
            Check(Record.CompletedAtMs == 0 && Record.CancelledAtMs == 0 && Record.CancelReason.empty(),
                  "Trade: 进行中的记录没有完成/作废信息");
            Check(Record.ItemID == Goods->ItemID, "Trade: 交易记录指向商品");
            Check(Record.BuyerID == Buyer1 && Record.SellerID == SellerID, "Trade: 交易记录记住买卖双方");
            Check(Record.ItemTitle == "交易商品", "Trade: 标题为确认时快照");
            Check(Record.PriceCents == 3000, "Trade: 价格为确认时快照");
            Check(Record.BuyerContact == "10001", "Trade: 买家联系方式为学号");
            Check(Record.SellerContact == "QQ 10001", "Trade: 卖家联系方式来自商品");
            Check(F.Buyers.FindMyTrade(Buyer1, Record.TradeID, &F.Error).has_value(),
                  "Trade: 买家可按编号查自己的交易记录");
            Check(F.Buyers.FindTradeByReservation(Buyer1, R1->ReservationID, &F.Error).has_value(),
                  "Trade: 买家可按预约查交易记录");
            Check(!F.Buyers.FindMyTrade(Buyer2, Record.TradeID, &F.Error).has_value(),
                  "Trade: 非参与人不能查看交易记录");
            Check(Record.IsParticipant(Buyer1) && Record.IsParticipant(SellerID), "Trade: 买卖双方都算参与人");
            Check(!Record.IsParticipant(Buyer2) && !Record.IsParticipant(""), "Trade: 无关用户和空编号不算参与人");
        }

        Check(!F.Buyers.CancelReservation(Buyer2, Goods->ItemID, R2->ReservationID, &F.Error),
              "Trade: 旧页面取消已被删除的预约失败");
        Check(F.Error.find("商品已被约走") != std::string::npos, "Trade: 旧页面操作提示商品已被约走");
        Check(!F.Buyers.Reserve(Buyer2, Goods->ItemID, &F.Error).has_value(), "Trade: 已确认商品不能再预约");

        ++F.Now;
        Check(F.Sellers.CompleteTrade(SellerID, Goods->ItemID, &F.Error), "Trade: 卖家完成交易");
        Check(F.Items.FindByID(Goods->ItemID)->Status == ItemStatus::Traded, "Trade: 商品交易完成");
        Check(F.Reservations.FindByID(R1->ReservationID)->Status == ReservationStatus::Succeeded,
              "Trade: 成功预约保留");
        Check(!F.Sellers.CompleteTrade(SellerID, Goods->ItemID, &F.Error), "Trade: 不能重复完成交易");

        Check(F.Trades.Size() == 1, "Trade: 完成交易不新增记录");
        const std::vector<TradeRecord> Finished = F.Buyers.ListMyTrades(Buyer1);
        Check(Finished.size() == 1, "Trade: 交易完成后买家仍能查到记录");
        if (!Finished.empty())
        {
            const TradeRecord& Record = Finished.front();
            Check(Record.Status == TradeStatus::Completed, "Trade: 完成交易后记录变为已完成");
            Check(Record.CompletedAtMs == F.Now && Record.CompletedAtMs > Record.CreatedAtMs,
                  "Trade: 记录完成时间晚于生成时间");
            Check(Record.CancelledAtMs == 0 && Record.CancelReason.empty(), "Trade: 已完成的记录没有作废信息");
            Check(Record.ItemTitle == "交易商品" && Record.PriceCents == 3000, "Trade: 快照保持不变");
        }

        Check(ServiceRules::CancelTradesByItem(F.Trades, Goods->ItemID, "下架", F.Now) == 0,
              "Trade: 已完成的交易不能被作废");
        Check(F.Buyers.ListMyTrades(Buyer1).front().Status == TradeStatus::Completed,
              "Trade: 作废请求后记录仍为已完成");

        const SellerStats SellerResult = F.Sellers.GetSellerStats(SellerID);
        Check(SellerResult.OnSaleCount == 0 && SellerResult.CompletedCount == 1, "Account: 卖家汇总统计");
        const PlatformStats PlatformResult = F.Admins.GetPlatformStats(0, F.Now + 1);
        Check(PlatformResult.PublishedCount == 1, "Account: 平台发布量");
        Check(PlatformResult.ReservationCount == 2, "Account: 平台预约量按日志统计");
        Check(PlatformResult.CompletedTradeCount == 1, "Account: 平台成交量按交易记录统计");
        Check(F.Logs.CountByType(LogType::ConfirmReservation) == 1, "Trade: 确认预约日志");
        Check(F.Logs.CountByType(LogType::DeleteReservation) == 1, "Trade: 删除落选预约日志");
        Check(F.Logs.CountByType(LogType::CompleteTrade) == 1, "Trade: 完成交易日志");
    }
}
