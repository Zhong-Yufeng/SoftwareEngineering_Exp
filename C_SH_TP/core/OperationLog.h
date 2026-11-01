#pragma once

#include <optional>
#include <string>
#include <vector>

#include "enums.h"

// ============================================================
// OperationLog —— 操作日志（UML 类图：操作日志 <<实体>>）
//
// 关联：
//   用户 1 -- 0..* 操作日志 : 操作人（operatorId）
//
// 类图说明：
//   对象类型 / 对象ID 构成多态关联，可指向 用户 / 商品 / 预约 / 卖家认证申请。
//   日志不可修改（REQ-021）。
//
// 多态关联的做法：不在本类里持有指向各实体的指针，而是存两段字符串
//   objectType（"user" / "item" / "reservation" / "seller_application"）
//   objectId  （对应实体的主键）
// 查询时按 objectType 决定去查哪张表。
// ============================================================
class OperationLog {
public:
    // ---------------- 数据成员（类图属性） ----------------
    std::string logId;                           // 日志ID {PK}
    std::string operatorId;                      // 操作人ID
    long long operatedAtMs = 0;                  // 操作时间
    LogType type = LogType::Register;            // 操作类型
    std::string objectType;                      // 对象类型
    std::string objectId;                        // 对象ID
    std::optional<std::string> description;      // 说明 {可空}

    // ---------------- 方法（类图：操作日志） ----------------

    // 查询：按类型与时间区间过滤，返回日志ID列表。
    // 类图只写了「查询()」，这里是可执行的最小签名。
    bool query(LogType type,
               long long startMs,
               long long endMs,
               std::vector<std::string>& outLogIds);
};
