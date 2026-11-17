#include"SellerService.h"
#include"ServiceRules.h"

namespace
{
    // 确认自己名下的商品；不属于自己时给出「只能…」的提示
    std::optional<Item> FindOwnItem(ItemStore& Items,
                                    const std::string& SellerID,
                                    const std::string& ItemID,
                                    const std::string& Message,
                                    std::string* Error)
    {
        const auto Goods = Items.FindByID(ItemID);
        if (!Goods.has_value())
        {
            SetError(Error, "商品不存在");
            return std::nullopt;
        }
        if (Goods->SellerID != SellerID)
        {
            SetError(Error, Message);
            return std::nullopt;
        }
        return Goods;
    }
}

SellerService::SellerService(UserStore& UserStoreRef,
                             ItemStore& ItemStoreRef,
                             ReservationStore& ReservationStoreRef,
                             TradeRecordStore& TradeRecordStoreRef,
                             LogService& LogStore,
                             Transaction& TransactionRef,
                             ServiceClock ClockSource)
    : Users(UserStoreRef),
      Items(ItemStoreRef),
      Reservations(ReservationStoreRef),
      Trades(TradeRecordStoreRef),
      Logs(LogStore),
      Tx(TransactionRef),
      Clock(ClockSource)
{
}

std::optional<Item> SellerService::CreateDraft(const std::string& SellerID,
                                               const ItemInfo& Info,
                                               std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return std::nullopt;
    }

    auto Created = Item::CreateDraft(Info, SellerID);
    if (!Created.has_value())
    {
        SetError(Error, "商品内容不完整或价格无效");
        return std::nullopt;
    }

    Item Saved = Created.value();
    if (!Items.Save(Saved))
    {
        SetError(Error, "商品保存失败");
        return std::nullopt;
    }
    return Saved;
}

bool SellerService::SubmitForReview(const std::string& SellerID,
                                    const std::string& ItemID,
                                    std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return false;
    }

    auto Goods = FindOwnItem(Items, SellerID, ItemID, "只能提交自己的商品", Error);
    if (!Goods.has_value())
    {
        return false;
    }
    if (!Goods->SubmitForReview())
    {
        SetError(Error, "仅草稿或驳回商品可提交审核");
        return false;
    }
    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return false;
    }
    if (!Items.Save(Goods.value()))
    {
        SetError(Error, "商品保存失败");
        return false;
    }

    Logs.Record(LogType::SubmitReview, SellerID, ServiceObject::Item,
                ItemID, "提交商品审核");
    if (!Unit.Commit())
    {
        SetError(Error, "商品保存失败");
        return false;
    }
    return true;
}

std::optional<Item> SellerService::FindItem(const std::string& SellerID,
                                            const std::string& ItemID,
                                            std::string* Error) const
{
    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return std::nullopt;
    }
    return ServiceRules::FindVisibleItem(Items, Role::Seller, SellerID, ItemID, Error);
}

std::vector<Item> SellerService::QueryItems(const ItemQuery& Cond,
                                            const std::string& SellerID) const
{
    return ServiceRules::QueryVisibleItems(Items, Cond, Role::Seller, SellerID);
}

std::vector<Item> SellerService::ListMyItems(const std::string& SellerID) const
{
    // 卖家看自己的商品不受可见性限制，草稿、驳回、下架的也要能看见
    ItemQuery Cond;
    Cond.SellerID = SellerID;
    return Items.Query(Cond);
}

bool SellerService::ConfirmReservation(const std::string& SellerID,
                                       const std::string& ItemID,
                                       const std::string& ReservationID,
                                       std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return false;
    }

    auto Goods = FindOwnItem(Items, SellerID, ItemID, "只能确认自己商品的预约", Error);
    if (!Goods.has_value())
    {
        return false;
    }

    if (Goods->Status == ItemStatus::Confirmed || Goods->Status == ItemStatus::Traded)
    {
        SetError(Error, "商品已被约走");
        return false;
    }
    if (Goods->Status != ItemStatus::Published)
    {
        SetError(Error, "商品当前不可确认");
        return false;
    }

    const auto Order = Reservations.FindByID(ReservationID);
    if (!Order.has_value())
    {
        SetError(Error, "预约不存在");
        return false;
    }
    if (Order->ItemID != ItemID)
    {
        SetError(Error, "预约与商品不匹配");
        return false;
    }
    if (Order->Status != ReservationStatus::PendingConfirm)
    {
        SetError(Error, "该预约已被处理");
        return false;
    }

    if (!Goods->Confirm())
    {
        SetError(Error, "商品状态变更失败");
        return false;
    }

    Reservation Confirmed = Order.value();
    if (!Confirmed.Confirm(Clock()))
    {
        SetError(Error, "预约确认失败");
        return false;
    }

    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return false;
    }
    if (!Items.Save(Goods.value()) || !Reservations.Save(Confirmed))
    {
        SetError(Error, "预约确认保存失败");
        return false;
    }

    // 一物一件：其余待确认的预约在确认瞬间即失去意义（REQ-012）
    const auto Others = Reservations.ListByItem(ItemID);
    for (const Reservation& Other : Others)
    {
        if (Other.ReservationID == ReservationID
            || Other.Status != ReservationStatus::PendingConfirm)
        {
            continue;
        }
        Reservations.Remove(Other.ReservationID);
        Logs.Record(LogType::DeleteReservation, SellerID, ServiceObject::Reservation,
                    Other.ReservationID, "其他预约未被选中，记录删除");
    }

    // 确认成功即生成交易记录：之后商品下架、预约被删，买家仍能查到自己约成了什么
    const auto Buyer = Users.FindByID(Confirmed.BuyerID);
    if (!Buyer.has_value())
    {
        SetError(Error, "买家不存在");
        return false;
    }

    const auto NewTrade = TradeRecord::Create(Confirmed, Goods.value(), Buyer->QQNo, Clock());
    if (!NewTrade.has_value())
    {
        SetError(Error, "交易记录生成失败");
        return false;
    }

    TradeRecord Record = NewTrade.value();
    if (!Trades.Save(Record))
    {
        SetError(Error, "交易记录保存失败");
        return false;
    }

    Logs.Record(LogType::ConfirmReservation, SellerID, ServiceObject::Reservation,
                ReservationID, "确认预约");
    if (!Unit.Commit())
    {
        SetError(Error, "预约确认保存失败");
        return false;
    }
    return true;
}

