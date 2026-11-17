#include"SqliteTradeRecordStore.h"
#include"SqliteStatement.h"

namespace
{
    const char* const SelectColumns =
        "SELECT TradeID, ReservationID, ItemID, BuyerID, SellerID, Status, CreatedAtMs, "
        "CompletedAtMs, CancelledAtMs, CancelReason, ItemTitle, PriceCents, TradeLocation, "
        "SellerContact, BuyerContact FROM trade_records ";

    // 交易记录唯一的阅读方式是「最近的在最上面」，所以统一按生成时间倒序
    const char* const OrderByTimeDesc = " ORDER BY CreatedAtMs DESC, TradeID DESC;";
}

SqliteTradeRecordStore::SqliteTradeRecordStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteTradeRecordStore::Save(TradeRecord& Record)
{
    if (Record.TradeID.empty())
    {
        if (!Database.NextID("trade", "trade-", Record.TradeID))
        {
            return false;
        }
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO trade_records(TradeID, ReservationID, ItemID, BuyerID, "
                           "SellerID, Status, CreatedAtMs, CompletedAtMs, CancelledAtMs, "
                           "CancelReason, ItemTitle, PriceCents, TradeLocation, SellerContact, "
                           "BuyerContact) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT(TradeID) DO UPDATE SET "
                           "ReservationID = excluded.ReservationID, ItemID = excluded.ItemID, "
                           "BuyerID = excluded.BuyerID, SellerID = excluded.SellerID, "
                           "Status = excluded.Status, CreatedAtMs = excluded.CreatedAtMs, "
                           "CompletedAtMs = excluded.CompletedAtMs, "
                           "CancelledAtMs = excluded.CancelledAtMs, "
                           "CancelReason = excluded.CancelReason, "
                           "ItemTitle = excluded.ItemTitle, PriceCents = excluded.PriceCents, "
                           "TradeLocation = excluded.TradeLocation, "
                           "SellerContact = excluded.SellerContact, "
                           "BuyerContact = excluded.BuyerContact;");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Record.TradeID);
    Insert.BindText(2, Record.ReservationID);
    Insert.BindText(3, Record.ItemID);
    Insert.BindText(4, Record.BuyerID);
    Insert.BindText(5, Record.SellerID);
    Insert.BindInt64(6, static_cast<long long>(Record.Status));
    Insert.BindInt64(7, Record.CreatedAtMs);
    Insert.BindInt64(8, Record.CompletedAtMs);
    Insert.BindInt64(9, Record.CancelledAtMs);
    Insert.BindText(10, Record.CancelReason);
    Insert.BindText(11, Record.ItemTitle);
    Insert.BindInt64(12, Record.PriceCents);
    Insert.BindText(13, Record.TradeLocation);
    Insert.BindText(14, Record.SellerContact);
    Insert.BindText(15, Record.BuyerContact);

    if (Insert.Step() || Insert.HasError())
    {
        return false;
    }
    return true;
}

std::optional<TradeRecord> SqliteTradeRecordStore::FindByID(const std::string& TradeID) const
{
    if (TradeID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE TradeID = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, TradeID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::optional<TradeRecord> SqliteTradeRecordStore::FindByReservation(const std::string& ReservationID) const
{
    if (ReservationID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ReservationID = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, ReservationID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::vector<TradeRecord> SqliteTradeRecordStore::ListByBuyer(const std::string& BuyerID) const
{
    std::vector<TradeRecord> Result;
    if (BuyerID.empty())
    {
        return Result;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE BuyerID = ?" + OrderByTimeDesc);
    if (!Query.IsReady())
    {
        return Result;
    }

    Query.BindText(1, BuyerID);
    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::vector<TradeRecord> SqliteTradeRecordStore::ListBySeller(const std::string& SellerID) const
{
    std::vector<TradeRecord> Result;
    if (SellerID.empty())
    {
        return Result;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE SellerID = ?" + OrderByTimeDesc);
    if (!Query.IsReady())
    {
        return Result;
    }

    Query.BindText(1, SellerID);
    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::vector<TradeRecord> SqliteTradeRecordStore::ListByItem(const std::string& ItemID) const
{
    std::vector<TradeRecord> Result;
    if (ItemID.empty())
    {
        return Result;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ItemID = ?" + OrderByTimeDesc);
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

std::vector<TradeRecord> SqliteTradeRecordStore::All() const
{
    std::vector<TradeRecord> Result;
    SqliteStatement Query(Database, std::string(SelectColumns) + std::string(OrderByTimeDesc).substr(1));
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

std::size_t SqliteTradeRecordStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM trade_records;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

TradeRecord SqliteTradeRecordStore::Load(SqliteStatement& Row)
{
    TradeRecord Record;
    Record.TradeID = Row.Text(0);
    Record.ReservationID = Row.Text(1);
    Record.ItemID = Row.Text(2);
    Record.BuyerID = Row.Text(3);
    Record.SellerID = Row.Text(4);
    Record.Status = static_cast<TradeStatus>(Row.Int64(5));
    Record.CreatedAtMs = Row.Int64(6);
    Record.CompletedAtMs = Row.Int64(7);
    Record.CancelledAtMs = Row.Int64(8);
    Record.CancelReason = Row.Text(9);
    Record.ItemTitle = Row.Text(10);
    Record.PriceCents = Row.Int64(11);
    Record.TradeLocation = Row.Text(12);
    Record.SellerContact = Row.Text(13);
    Record.BuyerContact = Row.Text(14);
    return Record;
}
