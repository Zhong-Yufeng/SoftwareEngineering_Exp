#include"InMemoryTradeRecordStore.h"
#include<algorithm>

namespace
{
    // 交易记录唯一的阅读方式是「最近的在最上面」，所以统一按生成时间倒序
    void SortByTimeDesc(std::vector<TradeRecord>& Records)
    {
        std::sort(Records.begin(), Records.end(),
                  [](const TradeRecord& A, const TradeRecord& B)
                  {
                      if (A.CreatedAtMs != B.CreatedAtMs)
                      {
                          return A.CreatedAtMs > B.CreatedAtMs;
                      }
                      return A.TradeID > B.TradeID;
                  });
    }
}

bool InMemoryTradeRecordStore::Save(TradeRecord& Record)
{
    if (Record.TradeID.empty())
    {
        Record.TradeID = MakeNextID();
    }

    Records.insert_or_assign(Record.TradeID, Record);
    return true;
}

std::optional<TradeRecord> InMemoryTradeRecordStore::FindByID(const std::string& TradeID) const
{
    const auto It = Records.find(TradeID);
    if (It == Records.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::optional<TradeRecord> InMemoryTradeRecordStore::FindByReservation(const std::string& ReservationID) const
{
    if (ReservationID.empty())
    {
        return std::nullopt;
    }

    for (const auto& Entry : Records)
    {
        if (Entry.second.ReservationID == ReservationID)
        {
            return Entry.second;
        }
    }
    return std::nullopt;
}

std::vector<TradeRecord> InMemoryTradeRecordStore::ListByBuyer(const std::string& BuyerID) const
{
    std::vector<TradeRecord> Out;
    if (BuyerID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Records)
    {
        if (Entry.second.BuyerID == BuyerID)
        {
            Out.push_back(Entry.second);
        }
    }

    SortByTimeDesc(Out);
    return Out;
}

std::vector<TradeRecord> InMemoryTradeRecordStore::ListBySeller(const std::string& SellerID) const
{
    std::vector<TradeRecord> Out;
    if (SellerID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Records)
    {
        if (Entry.second.SellerID == SellerID)
        {
            Out.push_back(Entry.second);
        }
    }

    SortByTimeDesc(Out);
    return Out;
}

std::vector<TradeRecord> InMemoryTradeRecordStore::ListByItem(const std::string& ItemID) const
{
    std::vector<TradeRecord> Out;
    if (ItemID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Records)
    {
        if (Entry.second.ItemID == ItemID)
        {
            Out.push_back(Entry.second);
        }
    }

    SortByTimeDesc(Out);
    return Out;
}

std::vector<TradeRecord> InMemoryTradeRecordStore::All() const
{
    std::vector<TradeRecord> Out;
    Out.reserve(Records.size());
    for (const auto& Entry : Records)
    {
        Out.push_back(Entry.second);
    }

    SortByTimeDesc(Out);
    return Out;
}

std::size_t InMemoryTradeRecordStore::Size() const
{
    return Records.size();
}

std::string InMemoryTradeRecordStore::MakeNextID()
{
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "trade-" + std::to_string(LastAssignedNumber);
        if (Records.find(Candidate) == Records.end())
        {
            return Candidate;
        }
    }
}
