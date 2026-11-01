#pragma once

#include <optional>
#include <string>

#include "enums.h"

// ============================================================
// SellerApplication —— 卖家认证申请（UML 类图：卖家认证申请 <<实体>>）
//
// 关联：
//   用户 1 -- 0..* 卖家认证申请 : 申请人（applicantId）
//   用户 1 -- 0..* 卖家认证申请 : 审核人（reviewerId）
// ============================================================
class SellerApplication {
public:
    // ---------------- 数据成员（类图属性） ----------------
    std::string applicationId;                                              // 申请ID {PK}
    std::string applicantId;                                                // 申请人ID
    std::string realName;                                                   // 姓名
    long long submittedAtMs = 0;                                            // 提交时间
    ApplicationStatus auditStatus = ApplicationStatus::PendingReview;       // 审核状态
    std::optional<long long> auditedAtMs;                                   // 审核时间 {可空}
    std::optional<std::string> rejectReason;                                // 驳回理由 {可空}
    std::optional<std::string> reviewerId;                                  // 审核人ID（由关联导出）

    // ---------------- 方法（类图：卖家认证申请，按图内顺序） ----------------

    // 提交申请：校验该用户无待审核申请，写 submittedAtMs。
    bool submit();

    // 审核：待审核 -> 通过 / 驳回。驳回时必须写 reason。
    // 通过后由上层把该用户 role 提升为 Seller。
    bool audit(bool passed, const std::string& reason);
};
