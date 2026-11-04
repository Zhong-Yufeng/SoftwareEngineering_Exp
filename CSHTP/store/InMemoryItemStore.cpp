#include"InMemoryItemStore.h"
#include<algorithm>

bool InMemoryItemStore::Save(Item& Goods)
{
    // 没有编号 = 新商品，由存储层分配一个
    if (Goods.GetItemID().empty())
    {
        if (!Goods.AssignID(MakeNextID()))
        {
            return false;
        }
    }

    // 有编号 = 新增或覆盖。用 insert_or_assign 而不是 operator[]，
    // 因为 operator[] 要求 Item 可默认构造（它是私有的）。
    Items.insert_or_assign(Goods.GetItemID(), Goods);
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

    // 先筛后排：排序只对筛出来的结果做，不会打乱存储本身
    switch (Cond.Sort)
    {
        case ItemSort::PriceAsc:
            std::sort(Out.begin(), Out.end(),
                      [](const Item& A, const Item& B)
                      {
                          return A.GetPriceCents() < B.GetPriceCents();
                      });
            break;

        case ItemSort::PriceDesc:
            std::sort(Out.begin(), Out.end(),
                      [](const Item& A, const Item& B)
                      {
                          return A.GetPriceCents() > B.GetPriceCents();
                      });
            break;

        case ItemSort::PublishedNewest:
            std::sort(Out.begin(), Out.end(),
                      [](const Item& A, const Item& B)
                      {
                          return A.GetPublishedAtMs() > B.GetPublishedAtMs();
                      });
            break;

        case ItemSort::None:
        default:
            break;
    }

    return Out;
}

bool InMemoryItemStore::Remove(const std::string& ItemID)
{
    // erase 返回删掉的个数，0 表示本来就没有
    return Items.erase(ItemID) > 0;
}

std::size_t InMemoryItemStore::Size() const
{
    return Items.size();
}

std::string InMemoryItemStore::MakeNextID()
{
    // 正常情况下 ++ 一次就能拿到空编号；循环是为了防御「编号被别人占用」
    //（比如以后从文件读入数据，里面已经存在 item-3）
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
    // 关键词只要出现在标题或描述任意一处即可
    if (Cond.Keyword.has_value())
    {
        const bool InTitle = Goods.GetTitle().find(Cond.Keyword.value()) != std::string::npos;
        const bool InDescription = Goods.GetDescription().find(Cond.Keyword.value()) != std::string::npos;
        if (!InTitle && !InDescription)
        {
            return false;
        }
    }

    if (Cond.ItemCategory.has_value() && Goods.GetCategory() != Cond.ItemCategory.value())
    {
        return false;
    }

    if (Cond.ItemCondition.has_value() && Goods.GetCondition() != Cond.ItemCondition.value())
    {
        return false;
    }

    // 价格区间两端都包含
    if (Cond.MinPriceCents.has_value() && Goods.GetPriceCents() < Cond.MinPriceCents.value())
    {
        return false;
    }

    if (Cond.MaxPriceCents.has_value() && Goods.GetPriceCents() > Cond.MaxPriceCents.value())
    {
        return false;
    }

    if (Cond.Status.has_value() && Goods.GetStatus() != Cond.Status.value())
    {
        return false;
    }

    if (Cond.SellerID.has_value() && Goods.GetSellerID() != Cond.SellerID.value())
    {
        return false;
    }

    if (Cond.TradeLocation.has_value()
        && Goods.GetTradeLocation().find(Cond.TradeLocation.value()) == std::string::npos)
    {
        return false;
    }

    if (Cond.PublishedAfterMs.has_value() && Goods.GetPublishedAtMs() < Cond.PublishedAfterMs.value())
    {
        return false;
    }

    return true;
}
