#pragma once
#include<optional>
#include<vector>
#include<string>
#include<cstddef>
#include"Reservation.h"

class ReservationStore
{
    public:
        virtual ~ReservationStore() = default;

        // 编号为空时由存储层分配
        virtual bool Save(Reservation& Order) = 0;
        virtual std::optional<Reservation> FindByID(const std::string& ReservationID) const = 0;
        virtual std::optional<Reservation> FindByBuyerAndItem(const std::string& BuyerID,
                                                              const std::string& ItemID) const = 0;
        virtual std::vector<Reservation> ListByBuyer(const std::string& BuyerID) const = 0;
        virtual std::vector<Reservation> ListByItem(const std::string& ItemID) const = 0;
        virtual bool Remove(const std::string& ReservationID) = 0;
        virtual std::size_t RemoveByItem(const std::string& ItemID) = 0;
        virtual std::size_t Size() const = 0;
};
