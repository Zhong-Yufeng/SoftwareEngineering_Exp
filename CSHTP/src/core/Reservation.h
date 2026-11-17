#pragma once
#include<optional>
#include<string>
#include"enums.h"

class SqliteReservationStore;

// 预约：买家对某件商品的购买意向，卖家确认后生效
// 失效的预约由存储层直接删除，不保留作废状态。
class Reservation
{
    public:
        std::string ReservationID;      // 由存储层分配
        std::string BuyerID;
        std::string ItemID;
        long long CreatedAtMs = 0;
        ReservationStatus Status = ReservationStatus::PendingConfirm;
        std::optional<long long> ConfirmedAtMs;

        static std::optional<Reservation> Create(const std::string& BuyerID,
                                                 const std::string& ItemID,
                                                 long long CreatedAtMs);

        // 待确认 → 已成功
        bool Confirm(long long NowMs);

    private:
        friend class SqliteReservationStore;
        Reservation() = default;
};
