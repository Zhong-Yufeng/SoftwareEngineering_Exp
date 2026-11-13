#include"InMemoryItemStore.h"
#include<algorithm>

bool InMemoryItemStore::Save(Item& Goods)
{
    if (Goods.ItemID.empty())
    {
        Goods.ItemID = MakeNextID();
    }

    Items.insert_or_assign(Goods.ItemID, Goods);
    return true;
}

std::optional<Item> InMemoryItemStore::FindByID(const std::string& ItemID) const
{
    const auto It = Items.find(ItemID);
    if (It == Items.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::vector<Item> InMemoryItemStore::Query(const ItemQuery& Cond) const
{
    std::vector<Item> Out;
    Out.reserve(Items.size());

    for (const auto& Entry : Items)
    {
        if (Matches(Entry.second, Cond))
        {
            Out.push_back(Entry.second);
        }
    }

    switch (Cond.Sort)
    {
    case ItemSort::PriceAsc:
        std::sort(Out.begin(), Out.end(),
                  [](const Item& A, const Item& B) { return A.PriceCents < B.PriceCents; });
        break;
    case ItemSort::PriceDesc:
        std::sort(Out.begin(), Out.end(),
                  [](const Item& A, const Item& B) { return A.PriceCents > B.PriceCents; });
        break;
    case ItemSort::PublishedNewest:
        std::sort(Out.begin(), Out.end(),
                  [](const Item& A, const Item& B) { return A.PublishedAtMs > B.PublishedAtMs; });
        break;
    case ItemSort::None:
    default:
        break;
    }

    return Out;
}

std::vector<Item> InMemoryItemStore::All() const
{
    std::vector<Item> Out;
    Out.reserve(Items.size());
    for (const auto& Entry : Items)
    {
        Out.push_back(Entry.second);
    }
    return Out;
}

bool InMemoryItemStore::Remove(const std::string& ItemID)
{
    return Items.erase(ItemID) > 0;
}

std::size_t InMemoryItemStore::Size() const
{
    return Items.size();
}

std::string InMemoryItemStore::MakeNextID()
{
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "item-" + std::to_string(LastAssignedNumber);
        if (Items.find(Candidate) == Items.end())
        {
            return Candidate;
        }
    }
}

bool InMemoryItemStore::Matches(const Item& Goods, const ItemQuery& Cond) const
{
    if (Cond.Keyword.has_value())
    {
        const bool InTitle = Goods.Title.find(Cond.Keyword.value()) != std::string::npos;
        const bool InDescription = Goods.Description.find(Cond.Keyword.value()) != std::string::npos;
        if (!InTitle && !InDescription)
        {
            return false;
        }
    }

    if (Cond.ItemCategory.has_value() && Goods.ItemCategory != Cond.ItemCategory.value())
    {
        return false;
    }

    if (Cond.ItemCondition.has_value() && Goods.ItemCondition != Cond.ItemCondition.value())
    {
        return false;
    }

    if (Cond.MinPriceCents.has_value() && Goods.PriceCents < Cond.MinPriceCents.value())
    {
        return false;
    }

    if (Cond.MaxPriceCents.has_value() && Goods.PriceCents > Cond.MaxPriceCents.value())
    {
        return false;
    }

    if (Cond.Status.has_value() && Goods.Status != Cond.Status.value())
    {
        return false;
    }

    if (Cond.SellerID.has_value() && Goods.SellerID != Cond.SellerID.value())
    {
        return false;
    }

    if (Cond.TradeLocation.has_value()
        && Goods.TradeLocation.find(Cond.TradeLocation.value()) == std::string::npos)
    {
        return false;
    }

    if (Cond.PublishedAfterMs.has_value() && Goods.PublishedAtMs < Cond.PublishedAfterMs.value())
    {
        return false;
    }

    return true;
}