bool SellerService::CompleteTrade(const std::string& SellerID,
                                  const std::string& ItemID,
                                  std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return false;
    }

    auto Goods = FindOwnItem(Items, SellerID, ItemID, "只能完成自己商品的交易", Error);
    if (!Goods.has_value())
    {
        return false;
    }
    if (!Goods->CompleteTrade())
    {
        SetError(Error, "仅已确认商品可完成交易");
        return false;
    }

    // 记录在卖家确认时生成，这里只把它从进行中推进到已完成
    std::optional<TradeRecord> Record;
    for (const TradeRecord& Trade : Trades.ListByItem(ItemID))
    {
        if (Trade.Status == TradeStatus::InProgress)
        {
            Record = Trade;
            break;
        }
    }
    if (!Record.has_value())
    {
        SetError(Error, "未找到进行中的交易记录");
        return false;
    }
    if (!Record->Complete(Clock()))
    {
        SetError(Error, "交易记录状态变更失败");
        return false;
    }

    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return false;
    }
    if (!Items.Save(Goods.value()))
    {
        SetError(Error, "交易状态保存失败");
        return false;
    }

    if (!Trades.Save(Record.value()))
    {
        SetError(Error, "交易记录保存失败");
        return false;
    }

    Logs.Record(LogType::CompleteTrade, SellerID, ServiceObject::Item,
                ItemID, "完成交易");
    if (!Unit.Commit())
    {
        SetError(Error, "交易记录保存失败");
        return false;
    }
    return true;
}

std::vector<Reservation> SellerService::ListItemReservations(const std::string& SellerID,
                                                             const std::string& ItemID,
                                                             std::string* Error) const
{
    ClearError(Error);

    if (!FindOwnItem(Items, SellerID, ItemID, "只能查看自己商品的预约", Error).has_value())
    {
        return {};
    }
    return Reservations.ListByItem(ItemID);
}

std::vector<TradeRecord> SellerService::ListItemTrades(const std::string& SellerID,
                                                       const std::string& ItemID,
                                                       std::string* Error) const
{
    ClearError(Error);

    if (!FindOwnItem(Items, SellerID, ItemID, "只能查看自己商品的交易记录", Error).has_value())
    {
        return {};
    }
    return Trades.ListByItem(ItemID);
}

std::vector<TradeRecord> SellerService::ListMyTrades(const std::string& SellerID) const
{
    return Trades.ListBySeller(SellerID);
}

SellerStats SellerService::GetSellerStats(const std::string& SellerID) const
{
    SellerStats Result;

    for (const Item& Goods : ListMyItems(SellerID))
    {
        if (Goods.Status == ItemStatus::Published || Goods.Status == ItemStatus::Confirmed)
        {
            ++Result.OnSaleCount;
        }
        if (Goods.Status == ItemStatus::Traded)
        {
            ++Result.CompletedCount;
        }
    }
    return Result;
}

bool SellerService::ModifyItem(const std::string& SellerID,
                               const std::string& ItemID,
                               const ItemInfo& Info,
                               std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return false;
    }

    auto Own = FindOwnItem(Items, SellerID, ItemID, "只能修改自己的商品", Error);
    if (!Own.has_value())
    {
        return false;
    }

    Item Goods = Own.value();
    if (!Goods.Modify(Info))
    {
        SetError(Error, "当前状态不可修改，或商品内容不完整");
        return false;
    }
    if (!Items.Save(Goods))
    {
        SetError(Error, "商品保存失败");
        return false;
    }
    return true;
}

bool SellerService::OffShelfItem(const std::string& SellerID,
                                 const std::string& ItemID,
                                 std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireSeller(Users, SellerID, Error))
    {
        return false;
    }

    auto Own = FindOwnItem(Items, SellerID, ItemID, "只能下架自己的商品", Error);
    if (!Own.has_value())
    {
        return false;
    }

    Item Goods = Own.value();
    if (!Goods.OffShelf())
    {
        SetError(Error, "仅已发布或已确认的商品可下架");
        return false;
    }

    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return false;
    }
    if (!Items.Save(Goods))
    {
        SetError(Error, "商品保存失败");
        return false;
    }

    // 下架后商品不可再交易：进行中的交易作废，相关预约记录删除
    ServiceRules::CancelTradesByItem(Trades, ItemID, "商品已下架", Clock());
    Reservations.RemoveByItem(ItemID);

    Logs.Record(LogType::OffShelf, SellerID, ServiceObject::Item, ItemID, "卖家下架商品");
    if (!Unit.Commit())
    {
        SetError(Error, "商品保存失败");
        return false;
    }
    return true;
}
