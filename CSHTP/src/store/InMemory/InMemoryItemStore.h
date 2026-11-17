#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"ItemStore.h"

class InMemoryItemStore : public ItemStore
{
    public:
        bool Save(Item& Goods) override;
        std::optional<Item> FindByID(const std::string& ItemID) const override;
        std::vector<Item> Query(const ItemQuery& Cond) const override;
        std::vector<Item> All() const override;
        bool Remove(const std::string& ItemID) override;
        std::size_t Size() const override;

    private:
        std::string MakeNextID();
        bool Matches(const Item& Goods, const ItemQuery& Cond) const;

        std::unordered_map<std::string, Item> Items;
        long long LastAssignedNumber = 0;
};
