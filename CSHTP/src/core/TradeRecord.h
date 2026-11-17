#pragma once
#include<optional>
#include<string>
#include"Item.h"
#include"Reservation.h"
#include"enums.h"

class SqliteTradeRecordStore;

// 交易记录：卖家确认预约那一刻生成的凭证，买家「约成了什么」靠它回答。
// 商品与预约随后可能被下架、删除，所以标题、价格、联系方式都存值拷贝。
// 记录只增不删，生命周期靠 Status 推进：进行中 → 已完成 / 已作废。
class TradeRecord
{
    public:
        std::string TradeID;            // 由存储层分配
        std::string ReservationID;      // 由哪条预约产生
        std::string ItemID;             // 指向商品，仅用于追溯
        std::string BuyerID;
        std::string SellerID;
        TradeStatus Status = TradeStatus::InProgress;

        long long CreatedAtMs = 0;      // 卖家确认、记录生成的时间
        long long CompletedAtMs = 0;    // 0 表示尚未完成
        long long CancelledAtMs = 0;    // 0 表示未作废
        std::string CancelReason;       // 作废原因，仅已作废时非空

        std::string ItemTitle;          // 确认时的商品标题
        long long PriceCents = 0;       // 确认时的价格，单位分
        std::string TradeLocation;      // 交易地点
        std::string SellerContact;      // 卖家联系方式
        std::string BuyerContact;       // 买家联系方式

        // 卖家确认预约成功后调用：预约必须已成功，商品必须已确认
        static std::optional<TradeRecord> Create(const Reservation& Order,
                                                 const Item& Goods,
                                                 const std::string& BuyerContact,
                                                 long long CreatedAtMs);

        // 进行中 → 已完成
        bool Complete(long long TimeMs);

        // 进行中 → 已作废；已完成不可作废
        bool Cancel(const std::string& Reason, long long TimeMs);

        // 是否与某人有关（买家或卖家），用于「我买了/我卖了」的过滤
        bool IsParticipant(const std::string& UserID) const;

    private:
        friend class SqliteTradeRecordStore;
        TradeRecord() = default;
};
