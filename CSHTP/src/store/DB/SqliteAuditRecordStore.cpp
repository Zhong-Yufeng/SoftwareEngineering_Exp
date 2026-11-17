#include"SqliteAuditRecordStore.h"
#include"SqliteStatement.h"

namespace
{
    const char* const SelectColumns =
        "SELECT RecordID, ItemID, ReviewerID, Result, AuditedAtMs, Reason "
        "FROM audit_records ";

    const char* const OrderByTime = " ORDER BY AuditedAtMs ASC, RecordID ASC;";
}

SqliteAuditRecordStore::SqliteAuditRecordStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteAuditRecordStore::Save(AuditRecord& Record)
{
    if (Record.RecordID.empty())
    {
        if (!Database.NextID("aud", "aud-", Record.RecordID))
        {
            return false;
        }
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO audit_records(RecordID, ItemID, ReviewerID, Result, "
                           "AuditedAtMs, Reason) VALUES(?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT(RecordID) DO UPDATE SET "
                           "ItemID = excluded.ItemID, ReviewerID = excluded.ReviewerID, "
                           "Result = excluded.Result, AuditedAtMs = excluded.AuditedAtMs, "
                           "Reason = excluded.Reason;");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Record.RecordID);
    Insert.BindText(2, Record.ItemID);
    Insert.BindText(3, Record.ReviewerID);
    Insert.BindInt64(4, static_cast<long long>(Record.Result));
    Insert.BindInt64(5, Record.AuditedAtMs);
    if (Record.Reason.has_value())
    {
        Insert.BindText(6, Record.Reason.value());
    }
    else
    {
        Insert.BindNull(6);
    }

    if (Insert.Step() || Insert.HasError())
    {
        return false;
    }
    return true;
}

std::optional<AuditRecord> SqliteAuditRecordStore::FindByID(const std::string& RecordID) const
{
    if (RecordID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE RecordID = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, RecordID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::vector<AuditRecord> SqliteAuditRecordStore::ListByItem(const std::string& ItemID) const
{
    std::vector<AuditRecord> Result;
    if (ItemID.empty())
    {
        return Result;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ItemID = ?" + OrderByTime);
    if (!Query.IsReady())
    {
        return Result;
    }

    Query.BindText(1, ItemID);
    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::vector<AuditRecord> SqliteAuditRecordStore::ListByReviewer(const std::string& ReviewerID) const
{
    std::vector<AuditRecord> Result;
    if (ReviewerID.empty())
    {
        return Result;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ReviewerID = ?" + OrderByTime);
    if (!Query.IsReady())
    {
        return Result;
    }

    Query.BindText(1, ReviewerID);
    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::optional<AuditRecord> SqliteAuditRecordStore::FindLatestByItem(const std::string& ItemID) const
{
    if (ItemID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns)
                                      + "WHERE ItemID = ? ORDER BY AuditedAtMs DESC, RecordID DESC "
                                        "LIMIT 1;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, ItemID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::vector<AuditRecord> SqliteAuditRecordStore::All() const
{
    std::vector<AuditRecord> Result;

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

std::size_t SqliteAuditRecordStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM audit_records;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

AuditRecord SqliteAuditRecordStore::Load(SqliteStatement& Row)
{
    AuditRecord Record;
    Record.RecordID = Row.Text(0);
    Record.ItemID = Row.Text(1);
    Record.ReviewerID = Row.Text(2);
    Record.Result = static_cast<AuditResult>(Row.Int64(3));
    Record.AuditedAtMs = Row.Int64(4);
    if (!Row.IsNull(5))
    {
        Record.Reason = Row.Text(5);
    }
    return Record;
}
