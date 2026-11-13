#include"TradeRecord.h"

std::optional<TradeRecord> TradeRecord::Create(const Reservation& Order,
                                               const Item& Goods,
                                               const std::string& BuyerContact,
                                               long long CreatedAtMs)
{
    // 记录必须挂在一条已成功的预约和一件已确认的商品上，否则不成立
    if (Order.ReservationID.empty() || Order.ItemID.empty()
        || Goods.ItemID.empty() || Goods.SellerID.empty())
    {
        return std::nullopt;
    }
    if (Order.Status != ReservationStatus::Succeeded)
    {
        return std::nullopt;
    }
    if (Goods.Status != ItemStatus::Confirmed)
    {
        return std::nullopt;
    }
    if (Order.ItemID != Goods.ItemID)
    {
        return std::nullopt;
    }

    // 买家必须有身份，且不能是自己买自己
    if (Order.BuyerID.empty() || Order.BuyerID == Goods.SellerID)
    {
        return std::nullopt;
    }

    // 成交后双方只能靠联系方式联系，缺一个这条记录就残缺
    if (BuyerContact.empty() || Goods.Contact.empty())
    {
        return std::nullopt;
    }

    if (Goods.PriceCents < 0 || CreatedAtMs <= 0)
    {
        return std::nullopt;
    }

    TradeRecord Record;
    Record.TradeID = "";                // 编号留给存储层分配
    Record.ReservationID = Order.ReservationID;
    Record.ItemID = Goods.ItemID;
    Record.BuyerID = Order.BuyerID;
    Record.SellerID = Goods.SellerID;
    Record.Status = TradeStatus::InProgress;
    Record.CreatedAtMs = CreatedAtMs;

    // 以下字段是确认那一刻的快照，之后商品、用户怎么改都不跟着变
    Record.ItemTitle = Goods.Title;
    Record.PriceCents = Goods.PriceCents;
    Record.TradeLocation = Goods.TradeLocation;
    Record.SellerContact = Goods.Contact;
    Record.BuyerContact = BuyerContact;
    return Record;
}

bool TradeRecord::Complete(long long TimeMs)
{
    // 只有进行中的交易才会结束；已完成不可再来一次，已作废更不能被「完成」
    if (Status != TradeStatus::InProgress || TimeMs <= 0)
    {
        return false;
    }

    Status = TradeStatus::Completed;
    CompletedAtMs = TimeMs;
    return true;
}

bool TradeRecord::Cancel(const std::string& Reason, long long TimeMs)
{
    // 已完成的交易已经发生，不能事后作废；只有进行中的才能被打断
    if (Status != TradeStatus::InProgress)
    {
        return false;
    }
    if (Reason.empty() || TimeMs <= 0)
    {
        return false;
    }

    Status = TradeStatus::Cancelled;
    CancelReason = Reason;
    CancelledAtMs = TimeMs;
    return true;
}

bool TradeRecord::IsParticipant(const std::string& UserID) const
{
    // 空串必须挡掉：记录里不会出现空 ID，不能因为「空 == 空」把陌生人放进来
    if (UserID.empty())
    {
        return false;
    }
    return UserID == BuyerID || UserID == SellerID;
}
