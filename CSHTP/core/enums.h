#pragma once

// 领域枚举。标识符用 ASCII 英文，注释标中文原名；enum class 防止跨枚举混用。

// 账号状态
enum class AccountStatus {
    Normal,   // 正常
    Banned    // 已封禁
};

// 当前权限
enum class Role {
    Buyer,    // 买家
    Seller,   // 卖家
    Admin     // 管理员
};

// 商品状态（REQ-018）
// 允许的转换：草稿→待审核、待审核→已发布/驳回、驳回→待审核、
// 已发布→已确认/已下架/违规下架、已确认→交易完成/已下架/违规下架。
enum class ItemStatus {
    Draft,              // 草稿
    PendingReview,      // 待审核
    Rejected,           // 驳回
    Published,          // 已发布
    Confirmed,          // 已确认（交易中）
    OffShelf,           // 已下架
    ViolationOffShelf,  // 违规下架
    Traded              // 交易完成
};

// 预约状态（REQ-019）
// 只有待确认与已成功两种存续状态；失效的预约直接删除记录。
enum class ReservationStatus {
    PendingConfirm,   // 待确认
    Succeeded         // 已成功
};

// 交易记录状态
// 卖家确认预约时生成（进行中）；交易完成变已完成；商品下架、违规下架或卖家被封禁
// 导致交易无法继续时变已作废。已完成不可再作废，记录本身只增不删。
enum class TradeStatus {
    InProgress,   // 进行中
    Completed,    // 已完成
    Cancelled     // 已作废
};

// 认证审核状态
enum class ApplicationStatus {
    PendingReview,   // 待审核
    Approved,        // 通过
    Rejected         // 驳回
};

// 审核结果
enum class AuditResult {
    Approved,        // 通过
    Rejected         // 驳回
};

// 商品分类
enum class Category {
    Textbook,        // 教材
    Digital,         // 数码
    Clothing,        // 服饰
    DailyGoods       // 生活用品
};

// 成色
enum class Condition {
    BrandNew,        // 全新
    LikeNew,         // 九成新
    Flawed           // 有瑕疵
};

// 日志类型（REQ-021）
enum class LogType {
    Register,             // 注册
    SubmitReview,         // 提交审核
    Audit,                // 审核
    OffShelf,             // 下架
    ViolationHandling,    // 违规处置
    Ban,                  // 封禁
    Unban,                // 解封
    CreateReservation,    // 发起预约
    CancelReservation,    // 取消预约
    ConfirmReservation,   // 确认预约
    DeleteReservation,    // 预约记录删除
    CompleteTrade         // 完成交易
};
