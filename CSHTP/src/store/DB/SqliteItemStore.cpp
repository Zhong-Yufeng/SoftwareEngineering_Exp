#include"SqliteItemStore.h"
#include"SqliteStatement.h"
#include<functional>

namespace
{
    const char* const SelectColumns =
        "SELECT ItemID, Title, Description, Category, PriceCents, Condition, TradeLocation, "
        "Contact, Status, PublishedAtMs, RejectReason, SellerID FROM items ";
}

SqliteItemStore::SqliteItemStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteItemStore::Save(Item& Goods)
{
    if (Goods.ItemID.empty())
    {
        if (!Database.NextID("item", "item-", Goods.ItemID))
        {
            return false;
        }
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO items(ItemID, Title, Description, Category, PriceCents, "
                           "Condition, TradeLocation, Contact, Status, PublishedAtMs, RejectReason, "
                           "SellerID) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT(ItemID) DO UPDATE SET "
                           "Title = excluded.Title, Description = excluded.Description, "
                           "Category = excluded.Category, PriceCents = excluded.PriceCents, "
                           "Condition = excluded.Condition, TradeLocation = excluded.TradeLocation, "
                           "Contact = excluded.Contact, Status = excluded.Status, "
                           "PublishedAtMs = excluded.PublishedAtMs, "
                           "RejectReason = excluded.RejectReason, SellerID = excluded.SellerID;");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Goods.ItemID);
    Insert.BindText(2, Goods.Title);
    Insert.BindText(3, Goods.Description);
    Insert.BindInt64(4, static_cast<long long>(Goods.ItemCategory));
    Insert.BindInt64(5, Goods.PriceCents);
    Insert.BindInt64(6, static_cast<long long>(Goods.ItemCondition));
    Insert.BindText(7, Goods.TradeLocation);
    Insert.BindText(8, Goods.Contact);
    Insert.BindInt64(9, static_cast<long long>(Goods.Status));
    Insert.BindInt64(10, Goods.PublishedAtMs);
    if (Goods.RejectReason.has_value())
    {
        Insert.BindText(11, Goods.RejectReason.value());
    }
    else
    {
        Insert.BindNull(11);
    }
    Insert.BindText(12, Goods.SellerID);

    if (Insert.Step() || Insert.HasError())
    {
        return false;
    }
    return true;
}

std::optional<Item> SqliteItemStore::FindByID(const std::string& ItemID) const
{
    if (ItemID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE ItemID = ?;");
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

std::vector<Item> SqliteItemStore::Query(const ItemQuery& Cond) const
{
    std::vector<Item> Result;

    std::string Sql = SelectColumns;
    std::vector<std::string> Where;
    std::vector<std::function<void(SqliteStatement&, int)>> Binds;

    if (Cond.Keyword.has_value())
    {
        // 用 instr 做区分大小写的子串匹配，与内存版的 std::string::find 保持一致
        Where.push_back("(instr(Title, ?) > 0 OR instr(Description, ?) > 0)");
        const std::string Keyword = Cond.Keyword.value();
        Binds.push_back([Keyword](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Keyword);
                        });
        Binds.push_back([Keyword](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Keyword);
                        });
    }

    if (Cond.ItemCategory.has_value())
    {
        const long long Value = static_cast<long long>(Cond.ItemCategory.value());
        Where.push_back("Category = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (Cond.ItemCondition.has_value())
    {
        const long long Value = static_cast<long long>(Cond.ItemCondition.value());
        Where.push_back("Condition = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (Cond.MinPriceCents.has_value())
    {
        const long long Value = Cond.MinPriceCents.value();
        Where.push_back("PriceCents >= ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (Cond.MaxPriceCents.has_value())
    {
        const long long Value = Cond.MaxPriceCents.value();
        Where.push_back("PriceCents <= ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (Cond.Status.has_value())
    {
        const long long Value = static_cast<long long>(Cond.Status.value());
        Where.push_back("Status = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (Cond.SellerID.has_value())
    {
        const std::string Value = Cond.SellerID.value();
        Where.push_back("SellerID = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Value);
                        });
    }

    if (Cond.TradeLocation.has_value())
    {
        // 同样区分大小写，与内存版一致
        Where.push_back("instr(TradeLocation, ?) > 0");
        const std::string Keyword = Cond.TradeLocation.value();
        Binds.push_back([Keyword](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Keyword);
                        });
    }

    if (Cond.PublishedAfterMs.has_value())
    {
        const long long Value = Cond.PublishedAfterMs.value();
        Where.push_back("PublishedAtMs >= ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    for (std::size_t Index = 0; Index < Where.size(); ++Index)
    {
        Sql += (Index == 0 ? "WHERE " : " AND ");
        Sql += Where[Index];
    }

    switch (Cond.Sort)
    {
    case ItemSort::PriceAsc:
        Sql += " ORDER BY PriceCents ASC, ItemID ASC";
        break;
    case ItemSort::PriceDesc:
        Sql += " ORDER BY PriceCents DESC, ItemID ASC";
        break;
    case ItemSort::PublishedNewest:
        Sql += " ORDER BY PublishedAtMs DESC, ItemID ASC";
        break;
    case ItemSort::None:
    default:
        Sql += " ORDER BY ItemID ASC";
        break;
    }
    Sql += ";";

    SqliteStatement QueryStatement(Database, Sql);
    if (!QueryStatement.IsReady())
    {
        return Result;
    }

    int BindIndex = 1;
    for (const auto& Binder : Binds)
    {
        Binder(QueryStatement, BindIndex);
        ++BindIndex;
    }

    while (QueryStatement.Step())
    {
        Result.push_back(Load(QueryStatement));
    }
    return Result;
}

std::vector<Item> SqliteItemStore::All() const
{
    std::vector<Item> Result;
    SqliteStatement QueryStatement(Database, std::string(SelectColumns) + "ORDER BY ItemID;");
    if (!QueryStatement.IsReady())
    {
        return Result;
    }

    while (QueryStatement.Step())
    {
        Result.push_back(Load(QueryStatement));
    }
    return Result;
}

bool SqliteItemStore::Remove(const std::string& ItemID)
{
    if (ItemID.empty())
    {
        return false;
    }

    SqliteStatement Delete(Database, "DELETE FROM items WHERE ItemID = ?;");
    if (!Delete.IsReady())
    {
        return false;
    }

    Delete.BindText(1, ItemID);
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

std::size_t SqliteItemStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM items;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

Item SqliteItemStore::Load(SqliteStatement& Row)
{
    Item Goods;
    Goods.ItemID = Row.Text(0);
    Goods.Title = Row.Text(1);
    Goods.Description = Row.Text(2);
    Goods.ItemCategory = static_cast<Category>(Row.Int64(3));
    Goods.PriceCents = Row.Int64(4);
    Goods.ItemCondition = static_cast<Condition>(Row.Int64(5));
    Goods.TradeLocation = Row.Text(6);
    Goods.Contact = Row.Text(7);
    Goods.Status = static_cast<ItemStatus>(Row.Int64(8));
    Goods.PublishedAtMs = Row.Int64(9);
    if (!Row.IsNull(10))
    {
        Goods.RejectReason = Row.Text(10);
    }
    Goods.SellerID = Row.Text(11);
    return Goods;
}
