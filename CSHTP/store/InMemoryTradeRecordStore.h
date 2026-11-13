#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"TradeRecordStore.h"

// 把成交记录放在哈希表里的实现。
// 编号由这里分配（trade-1、trade-2……），因为「不重复」是整个集合的性质。
class InMemoryTradeRecordStore : public TradeRecordStore
{
    public:
        bool Save(TradeRecord& Record) override;
        std::optional<TradeRecord> FindByID(const std::string& TradeID) const override;
        std::optional<TradeRecord> FindByReservation(const std::string& ReservationID) const override;
        std::vector<TradeRecord> ListByBuyer(const std::string& BuyerID) const override;
        std::vector<TradeRecord> ListBySeller(const std::string& SellerID) const override;
        std::vector<TradeRecord> ListByItem(const std::string& ItemID) const override;
        std::vector<TradeRecord> All() const override;
        std::size_t Size() const override;

    private:
        // 生成下一个没被占用的编号，形如 trade-1
        std::string MakeNextID();

        std::unordered_map<std::string, TradeRecord> Records;
        long long LastAssignedNumber = 0;
};
