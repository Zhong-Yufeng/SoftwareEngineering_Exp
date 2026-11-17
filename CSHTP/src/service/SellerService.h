#pragma once
#include<cstddef>
#include<optional>
#include<string>
#include<vector>
#include"ServiceCommon.h"
#include"UserStore.h"
#include"ItemStore.h"
#include"ReservationStore.h"
#include"TradeRecordStore.h"
#include"LogService.h"

// REQ-022 卖家汇总：名下在售商品数（已发布 + 已确认）与已完成交易数
struct SellerStats
{
    std::size_t OnSaleCount = 0;
    std::size_t CompletedCount = 0;
};

// 卖家用例：发布与维护商品、确认预约、完成交易、查看自己的商品与交易记录。
class SellerService
{
    public:
        SellerService(UserStore& UserStoreRef,
                      ItemStore& ItemStoreRef,
                      ReservationStore& ReservationStoreRef,
                      TradeRecordStore& TradeRecordStoreRef,
                      LogService& LogStore,
                      Transaction& TransactionRef,
                      ServiceClock Clock = SystemTimeMs);

        // ==================== 商品管理 ====================

        std::optional<Item> CreateDraft(const std::string& SellerID,
                                        const ItemInfo& Info,
                                        std::string* Error = nullptr);

        bool SubmitForReview(const std::string& SellerID,
                             const std::string& ItemID,
                             std::string* Error = nullptr);

        std::optional<Item> FindItem(const std::string& SellerID,
                                     const std::string& ItemID,
                                     std::string* Error = nullptr) const;

        std::vector<Item> QueryItems(const ItemQuery& Cond,
                                     const std::string& SellerID) const;

        std::vector<Item> ListMyItems(const std::string& SellerID) const;

        // ==================== 交易 ====================

        // 确认一条预约：商品转「已确认」、其余待确认记录删除、生成进行中的交易记录
        bool ConfirmReservation(const std::string& SellerID,
                                const std::string& ItemID,
                                const std::string& ReservationID,
                                std::string* Error = nullptr);

        bool CompleteTrade(const std::string& SellerID,
                           const std::string& ItemID,
                           std::string* Error = nullptr);

        std::vector<Reservation> ListItemReservations(const std::string& SellerID,
                                                      const std::string& ItemID,
                                                      std::string* Error = nullptr) const;

        std::vector<TradeRecord> ListItemTrades(const std::string& SellerID,
                                                const std::string& ItemID,
                                                std::string* Error = nullptr) const;

        std::vector<TradeRecord> ListMyTrades(const std::string& SellerID) const;

        // ==================== 汇总统计 ====================

        SellerStats GetSellerStats(const std::string& SellerID) const;

        // ----- 以下函数暂未实现 ----------------------------------------
        bool ModifyItem(const std::string& SellerID,
                        const std::string& ItemID,
                        const ItemInfo& Info,
                        std::string* Error = nullptr);

        bool OffShelfItem(const std::string& SellerID,
                          const std::string& ItemID,
                          std::string* Error = nullptr);

    private:
        UserStore& Users;
        ItemStore& Items;
        ReservationStore& Reservations;
        TradeRecordStore& Trades;
        LogService& Logs;
        Transaction& Tx;
        ServiceClock Clock;
};
