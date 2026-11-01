#pragma once

#include <optional>
#include <string>

#include "enums.h"

// ============================================================
// Reservation —— 预约（UML 类图：预约 <<实体>>）
//
// 关联：
//   用户 1 -- 0..* 预约 : 买家（buyerId）
//   商品 1 -- 0..* 预约 : 被预约（itemId）
//
// 类图约束（OCL）：
//   同一商品中 状态=已成功 的预约数量 <= 1（REQ-019 / REQ-023）
//   失效 = 删除记录，不保留状态值
// ============================================================
class Reservation {
public:
    // ---------------- 数据成员（类图属性） ----------------
    std::string reservationId;                                        // 预约ID {PK}
    std::string buyerId;                                              // 买家ID
    std::string itemId;                                               // 被预约商品ID
    long long createdAtMs = 0;                                        // 发起时间
    ReservationStatus status = ReservationStatus::PendingConfirm;      // 状态
    std::optional<long long> confirmedAtMs;                            // 确认时间 {可空}

    // ---------------- 方法（类图：预约，按图内顺序） ----------------

    // 发起预约。实现内需校验：商品可预约、该商品尚无已成功预约。
    bool create();

    // 确认预约：待确认 -> 已成功，并写 confirmedAtMs。
    bool confirm();

    // 删除预约记录（取消或失效），不保留状态值。
    bool remove();
};
