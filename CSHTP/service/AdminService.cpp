#include"AdminService.h"
#include"ServiceRules.h"

AdminService::AdminService(UserStore& UserStoreRef,
                           SellerApplicationStore& ApplicationStore,
                           ItemStore& ItemStoreRef,
                           AuditRecordStore& AuditStore,
                           ReservationStore& ReservationStoreRef,
                           TradeRecordStore& TradeRecordStoreRef,
                           LogService& LogStore,
                           ServiceClock ClockSource)
    : Users(UserStoreRef),
      Applications(ApplicationStore),
      Items(ItemStoreRef),
      Audits(AuditStore),
      Reservations(ReservationStoreRef),
      Trades(TradeRecordStoreRef),
      Logs(LogStore),
      Clock(ClockSource)
{
}

std::optional<User> AdminService::IssueAdminAccount(const std::string& AdminID,
                                                    const std::string& Password,
                                                    const std::string& Nickname,
                                                    std::string* Error)
{
    ClearError(Error);

    // 工号同时是手机号与学号，账号查重按这两个口径判断；编号由存储层分配
    if (Users.FindByPhone(AdminID).has_value() || Users.FindByStdNo(AdminID).has_value())
    {
        SetError(Error, "账号已存在");
        return std::nullopt;
    }

    const auto Candidate = User::createAdmin(AdminID, Password, Nickname);
    if (!Candidate.has_value())
    {
        SetError(Error, "管理员信息不完整：密码至少 6 位，昵称必填");
        return std::nullopt;
    }

    User Created = *Candidate;
    if (!Users.Save(Created))
    {
        SetError(Error, "管理员账号保存失败");
        return std::nullopt;
    }
    return Created;
}

namespace
{
    // 取一条仍处于待审核状态的申请；不存在或已处理时报错
    std::optional<SellerApplication> LoadPendingApplication(
        SellerApplicationStore& Applications,
        const std::string& ApplicationID,
        std::string* Error)
    {
        const auto Application = Applications.FindByID(ApplicationID);
        if (!Application.has_value())
        {
            SetError(Error, "申请不存在");
            return std::nullopt;
        }
        if (Application->AuditStatus != ApplicationStatus::PendingReview)
        {
            SetError(Error, "仅待审核的申请可处理");
            return std::nullopt;
        }
        return Application;
    }
}

std::optional<SellerApplication> AdminService::FindApplication(
    const std::string&,
    const std::string& ApplicationID,
    std::string* Error) const
{
    ClearError(Error);

    const auto Application = Applications.FindByID(ApplicationID);
    if (!Application.has_value())
    {
        SetError(Error, "申请不存在");
        return std::nullopt;
    }
    return Application;
}

bool AdminService::ApproveApplication(const std::string& AdminID,
                                      const std::string& ApplicationID,
                                      std::string* Error)
{
    ClearError(Error);

    auto Pending = LoadPendingApplication(Applications, ApplicationID, Error);
    if (!Pending.has_value())
    {
        return false;
    }

    SellerApplication Application = Pending.value();
    if (!Application.Approve(AdminID, Clock()))
    {
        SetError(Error, "审核失败");
        return false;
    }

    auto Applicant = Users.FindByID(Application.ApplicantID);
    if (!Applicant.has_value() || !Applicant->BecomeSeller())
    {
        SetError(Error, "申请人状态异常");
        return false;
    }

    if (!Users.Save(Applicant.value()) || !Applications.Save(Application))
    {
        SetError(Error, "审核结果保存失败");
        return false;
    }

    Logs.Record(LogType::Audit, AdminID, ServiceObject::SellerApplication,
                Application.ApplicationID, "卖家认证通过");
    return true;
}

bool AdminService::RejectApplication(const std::string& AdminID,
                                     const std::string& ApplicationID,
                                     const std::string& Reason,
                                     std::string* Error)
{
    ClearError(Error);

    if (Reason.empty())
    {
        SetError(Error, "驳回理由不能为空");
        return false;
    }

    auto Pending = LoadPendingApplication(Applications, ApplicationID, Error);
    if (!Pending.has_value())
    {
        return false;
    }

    SellerApplication Application = Pending.value();
    if (!Application.Reject(Reason, AdminID, Clock()))
    {
        SetError(Error, "审核失败");
        return false;
    }
    if (!Applications.Save(Application))
    {
        SetError(Error, "审核结果保存失败");
        return false;
    }

    Logs.Record(LogType::Audit, AdminID, ServiceObject::SellerApplication,
                Application.ApplicationID, "卖家认证驳回：" + Reason);
    return true;
}

std::vector<SellerApplication> AdminService::ListApplications(
    std::optional<ApplicationStatus> Status) const
{
    if (Status.has_value())
    {
        return Applications.ListByStatus(Status.value());
    }
    return Applications.All();
}

