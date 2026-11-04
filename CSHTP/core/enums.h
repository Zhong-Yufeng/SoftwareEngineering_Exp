#pragma once

// ============================================================
// 校园二手交易平台 —— 领域枚举
//
// 命名约定：类名 / 枚举名 / 枚举值用 ASCII 英文，注释标中文原名。
// 原因：C++ 标准允许 Unicode 标识符，但 MSVC 与 GCC 处理源码编码的方式不同，
//       构建脚本、调试器、外部工具对中文字符的支持也不一致。
//       工程上统一用 ASCII 标识符 + 中文注释，可避免换编译器后突然编不过。
//
// 使用 enum class（强类型枚举）而非 enum：
//   1. 枚举值不会泄漏到外层作用域，AccountStatus::Banned 与 ItemStatus::Banned 不冲突
//   2. 不会隐式转换成 int，避免拿整数和状态互相赋值
//
// 【2026-11-03 调整】商品状态由 8 个减为 4 个，删除 ReservationStatus。
//   理由见 Item.h / Reservation.h 顶部注释。实验二类图与 REQ-018 需同步修改。
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
//
// ============================ 状态迁移 ============================
// 4 个状态、5 条状态间的边，另有 1 条删除出口。
// 注意「驳回」是独立状态，不是草稿的一种：它只能走向「待审核」（重新提交）
// 或者被删除，永远不经过草稿。
//
//   ┌────────────┐  ① 提交审核  ┌─────────────────┐  ③ 通过  ┌─────────────┐
//   │   Draft    │ ───────────► │  PendingReview  │ ──────► │  Published  │
//   │   草稿      │ ◄─────────── │     待审核       │         │   已发布     │
//   └────────────┘  ② 撤回       └─────────────────┘         └─────────────┘
//                                        │  ▲
//                          ④ 驳回         │  │ ⑤ 重新提交
//                                        ▼  │
//                                 ┌─────────────────┐
//                                 │    Rejected     │
//                                 │     驳回         │
//                                 └─────────────────┘
//
// 每条边的触发者与限制：
//   ① Draft      ──提交审核──► PendingReview  该卖家，内容须齐全
//   ② PendingReview ──撤回──► Draft          该卖家，但被管理员占用时禁止
//   ③ PendingReview ──通过──► Published       管理员
//   ④ PendingReview ──驳回──► Rejected        管理员，必须填理由
//   ⑤ Rejected   ──重新提交──► PendingReview  该卖家，内容可先修改
//
// ②与③④的冲突靠「审核占用锁」（Item::ReviewLockedByID）解决：
// 管理员开始审核即上锁，卖家此时不能撤回；管理员下结论（通过/驳回）或
// 放弃审核后解锁，卖家又能撤回。所以 ② 与 ③④ 永不并发生效。
//
// ============================ 商品的归宿 ============================
// 任何一件商品，最终都会从库里消失——「删除」不是第五个状态，
// 而是状态机的出口，走完就查无此物，只留一条日志。
//
//   Draft         ──卖家删除──────► ⊗
//   Rejected      ──卖家删除──────► ⊗
//   PendingReview ──不能直接删（要先撤回成草稿再删，避免删掉管理员正在审的东西）
//   Published     ──卖家确认预约──► ⊗  另生成一条 TradeRecord（成交凭证）
//
// 由此得到两条贯穿全局的不变式：
//   1. 库里存在的商品，一定还没成交（成交即删除，所以「在售」不必排除任何状态）
//   2. 商品的删除是不可逆的，删除后只能靠 OperationLog / TradeRecord 追溯
//
// 已删除的四个值及其去向：
//   Confirmed（已确认）→ 与「交易完成」合并，成交即删除记录，不再需要中间状态
//   OffShelf（已下架） → 卖家改为「删除」草稿/驳回商品，不保留下架态
//   Banned（违规下架） → 管理员只负责审核；封禁账号通过查询过滤表达，不加状态
//   Traded（交易完成） → 成交后商品记录被删除，成交事实由 TradeRecord 承载
enum class ItemStatus {
    Draft,           // 草稿：仅该卖家可见，可编辑、可提交审核、可删除
    PendingReview,   // 待审核：该卖家与全体管理员可见，内容冻结
    Rejected,        // 驳回：仅该卖家可见，可编辑后重新提交
    Published        // 已发布：对买家与其他卖家可见，管理员不可见
};

// 【已删除】ReservationStatus / 预约状态
// 原值 PendingConfirm、Succeeded。
// 删除理由：确认成交后预约记录直接从库里删除（卖家确认谁，就是与谁成交），
//           所以「库里存在的预约」必然处于待确认，一个只有一个值的状态枚举没有信息量。
//           预约的取消同样是删除记录，不再需要 Canceled 之类的状态。

// 认证审核状态（类图：认证审核状态）
// 用于卖家申请（SellerApplication），不复用于商品审核
enum class ApplicationStatus {
    PendingReview,   // 待审核
    Approved,        // 通过
    Rejected         // 驳回
};

// 审核结果（类图：审核结果）
// 用于商品审核记录（AuditRecord）
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
//
// 设计口径：一个动作一种类型，日志的价值在于「能按动作筛选」，
//           所以商品审核与卖家申请审核要分开，否则查「谁把谁封了」时
//           会和「谁审核通过了哪件商品」混在一起。
//
// 已删除：OffShelf（下架）、BanDisposal（违规处置）—— 对应状态已删除；
//         DeleteReservation（预约记录删除）—— 与 CancelReservation 语义重复。
enum class LogType {
    Register,                  // 注册
    SubmitItemAudit,           // 提交商品审核
    AuditItem,                 // 审核商品（通过与驳回都记这一条，结果看描述）
    WithdrawItemAudit,         // 撤回审核（卖家在待审核期间收回）
    DeleteItem,                // 删除商品（草稿/驳回），描述里写商品标题
    CreateReservation,         // 发起预约
    CancelReservation,         // 买家主动取消预约
    InvalidateReservation,     // 预约被作废（成交时其余预约作废 / 商品被删时全部作废）
    ConfirmReservation,        // 卖家确认预约，即成交
    SubmitSellerApplication,   // 提交卖家申请
    AuditSellerApplication,    // 审核卖家申请
    BanUser,                   // 封禁用户
    UnbanUser                  // 解封用户
};
