#pragma once
#include<optional>
#include<string>
#include"enums.h"

// ============================================================
// Reservation —— 预约
//
// 职责划分（与 Item 一致）：
//   Reservation::Create   校验入参 + 装配一条「待确认」记录，不产生 ID
//   Reservation::Confirm  状态迁移：待确认 -> 已成功，写确认时间
//   存储层                分配 ReservationID、按条件查询、删除记录
//
// 「失效 = 删除记录」由存储层的 Remove 完成，对象上不保留作废状态。
// 跨对象约束（同一商品最多一条已成功预约）单条记录看不到别的记录，
// 由存储层查询 + 上层编排保证。
// ============================================================
class Reservation
{
    public:
        // 预约类的变量
        std::string ReservationID;               // 由存储层分配
        std::string BuyerID;
        std::string ItemID;
        long long CreatedAtMs;                   // 发起时间
        ReservationStatus Status;                // 创建时固定为 PendingConfirm
        std::optional<long long> ConfirmedAtMs;  // 确认时间，未确认为空
        // 预约类的变量

        // 预约类的构造函数
        // 校验：买家与商品非空、发起时间为正值。不合法返回 std::nullopt。
        static std::optional<Reservation> Create(const std::string& BuyerID,
                                                 const std::string& ItemID,
                                                 long long CreatedAtMs);
        // 预约类的构造函数

        // 预约类的方法
        // 待确认 -> 已成功，并写入确认时间。
        // 只有 PendingConfirm 能确认，重复确认返回 false。
        bool Confirm(long long NowMs);
        // 预约类的方法
};
