#pragma once
#include<optional>
#include<vector>
#include<string>
#include<cstddef>
#include"Item.h"

enum class ItemSort
{
    None,
    PriceAsc,
    PriceDesc,
    PublishedNewest
};

struct ItemQuery
{
    std::optional<std::string> Keyword;
    std::optional<Category> ItemCategory;
    std::optional<Condition> ItemCondition;
    std::optional<long long> MinPriceCents;
    std::optional<long long> MaxPriceCents;
    std::optional<ItemStatus> Status;
    std::optional<std::string> SellerID;
    std::optional<std::string> TradeLocation;
    std::optional<long long> PublishedAfterMs;

    ItemSort Sort = ItemSort::None;
};

class ItemStore
{
    public:
        virtual ~ItemStore() = default;

        // 编号为空时由存储层分配
        virtual bool Save(Item& Goods) = 0;
        virtual std::optional<Item> FindByID(const std::string& ItemID) const = 0;
        virtual std::vector<Item> Query(const ItemQuery& Cond) const = 0;
        virtual std::vector<Item> All() const = 0;
        virtual bool Remove(const std::string& ItemID) = 0;
        virtual std::size_t Size() const = 0;
};
