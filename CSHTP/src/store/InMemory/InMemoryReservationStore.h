#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"ReservationStore.h"

class InMemoryReservationStore : public ReservationStore
{
    public:
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
        std::string MakeNextID();

        std::unordered_map<std::string, Reservation> Orders;
        long long LastAssignedNumber = 0;
};
