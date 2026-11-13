#pragma once
#include<optional>
#include<string>
#include<vector>
#include"ServiceCommon.h"
#include"UserStore.h"
#include"SellerApplicationStore.h"
#include"ItemStore.h"
#include"ReservationStore.h"
#include"TradeRecordStore.h"
#include"LogService.h"

// 买家用例：浏览商品、发起/取消预约、查看自己的预约与交易记录、
// 申请成为卖家。这里出现的每个方法都只服务买家身份。
class BuyerService
{
    public:
        BuyerService(UserStore& UserStoreRef,
                     SellerApplicationStore& ApplicationStore,
                     ItemStore& ItemStoreRef,
                     ReservationStore& ReservationStoreRef,
                     TradeRecordStore& TradeRecordStoreRef,
                     LogService& LogStore,
                     ServiceClock Clock = SystemTimeMs);

        // ==================== 注册与资料 ====================

        // 买家自助注册：手机号与学号至少填一个，都不能与已有账号重复
        std::optional<User> Register(const std::string& Phone,
                                     const std::string& StdNo,
                                     const std::string& Password,
                                     const std::string& Nickname,
                                     const std::string& QQNo,
                                     std::string* Error = nullptr);

        // ==================== 浏览商品 ====================

        std::optional<Item> FindItem(const std::string& BuyerID,
                                     const std::string& ItemID,
                                     std::string* Error = nullptr) const;

        std::vector<Item> QueryItems(const ItemQuery& Cond,
                                     const std::string& BuyerID) const;

        // ==================== 预约 ====================

        // 重复预约同一商品直接返回原记录，不新增（REQ-010）
        std::optional<Reservation> Reserve(const std::string& BuyerID,
                                           const std::string& ItemID,
                                           std::string* Error = nullptr);

        // ItemID 可为空：旧页面只带 ReservationID 时也能走通
        bool CancelReservation(const std::string& BuyerID,
                               const std::string& ItemID,
                               const std::string& ReservationID,
                               std::string* Error = nullptr);

        std::vector<Reservation> ListMyReservations(const std::string& BuyerID) const;

        std::optional<Reservation> FindMyReservation(const std::string& BuyerID,
                                                     const std::string& ReservationID,
                                                     std::string* Error = nullptr) const;

        // ==================== 卖家认证申请 ====================

        // 买家自助申请成为卖家，一人一条；审核动作在 AdminService
        std::optional<SellerApplication> ApplyForSeller(const std::string& BuyerID,
                                                        const std::string& RealName,
                                                        std::string* Error = nullptr);

        // 申请人查看自己这次申请的结果
        std::optional<SellerApplication> FindApplication(const std::string& ApplicationID) const;

        // ==================== 交易记录 ====================

        // 预约记录会随商品下架、卖家封禁被删除，交易记录永不删除，买家据此查询「我约成过什么」
        std::vector<TradeRecord> ListMyTrades(const std::string& BuyerID) const;

        std::optional<TradeRecord> FindMyTrade(const std::string& BuyerID,
                                               const std::string& TradeID,
                                               std::string* Error = nullptr) const;

        std::optional<TradeRecord> FindTradeByReservation(const std::string& BuyerID,
                                                          const std::string& ReservationID,
                                                          std::string* Error = nullptr) const;

        // ----- 以下函数暂未实现 ----------------------------------------
        bool UpdateProfile(const std::string& UserID,
                           const std::string& Nickname,
                           const std::string& QQNo,
                           std::string* Error = nullptr);

    private:
        UserStore& Users;
        SellerApplicationStore& Applications;
        ItemStore& Items;
        ReservationStore& Reservations;
        TradeRecordStore& Trades;
        LogService& Logs;
        ServiceClock Clock;
};
