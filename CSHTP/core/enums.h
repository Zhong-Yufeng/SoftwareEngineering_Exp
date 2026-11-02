#pragma once

// ============================================================
// 校园二手交易平台 —— 领域枚举
// 对应 UML 类图「枚举」区块，共 9 个
//
// 命名约定：类名 / 枚举名 / 枚举值用 ASCII 英文，注释标中文原名。
// 原因：C++ 标准允许 Unicode 标识符，但 MSVC 与 GCC 处理源码编码的方式不同，
//       构建脚本、调试器、外部工具对中文字符的支持也不一致。
//       工程上统一用 ASCII 标识符 + 中文注释，可避免换编译器后突然编不过。
//
// 使用 enum class（强类型枚举）而非 enum：
//   1. 枚举值不会泄漏到外层作用域，AccountStatus::Banned 与 ItemStatus::Banned 不冲突
//   2. 不会隐式转换成 int，避免拿整数和状态互相赋值
// ============================================================

// 账号状态（类图：账号状态）
enum class AccountStatus {
    Normal,   // 正常
    Banned    // 已封禁
};

// 当前权限（类图：当前权限）
// 约束：卖家权限 = 买家权限的超集；管理员不在买卖层级内，不参与买卖
enum class Role {
    Buyer,    // 买家
    Seller,   // 卖家
    Admin     // 管理员
};

// 商品状态（类图：商品状态）
enum class ItemStatus {
    Draft,           // 草稿
    PendingReview,   // 待审核
    Rejected,        // 驳回
    Published,       // 已发布
    Confirmed,       // 已确认
    OffShelf,        // 已下架
    Banned,          // 违规下架
    Traded           // 交易完成
};

// 预约状态（类图：预约状态）
enum class ReservationStatus {
    PendingConfirm,  // 待确认
    Succeeded        // 已成功
};

// 认证审核状态（类图：认证审核状态）
enum class ApplicationStatus {
    PendingReview,   // 待审核
    Approved,        // 通过
    Rejected         // 驳回
};

// 审核结果（类图：审核结果）
enum class AuditResult {
    Approved,        // 通过
    Rejected         // 驳回
};

// 商品分类（类图：商品分类）
enum class Category {
    Textbook,        // 教材
    Digital,         // 数码
    Clothing,        // 服饰
    DailyGoods       // 生活用品
};

// 成色（类图：成色）
enum class Condition {
    BrandNew,        // 全新
    LikeNew,         // 九成新
    Flawed           // 有瑕疵
};

// 日志类型（类图：日志类型）
enum class LogType {
    Register,             // 注册
    SubmitAudit,          // 提交审核
    Audit,                // 审核
    OffShelf,             // 下架
    BanDisposal,          // 违规处置
    BanUser,              // 封禁
    UnbanUser,            // 解封
    CreateReservation,    // 发起预约
    CancelReservation,    // 取消预约
    ConfirmReservation,   // 确认预约
    DeleteReservation     // 预约记录删除
};
