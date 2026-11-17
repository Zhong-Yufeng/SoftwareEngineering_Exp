#include"SqliteReservationStore.h"
#include"SqliteStatement.h"

namespace
{
    const char* const SelectColumns =
        "SELECT ReservationID, BuyerID, ItemID, CreatedAtMs, Status, ConfirmedAtMs "
        "FROM reservations ";

    const char* const OrderByTime = " ORDER BY CreatedAtMs ASC, ReservationID ASC;";
}

SqliteReservationStore::SqliteReservationStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteReservationStore::Save(Reservation& Order)
{
    if (Order.ReservationID.empty())
    {
        if (!Database.NextID("res", "res-", Order.ReservationID))
        {
            return false;
        }
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO reservations(ReservationID, BuyerID, ItemID, CreatedAtMs, "
                           "Status, ConfirmedAtMs) VALUES(?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT(ReservationID) DO UPDATE SET "
                           "BuyerID = excluded.BuyerID, ItemID = excluded.ItemID, "
                           "CreatedAtMs = excluded.CreatedAtMs, Status = excluded.Status, "
                           "ConfirmedAtMs = excluded.ConfirmedAtMs;");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Order.ReservationID);
    Insert.BindText(2, Order.BuyerID);
    Insert.BindText(3, Order.ItemID);
    Insert.BindInt64(4, Order.CreatedAtMs);
    Insert.BindInt64(5, static_cast<long long>(Order.Status));
    if (Order.ConfirmedAtMs.has_value())
    {
        Insert.BindInt64(6, Order.ConfirmedAtMs.value());
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

std::optional<Reservation> SqliteReservationStore::FindByID(const std::string& ReservationID) const
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

std::optional<Reservation> SqliteReservationStore::FindByBuyerAndItem(const std::string& BuyerID,
                                                                        const std::string& ItemID) const
{
    if (BuyerID.empty() || ItemID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns)
                                      + "WHERE BuyerID = ? AND ItemID = ?" + OrderByTime);
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, BuyerID);
    Query.BindText(2, ItemID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::vector<Reservation> SqliteReservationStore::ListByBuyer(const std::string& BuyerID) const
{
    std::vector<Reservation> Result;
    if (BuyerID.empty())
    {
        return Result;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE BuyerID = ?" + OrderByTime);
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

std::vector<Reservation> SqliteReservationStore::ListByItem(const std::string& ItemID) const
{
    std::vector<Reservation> Result;
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

bool SqliteReservationStore::Remove(const std::string& ReservationID)
{
    if (ReservationID.empty())
    {
        return false;
    }

    SqliteStatement Delete(Database, "DELETE FROM reservations WHERE ReservationID = ?;");
    if (!Delete.IsReady())
    {
        return false;
    }

    Delete.BindText(1, ReservationID);
    if (Delete.Step() || Delete.HasError())
    {
        return false;
    }

    SqliteStatement Count(Database, "SELECT changes();");
    if (!Count.IsReady() || !Count.Step())
    {
        return false;
    }
    return Count.Int64(0) > 0;
}

std::size_t SqliteReservationStore::RemoveByItem(const std::string& ItemID)
{
    if (ItemID.empty())
    {
        return 0;
    }

    SqliteStatement Delete(Database, "DELETE FROM reservations WHERE ItemID = ?;");
    if (!Delete.IsReady())
    {
        return 0;
    }

    Delete.BindText(1, ItemID);
    if (Delete.Step() || Delete.HasError())
    {
        return 0;
    }

    SqliteStatement Count(Database, "SELECT changes();");
    if (!Count.IsReady() || !Count.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Count.Int64(0));
}

std::size_t SqliteReservationStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM reservations;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

Reservation SqliteReservationStore::Load(SqliteStatement& Row)
{
    Reservation Order;
    Order.ReservationID = Row.Text(0);
    Order.BuyerID = Row.Text(1);
    Order.ItemID = Row.Text(2);
    Order.CreatedAtMs = Row.Int64(3);
    Order.Status = static_cast<ReservationStatus>(Row.Int64(4));
    if (!Row.IsNull(5))
    {
        Order.ConfirmedAtMs = Row.Int64(5);
    }
    return Order;
}
