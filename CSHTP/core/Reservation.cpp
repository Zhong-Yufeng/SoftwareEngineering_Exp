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

    // ID 留给存储层
    R.ReservationID = "";
    R.BuyerID = BuyerID;
    R.ItemID = ItemID;
    R.CreatedAtMs = CreatedAtMs;

    return R;
}
