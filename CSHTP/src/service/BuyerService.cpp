#include"BuyerService.h"
#include"ServiceRules.h"

BuyerService::BuyerService(UserStore& UserStoreRef,
                           SellerApplicationStore& ApplicationStore,
                           ItemStore& ItemStoreRef,
                           ReservationStore& ReservationStoreRef,
                           TradeRecordStore& TradeRecordStoreRef,
                           LogService& LogStore,
                           Transaction& TransactionRef,
                           ServiceClock ClockSource)
    : Users(UserStoreRef),
      Applications(ApplicationStore),
      Items(ItemStoreRef),
      Reservations(ReservationStoreRef),
      Trades(TradeRecordStoreRef),
      Logs(LogStore),
      Tx(TransactionRef),
      Clock(ClockSource)
{
}

std::optional<User> BuyerService::Register(const std::string& Phone,
                                           const std::string& StdNo,
                                           const std::string& Password,
                                           const std::string& Nickname,
                                           const std::string& QQNo,
                                           std::string* Error)
{
    ClearError(Error);

    if (Phone.empty() && StdNo.empty())
    {
        SetError(Error, "手机号与学号至少填写一项");
        return std::nullopt;
    }
    if (!Phone.empty() && Users.FindByPhone(Phone).has_value())
    {
        SetError(Error, "手机号已被注册");
        return std::nullopt;
    }
    if (!StdNo.empty() && Users.FindByStdNo(StdNo).has_value())
    {
        SetError(Error, "学号已被注册");
        return std::nullopt;
    }

    const auto Candidate = User::create(Phone, StdNo, Password, Nickname, QQNo);
    if (!Candidate.has_value())
    {
        SetError(Error, "注册信息不完整：密码至少 6 位，昵称与联系方式必填");
        return std::nullopt;
    }

    User Created = *Candidate;
    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return std::nullopt;
    }
    if (!Users.Save(Created))
    {
        SetError(Error, "用户保存失败");
        return std::nullopt;
    }

    Logs.Record(LogType::Register, Created.UserID, ServiceObject::User, Created.UserID, "注册账号");
    if (!Unit.Commit())
    {
        SetError(Error, "注册结果保存失败");
        return std::nullopt;
    }
    return Created;
}

std::optional<Item> BuyerService::FindItem(const std::string& BuyerID,
                                           const std::string& ItemID,
                                           std::string* Error) const
{
    if (!ServiceRules::RequireBuyer(Users, BuyerID, Error))
    {
        return std::nullopt;
    }
    return ServiceRules::FindVisibleItem(Items, Role::Buyer, BuyerID, ItemID, Error);
}

std::vector<Item> BuyerService::QueryItems(const ItemQuery& Cond,
                                           const std::string& BuyerID) const
{
    return ServiceRules::QueryVisibleItems(Items, Cond, Role::Buyer, BuyerID);
}

std::optional<Reservation> BuyerService::Reserve(const std::string& BuyerID,
                                                 const std::string& ItemID,
                                                 std::string* Error)
{
    ClearError(Error);

    if (!ServiceRules::RequireBuyer(Users, BuyerID, Error))
    {
        return std::nullopt;
    }

    const auto Goods = Items.FindByID(ItemID);
    if (!Goods.has_value())
    {
        SetError(Error, "商品不存在");
        return std::nullopt;
    }
    if (Goods->SellerID == BuyerID)
    {
        SetError(Error, "不能预约自己的商品");
        return std::nullopt;
    }

    // 重复提交返回原结果，而不是报错（REQ-010）
    const auto Existing = Reservations.FindByBuyerAndItem(BuyerID, ItemID);
    if (Existing.has_value())
    {
        return Existing;
    }
    if (Goods->Status != ItemStatus::Published)
    {
        SetError(Error, "商品当前不可预约");
        return std::nullopt;
    }

    auto Created = Reservation::Create(BuyerID, ItemID, Clock());
    if (!Created.has_value())
    {
        SetError(Error, "预约信息不完整");
        return std::nullopt;
    }

    Reservation Saved = Created.value();
    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return std::nullopt;
    }
    if (!Reservations.Save(Saved))
    {
        SetError(Error, "预约保存失败");
        return std::nullopt;
    }

    Logs.Record(LogType::CreateReservation, BuyerID, ServiceObject::Reservation,
                Saved.ReservationID, "发起预约");
    if (!Unit.Commit())
    {
        SetError(Error, "预约保存失败");
        return std::nullopt;
    }
    return Saved;
}

