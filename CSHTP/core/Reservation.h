#pragma once
#include<optional>
#include<string>
#include"enums.h"

// ============================================================
// Reservation —— 预约（本质是「一条待卖家确认的购买意向」）
//
// 【为什么没有状态字段】
//   旧模型里预约有「待确认 / 已成功」两个状态，还有 ConfirmedAtMs。
//   但成交的规则是：卖家确认谁，商品就成交，商品记录与全部预约记录一起删除。
//   于是「库里存在的预约」必然还在等确认——状态字段永远只有一个值，
//   留着它只会让读代码的人以为存在别的分支。所以 ReservationStatus
//   与 Confirm()、ConfirmedAtMs 一并删除。
//
// 职责划分（与 Item 一致）：
//   Reservation::Create   校验入参 + 装配一条记录，不产生 ID
//   存储层                分配 ReservationID、按买家/商品查询、删除记录
//
// 所有会改动预约的动作都不在 Reservation 里，因为它们都要同时改动别的对象：
//   买家取消预约   → 删本记录 + 写日志
//   卖家确认成交   → 删本记录 + 删同商品的其余预约 + 删商品 + 建 TradeRecord + 写日志
//   商品被删除     → 删该商品的全部预约 + 写日志
// 这些编排放服务层，预约对象只负责「我这条记录本身合法」。
//
// 跨对象的约束（单条记录看不到别的记录，所以都不在这里判）：
//   1. 同一买家对同一商品最多一条预约（是「已存在就不重复插入」的问题）
//   2. 一件商品的预约数不超过该商品的 MaxReservations
//   3. 每个买家同时最多 20 条预约
// ============================================================
class Reservation
{
    public:
        // 预约类的变量
        std::string ReservationID;   // 由存储层分配
        std::string BuyerID;
        std::string ItemID;
        long long CreatedAtMs;       // 发起时间，由调用方传入，便于测试断言
        // 预约类的变量

        // 预约类的构造函数
        // 校验：买家与商品非空、发起时间为正值。不合法返回 std::nullopt。
        static std::optional<Reservation> Create(const std::string& BuyerID,
                                                 const std::string& ItemID,
                                                 long long CreatedAtMs);
        // 预约类的构造函数
};
