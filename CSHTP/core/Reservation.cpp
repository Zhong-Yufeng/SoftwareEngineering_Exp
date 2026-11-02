#include"Reservation.h"

std::optional<Reservation> Reservation::Create(const std::string& BuyerID,
                                               const std::string& ItemID,
                                               long long CreatedAtMs)
{
    // 谁约的、约的什么，都不能缺
    if (BuyerID.empty() || ItemID.empty())
    {
        return std::nullopt;
    }

    // 发起时间必须是有效正值
    if (CreatedAtMs <= 0)
    {
        return std::nullopt;
    }

    Reservation R;

    // ID 留给存储层；确认时间在确认那一刻才写
    R.ReservationID = "";
    R.BuyerID = BuyerID;
    R.ItemID = ItemID;
    R.CreatedAtMs = CreatedAtMs;
    R.Status = ReservationStatus::PendingConfirm;
    R.ConfirmedAtMs = std::nullopt;

    return R;
}

bool Reservation::Confirm(long long NowMs)
{
    // 只有待确认的记录能确认
    if (Status != ReservationStatus::PendingConfirm)
    {
        return false;
    }

    // 确认时间必须有效
    if (NowMs <= 0)
    {
        return false;
    }

    Status = ReservationStatus::Succeeded;
    ConfirmedAtMs = NowMs;
    return true;
}
