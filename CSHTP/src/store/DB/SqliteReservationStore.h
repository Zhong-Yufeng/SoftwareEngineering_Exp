#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"ReservationStore.h"

class SqliteStatement;

// ReservationStore 的数据库实现，语义与 InMemoryReservationStore 一致
class SqliteReservationStore : public ReservationStore
{
    public:
        explicit SqliteReservationStore(SqliteDatabase& Db);

        bool Save(Reservation& Order) override;
        std::optional<Reservation> FindByID(const std::string& ReservationID) const override;
        std::optional<Reservation> FindByBuyerAndItem(const std::string& BuyerID,
                                                       const std::string& ItemID) const override;
        std::vector<Reservation> ListByBuyer(const std::string& BuyerID) const override;
        std::vector<Reservation> ListByItem(const std::string& ItemID) const override;
        bool Remove(const std::string& ReservationID) override;
        std::size_t RemoveByItem(const std::string& ItemID) override;
        std::size_t Size() const override;

    private:
        static Reservation Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
