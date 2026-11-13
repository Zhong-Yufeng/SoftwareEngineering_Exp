#include"Reservation.h"

std::optional<Reservation> Reservation::Create(const std::string& BuyerID,
                                               const std::string& ItemID,
                                               long long CreatedAtMs)
{
    if (BuyerID.empty() || ItemID.empty() || CreatedAtMs <= 0)
    {
        return std::nullopt;
    }

    Reservation R;
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
    if (Status != ReservationStatus::PendingConfirm || NowMs <= 0)
    {
        return false;
    }

    Status = ReservationStatus::Succeeded;
    ConfirmedAtMs = NowMs;
    return true;
}
