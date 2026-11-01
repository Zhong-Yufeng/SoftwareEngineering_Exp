#pragma once

#include <optional>
#include <string>

#include "enums.h"

// ============================================================
// AuditRecord —— 审核记录（UML 类图：审核记录 <<实体>>）
//
// 关联：
//   商品 1 -- 0..* 审核记录 : 被审核（itemId）
//   用户 1 -- 0..* 审核记录 : 审核人（reviewerId）
//
// 类图未定义方法：审核记录由商品审核流程生成，只写一次，不对外提供行为。
// ============================================================
class AuditRecord {
public:
    // ---------------- 数据成员（类图属性） ----------------
    std::string recordId;                                // 记录ID {PK}
    std::string itemId;                                  // 被审核商品ID
    std::string reviewerId;                              // 审核人ID
    AuditResult result = AuditResult::Approved;          // 审核结果
    long long auditedAtMs = 0;                           // 审核时间
    std::optional<std::string> reason;                   // 理由 {可空}
};
