#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"ItemStore.h"

class SqliteStatement;

// ItemStore 的数据库实现，查询语义与 InMemoryItemStore 一致
class SqliteItemStore : public ItemStore
{
    public:
        explicit SqliteItemStore(SqliteDatabase& Db);

        bool Save(Item& Goods) override;
        std::optional<Item> FindByID(const std::string& ItemID) const override;
        std::vector<Item> Query(const ItemQuery& Cond) const override;
        std::vector<Item> All() const override;
        bool Remove(const std::string& ItemID) override;
        std::size_t Size() const override;

    private:
        static Item Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
