#include"InMemoryReservationStore.h"
#include<algorithm>

namespace
{
    void SortByTime(std::vector<Reservation>& Orders)
    {
        std::sort(Orders.begin(), Orders.end(),
                  [](const Reservation& A, const Reservation& B)
                  {
                      if (A.CreatedAtMs != B.CreatedAtMs)
                      {
                          return A.CreatedAtMs < B.CreatedAtMs;
                      }
                      return A.ReservationID < B.ReservationID;
                  });
    }
}

bool InMemoryReservationStore::Save(Reservation& Order)
{
    if (Order.ReservationID.empty())
    {
        Order.ReservationID = MakeNextID();
    }

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
    if (BuyerID.empty() || ItemID.empty())
    {
        return std::nullopt;
    }

    for (const auto& Entry : Orders)
    {
        if (Entry.second.BuyerID == BuyerID && Entry.second.ItemID == ItemID)
        {
            return Entry.second;
        }
    }
    return std::nullopt;
}

std::vector<Reservation> InMemoryReservationStore::ListByBuyer(const std::string& BuyerID) const
{
    std::vector<Reservation> Out;
    if (BuyerID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Orders)
    {
        if (Entry.second.BuyerID == BuyerID)
        {
            Out.push_back(Entry.second);
        }
    }

    SortByTime(Out);
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

    SortByTime(Out);
    return Out;
}

bool InMemoryReservationStore::Remove(const std::string& ReservationID)
{
    return Orders.erase(ReservationID) > 0;
}

std::size_t InMemoryReservationStore::RemoveByItem(const std::string& ItemID)
{
    if (ItemID.empty())
    {
        return 0;
    }

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
