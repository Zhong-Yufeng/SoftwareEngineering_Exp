#pragma once
#include<optional>
#include<string>
#include<vector>
#include<cstddef>
#include"TradeRecord.h"

// 成交记录的存取。
// 成交记录是买卖双方唯一的凭证、也是平台流水的来源，所以只增不删，没有 Remove。
class TradeRecordStore
{
    public:
        virtual ~TradeRecordStore() = default;

        // 编号为空时由存储层分配
        virtual bool Save(TradeRecord& Record) = 0;
        virtual std::optional<TradeRecord> FindByID(const std::string& TradeID) const = 0;
        virtual std::optional<TradeRecord> FindByReservation(const std::string& ReservationID) const = 0;
        virtual std::vector<TradeRecord> ListByBuyer(const std::string& BuyerID) const = 0;
        virtual std::vector<TradeRecord> ListBySeller(const std::string& SellerID) const = 0;
        virtual std::vector<TradeRecord> ListByItem(const std::string& ItemID) const = 0;
        virtual std::vector<TradeRecord> All() const = 0;
        virtual std::size_t Size() const = 0;
};
