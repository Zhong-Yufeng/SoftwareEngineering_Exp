#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"TradeRecordStore.h"

class SqliteStatement;

// TradeRecordStore 的数据库实现：只有增与改，没有删
class SqliteTradeRecordStore : public TradeRecordStore
{
    public:
        explicit SqliteTradeRecordStore(SqliteDatabase& Db);

        bool Save(TradeRecord& Record) override;
        std::optional<TradeRecord> FindByID(const std::string& TradeID) const override;
        std::optional<TradeRecord> FindByReservation(const std::string& ReservationID) const override;
        std::vector<TradeRecord> ListByBuyer(const std::string& BuyerID) const override;
        std::vector<TradeRecord> ListBySeller(const std::string& SellerID) const override;
        std::vector<TradeRecord> ListByItem(const std::string& ItemID) const override;
        std::vector<TradeRecord> All() const override;
        std::size_t Size() const override;

    private:
        static TradeRecord Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
