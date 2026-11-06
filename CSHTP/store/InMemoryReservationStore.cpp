#include"InMemoryReservationStore.h"

bool InMemoryReservationStore::Save(Reservation& Order)
{
    // 没有编号 = 新预约，由存储层分配一个
    if (Order.ReservationID.empty())
    {
        Order.ReservationID = MakeNextID();
    }

    // 有编号 = 新增或覆盖。这里可以直接用 insert_or_assign：
    // Reservation 的字段是公开的，不受「默认构造函数私有」的限制。
    Orders.insert_or_assign(Order.ReservationID, Order);
    return true;
}

std::optional<Reservation> InMemoryReservationStore::FindByID(const std::string& ReservationID) const
{
    const auto It = Orders.find(ReservationID);
    if (It == Orders.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::optional<Reservation> InMemoryReservationStore::FindByBuyerAndItem(const std::string& BuyerID,
                                                                         const std::string& ItemID) const
{
    // 空买家或空商品不可能存在对应记录；提前返回省一次遍历，
    // 也避免「两个空串相等」造成的误命中
    if (BuyerID.empty() || ItemID.empty())
    {
        return std::nullopt;
    }

    for (const auto& Entry : Orders)
    {
        const Reservation& Order = Entry.second;
        if (Order.BuyerID == BuyerID && Order.ItemID == ItemID)
        {
            return Order;
        }
    }
    return std::nullopt;
}

std::vector<Reservation> InMemoryReservationStore::ListByBuyer(const std::string& BuyerID) const
{
    std::vector<Reservation> Out;

    if (BuyerID.empty())
    {
        // 空串不等于「不限」。若这里返回全部，调用方误传空串
        // 就会把别人的预约整批拿到，是危险的默认值。
        return Out;
    }

    for (const auto& Entry : Orders)
    {
        if (Entry.second.BuyerID == BuyerID)
        {
            Out.push_back(Entry.second);
        }
    }
    return Out;
}

std::vector<Reservation> InMemoryReservationStore::ListByItem(const std::string& ItemID) const
{
    std::vector<Reservation> Out;

    if (ItemID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Orders)
    {
        if (Entry.second.ItemID == ItemID)
        {
            Out.push_back(Entry.second);
        }
    }
    return Out;
}

std::size_t InMemoryReservationStore::CountByBuyer(const std::string& BuyerID) const
{
    if (BuyerID.empty())
    {
        return 0;
    }

    std::size_t Count = 0;
    for (const auto& Entry : Orders)
    {
        if (Entry.second.BuyerID == BuyerID)
        {
            ++Count;
        }
    }
    return Count;
}

std::size_t InMemoryReservationStore::CountByItem(const std::string& ItemID) const
{
    if (ItemID.empty())
    {
        return 0;
    }

    std::size_t Count = 0;
    for (const auto& Entry : Orders)
    {
        if (Entry.second.ItemID == ItemID)
        {
            ++Count;
        }
    }
    return Count;
}

bool InMemoryReservationStore::Remove(const std::string& ReservationID)
{
    // erase 返回删掉的个数，0 表示本来就没有
    return Orders.erase(ReservationID) > 0;
}

std::size_t InMemoryReservationStore::RemoveByItem(const std::string& ItemID)
{
    if (ItemID.empty())
    {
        return 0;
    }

    // 边遍历边删会让迭代器失效，所以分成两步：先记下要删的编号，再统一删。
    // 若直接在 for 里 erase(It++)，写法容易出错，也没必要省这一次遍历。
    std::vector<std::string> ToErase;
    for (const auto& Entry : Orders)
    {
        if (Entry.second.ItemID == ItemID)
        {
            ToErase.push_back(Entry.first);
        }
    }

    for (const std::string& ID : ToErase)
    {
        Orders.erase(ID);
    }
    return ToErase.size();
}

std::size_t InMemoryReservationStore::Size() const
{
    return Orders.size();
}

std::string InMemoryReservationStore::MakeNextID()
{
    // 循环是为了防御「编号已被占用」（比如以后从文件读入数据）
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "res-" + std::to_string(LastAssignedNumber);
        if (Orders.find(Candidate) == Orders.end())
        {
            return Candidate;
        }
    }
}
