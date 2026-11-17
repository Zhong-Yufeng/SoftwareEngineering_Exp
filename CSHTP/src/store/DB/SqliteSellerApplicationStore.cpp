#include"SqliteSellerApplicationStore.h"
#include"SqliteStatement.h"

namespace
{
    const char* const SelectColumns =
        "SELECT ApplicationID, ApplicantID, RealName, AuditStatus, SubmittedAtMs, AuditedAtMs, "
        "RejectReason, ReviewerID FROM seller_applications ";

    const char* const OrderByTime = " ORDER BY SubmittedAtMs ASC, ApplicationID ASC;";
}

SqliteSellerApplicationStore::SqliteSellerApplicationStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteSellerApplicationStore::Save(SellerApplication& Application)
{
    if (Application.ApplicationID.empty())
    {
        // 一个申请人最多一条记录
        if (HasApplication(Application.ApplicantID))
        {
            return false;
        }
        if (!Database.NextID("app", "app-", Application.ApplicationID))
        {
            return false;
        }
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO seller_applications(ApplicationID, ApplicantID, RealName, "
                           "AuditStatus, SubmittedAtMs, AuditedAtMs, RejectReason, ReviewerID) "
                           "VALUES(?, ?, ?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT(ApplicationID) DO UPDATE SET "
                           "ApplicantID = excluded.ApplicantID, RealName = excluded.RealName, "
                           "AuditStatus = excluded.AuditStatus, "
                           "SubmittedAtMs = excluded.SubmittedAtMs, "
                           "AuditedAtMs = excluded.AuditedAtMs, "
                           "RejectReason = excluded.RejectReason, "
                           "ReviewerID = excluded.ReviewerID;");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Application.ApplicationID);
    Insert.BindText(2, Application.ApplicantID);
    Insert.BindText(3, Application.RealName);
    Insert.BindInt64(4, static_cast<long long>(Application.AuditStatus));
    Insert.BindInt64(5, Application.SubmittedAtMs);
    if (Application.AuditedAtMs.has_value())
    {
        Insert.BindInt64(6, Application.AuditedAtMs.value());
    }
    else
    {
        Insert.BindNull(6);
    }
    if (Application.RejectReason.has_value())
    {
        Insert.BindText(7, Application.RejectReason.value());
    }
    else
    {
        Insert.BindNull(7);
    }
    if (Application.ReviewerID.has_value())
    {
        Insert.BindText(8, Application.ReviewerID.value());
    }
    else
    {
        Insert.BindNull(8);
    }

    if (Insert.Step() || Insert.HasError())
    {
        return false;
    }
    return true;
}

std::optional<SellerApplication> SqliteSellerApplicationStore::FindByID(const std::string& ApplicationID) const
{
    if (ApplicationID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ApplicationID = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, ApplicationID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::optional<SellerApplication> SqliteSellerApplicationStore::FindByApplicant(const std::string& ApplicantID) const
{
    if (ApplicantID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ApplicantID = ?" + OrderByTime);
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, ApplicantID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::optional<SellerApplication> SqliteSellerApplicationStore::FindPendingByApplicant(const std::string& ApplicantID) const
{
    const auto Application = FindByApplicant(ApplicantID);
    if (!Application.has_value() || Application->AuditStatus != ApplicationStatus::PendingReview)
    {
        return std::nullopt;
    }
    return Application;
}

bool SqliteSellerApplicationStore::HasApplication(const std::string& ApplicantID) const
{
    return FindByApplicant(ApplicantID).has_value();
}

std::vector<SellerApplication> SqliteSellerApplicationStore::ListByStatus(ApplicationStatus Status) const
{
    std::vector<SellerApplication> Result;

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE AuditStatus = ?" + OrderByTime);
    if (!Query.IsReady())
    {
        return Result;
    }

    Query.BindInt64(1, static_cast<long long>(Status));
    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::vector<SellerApplication> SqliteSellerApplicationStore::All() const
{
    std::vector<SellerApplication> Result;

    SqliteStatement Query(Database, std::string(SelectColumns) + std::string(OrderByTime).substr(1));
    if (!Query.IsReady())
    {
        return Result;
    }

    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::size_t SqliteSellerApplicationStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM seller_applications;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

SellerApplication SqliteSellerApplicationStore::Load(SqliteStatement& Row)
{
    SellerApplication Application;
    Application.ApplicationID = Row.Text(0);
    Application.ApplicantID = Row.Text(1);
    Application.RealName = Row.Text(2);
    Application.AuditStatus = static_cast<ApplicationStatus>(Row.Int64(3));
    Application.SubmittedAtMs = Row.Int64(4);
    if (!Row.IsNull(5))
    {
        Application.AuditedAtMs = Row.Int64(5);
    }
    if (!Row.IsNull(6))
    {
        Application.RejectReason = Row.Text(6);
    }
    if (!Row.IsNull(7))
    {
        Application.ReviewerID = Row.Text(7);
    }
    return Application;
}