bool BuyerService::CancelReservation(const std::string& BuyerID,
                                     const std::string& ItemID,
                                     const std::string& ReservationID,
                                     std::string* Error)
{
    ClearError(Error);

    const auto Order = Reservations.FindByID(ReservationID);
    if (!Order.has_value())
    {
        // 旧页面拿着已失效的预约来操作：按商品当前状态提示（REQ-020）
        const auto Goods = Items.FindByID(ItemID);
        if (Goods.has_value()
            && (Goods->Status == ItemStatus::Confirmed || Goods->Status == ItemStatus::Traded))
        {
            SetError(Error, "商品已被约走");
            return false;
        }
        SetError(Error, "预约不存在");
        return false;
    }
    if (Order->BuyerID != BuyerID)
    {
        SetError(Error, "只能取消自己的预约");
        return false;
    }
    if (!ItemID.empty() && Order->ItemID != ItemID)
    {
        SetError(Error, "预约与商品不匹配");
        return false;
    }
    if (Order->Status != ReservationStatus::PendingConfirm)
    {
        SetError(Error, "已成功的预约不能取消");
        return false;
    }

    TransactionScope Unit(Tx);
    if (!Unit.Ready())
    {
        SetError(Error, "数据库事务开启失败");
        return false;
    }
    if (!Reservations.Remove(ReservationID))
    {
        SetError(Error, "预约删除失败");
        return false;
    }

    Logs.Record(LogType::CancelReservation, BuyerID, ServiceObject::Reservation,
                ReservationID, "取消预约");
    if (!Unit.Commit())
    {
        SetError(Error, "取消预约失败");
        return false;
    }
    return true;
}

std::vector<Reservation> BuyerService::ListMyReservations(const std::string& BuyerID) const
{
    return Reservations.ListByBuyer(BuyerID);
}

std::optional<Reservation> BuyerService::FindMyReservation(const std::string& BuyerID,
                                                           const std::string& ReservationID,
                                                           std::string* Error) const
{
    ClearError(Error);

    const auto Order = Reservations.FindByID(ReservationID);
    if (!Order.has_value())
    {
        SetError(Error, "预约不存在");
        return std::nullopt;
    }
    if (Order->BuyerID != BuyerID)
    {
        SetError(Error, "只能查看自己的预约");
        return std::nullopt;
    }
    return Order;
}

std::vector<TradeRecord> BuyerService::ListMyTrades(const std::string& BuyerID) const
{
    return Trades.ListByBuyer(BuyerID);
}

std::optional<TradeRecord> BuyerService::FindMyTrade(const std::string& BuyerID,
                                                     const std::string& TradeID,
                                                     std::string* Error) const
{
    ClearError(Error);

    const auto Record = Trades.FindByID(TradeID);
    if (!Record.has_value())
    {
        SetError(Error, "交易记录不存在");
        return std::nullopt;
    }
    if (!Record->IsParticipant(BuyerID))
    {
        SetError(Error, "无权查看该交易记录");
        return std::nullopt;
    }
    return Record;
}

std::optional<TradeRecord> BuyerService::FindTradeByReservation(const std::string& BuyerID,
                                                                const std::string& ReservationID,
                                                                std::string* Error) const
{
    ClearError(Error);

    const auto Record = Trades.FindByReservation(ReservationID);
    if (!Record.has_value())
    {
        SetError(Error, "交易记录不存在");
        return std::nullopt;
    }
    if (!Record->IsParticipant(BuyerID))
    {
        SetError(Error, "无权查看该交易记录");
        return std::nullopt;
    }
    return Record;
}

std::optional<SellerApplication> BuyerService::ApplyForSeller(const std::string& BuyerID,
                                                              const std::string& RealName,
                                                              std::string* Error)
{
    ClearError(Error);

    const auto Applicant = Users.FindByID(BuyerID);
    if (!Applicant.has_value())
    {
        SetError(Error, "用户不存在");
        return std::nullopt;
    }
    if (Applicant->Status == AccountStatus::Banned)
    {
        SetError(Error, ServiceRules::BannedMessage(Applicant.value()));
        return std::nullopt;
    }
    if (Applicant->UserRole == Role::Admin)
    {
        SetError(Error, "管理员不能申请卖家");
        return std::nullopt;
    }
    if (Applicant->UserRole == Role::Seller)
    {
        SetError(Error, "该用户已经是卖家");
        return std::nullopt;
    }
    if (Applications.HasApplication(BuyerID))
    {
        SetError(Error, "每位用户只能提交一次卖家认证申请");
        return std::nullopt;
    }

    auto Created = SellerApplication::Create(BuyerID, RealName, Clock());
    if (!Created.has_value())
    {
        SetError(Error, "真实姓名不能为空");
        return std::nullopt;
    }

    SellerApplication Saved = Created.value();
    if (!Applications.Save(Saved))
    {
        SetError(Error, "申请保存失败");
        return std::nullopt;
    }
    return Saved;
}

std::optional<SellerApplication> BuyerService::FindApplication(const std::string& ApplicationID) const
{
    return Applications.FindByID(ApplicationID);
}

// ----- 以下函数暂未实现 ----------------------------------------
bool BuyerService::UpdateProfile(const std::string&,
                                 const std::string&,
                                 const std::string&,
                                 std::string*)
{
    // TODO
    return false;
}