bool AdminService::ApproveItem(const std::string& AdminID,
                               const std::string& ItemID,
                               std::string* Error)
{
    ClearError(Error);

    auto Goods = Items.FindByID(ItemID);
    if (!Goods.has_value())
    {
        SetError(Error, "商品不存在");
        return false;
    }
    if (!Goods->Approve(Clock()))
    {
        SetError(Error, "仅待审核商品可通过");
        return false;
    }
    if (!Items.Save(Goods.value()))
    {
        SetError(Error, "商品保存失败");
        return false;
    }

    auto Record = AuditRecord::Create(ItemID, AdminID, AuditResult::Approved,
                                      Clock(), "");
    if (Record.has_value())
    {
        Audits.Save(Record.value());
    }

    Logs.Record(LogType::Audit, AdminID, ServiceObject::Item,
                ItemID, "商品审核通过");
    return true;
}

bool AdminService::RejectItem(const std::string& AdminID,
                              const std::string& ItemID,
                              const std::string& Reason,
                              std::string* Error)
{
    ClearError(Error);

    if (Reason.empty())
    {
        SetError(Error, "驳回理由不能为空");
        return false;
    }

    auto Goods = Items.FindByID(ItemID);
    if (!Goods.has_value())
    {
        SetError(Error, "商品不存在");
        return false;
    }
    if (!Goods->Reject(Reason))
    {
        SetError(Error, "仅待审核商品可驳回");
        return false;
    }
    if (!Items.Save(Goods.value()))
    {
        SetError(Error, "商品保存失败");
        return false;
    }

    auto Record = AuditRecord::Create(ItemID, AdminID, AuditResult::Rejected,
                                      Clock(), Reason);
    if (Record.has_value())
    {
        Audits.Save(Record.value());
    }

    Logs.Record(LogType::Audit, AdminID, ServiceObject::Item,
                ItemID, "商品审核驳回：" + Reason);
    return true;
}

std::vector<AuditRecord> AdminService::ListAuditRecords(const std::string&,
                                                        const std::string& ItemID,
                                                        std::string* Error) const
{
    ClearError(Error);
    return Audits.ListByItem(ItemID);
}

std::optional<Item> AdminService::FindItem(const std::string& AdminID,
                                           const std::string& ItemID,
                                           std::string* Error) const
{
    return ServiceRules::FindVisibleItem(Items, Role::Admin, AdminID, ItemID, Error);
}

std::vector<Item> AdminService::QueryItems(const ItemQuery& Cond,
                                           const std::string& AdminID) const
{
    return ServiceRules::QueryVisibleItems(Items, Cond, Role::Admin, AdminID);
}

std::vector<Reservation> AdminService::ListItemReservations(const std::string&,
                                                            const std::string& ItemID,
                                                            std::string* Error) const
{
    ClearError(Error);
    return Reservations.ListByItem(ItemID);
}

std::vector<TradeRecord> AdminService::ListItemTrades(const std::string&,
                                                      const std::string& ItemID,
                                                      std::string* Error) const
{
    ClearError(Error);
    return Trades.ListByItem(ItemID);
}

std::vector<User> AdminService::ListUsers(const std::string&, std::string* Error) const
{
    ClearError(Error);
    return Users.All();
}

PlatformStats AdminService::GetPlatformStats(long long FromMs, long long ToMs) const
{
    PlatformStats Result;

    for (const Item& Goods : Items.All())
    {
        if (Goods.PublishedAtMs >= FromMs && Goods.PublishedAtMs <= ToMs)
        {
            ++Result.PublishedCount;
        }
    }

    // 预约记录会随确认、取消、下架被删除，只有日志能还原历史预约量。
    LogQuery Cond;
    Cond.Type = LogType::CreateReservation;
    Cond.FromMs = FromMs;
    Cond.ToMs = ToMs;
    Result.ReservationCount = Logs.Query(Cond).size();

    // 只有交易记录带完成时间，商品本身没有「成交时间」字段，所以这里以记录状态为准。
    for (const TradeRecord& Record : Trades.All())
    {
        if (Record.Status == TradeStatus::Completed &&
            Record.CompletedAtMs >= FromMs && Record.CompletedAtMs <= ToMs)
        {
            ++Result.CompletedTradeCount;
        }
    }
    return Result;
}

bool AdminService::QueryLogs(const std::string&,
                             const LogQuery& Cond,
                             std::vector<OperationLog>* Result,
                             std::string* Error) const
{
    ClearError(Error);

    if (Result == nullptr)
    {
        SetError(Error, "日志查询结果不能为空");
        return false;
    }

    *Result = Logs.Query(Cond);
    return true;
}

// ----- 以下函数暂未实现 ----------------------------------------
bool AdminService::BanUser(const std::string&,
                           const std::string&,
                           const std::string&,
                           std::string*)
{
    // TODO
    return false;
}

bool AdminService::UnbanUser(const std::string&,
                             const std::string&,
                             std::string*)
{
    // TODO
    return false;
}

bool AdminService::ViolationOffShelfItem(const std::string&,
                                         const std::string&,
                                         const std::string&,
                                         std::string*)
{
    // TODO
    return false;
}
