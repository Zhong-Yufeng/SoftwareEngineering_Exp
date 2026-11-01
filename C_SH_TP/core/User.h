#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "enums.h"

// ============================================================
// User —— 用户（UML 类图：用户 <<实体>>）
//
// 类图约束：
//   REQ-001  手机号 <> 空 或 学号 <> 空；昵称、联系方式必填
//            卖家权限 = 买家权限的超集；管理员不在买卖层级内、不参与买卖
//   REQ-016  封禁账号登录时展示封禁原因 + 管理员联系方式
//
// 关联（类图中的连线在关系库里落成外键）：
//   用户 1 -- 0..* 商品          : 发布者
//   用户 1 -- 0..* 预约          : 买家
//   用户 1 -- 0..* 卖家认证申请  : 申请人 / 审核人
//   用户 1 -- 0..* 审核记录      : 审核人
//   用户 1 -- 0..* 操作日志      : 操作人
//
// 说明：
//   1. 数据成员为 public：本类当前作为数据载体，封装（改 private + 只读访问器）
//      留到状态机与不变量真正落地时再收。
//   2. 类图未给出方法参数与返回值，以下签名为最小假设。
//   3. 统一约定：会失败的方法返回 bool，true = 成功；失败原因暂由调用方记录，
//      错误码模型未定，待统一替换。
// ============================================================
class User {
public:
    // ---------------- 数据成员（类图属性） ----------------
    std::string userId;                                     // 用户ID {PK}
    std::optional<std::string> phone;                       // 手机号 {唯一, 可空}
    std::optional<std::string> studentNo;                   // 学号   {唯一, 可空}
    std::string passwordHash;                               // 密码   {密文, >=6位}
    std::string nickname;                                   // 昵称
    std::string contact;                                    // 联系方式
    AccountStatus accountStatus = AccountStatus::Normal;    // 账号状态
    std::optional<std::string> banReason;                   // 封禁原因 {可空}
    Role role = Role::Buyer;                                // 当前权限

    // ---------------- 创建入口（类图没有，实现必需） ----------------
    // 静态工厂：没有 this，不要求对象已存在，只负责装配 + 摘要密码。
    // 查重与落库由 AccountService 负责，主键由存储层分配。
    // 返回 optional：输入不合法时给 nullopt，而不是抛异常或造半成品对象。
    static std::optional<User> createNew(const std::optional<std::string>& phone,
                                         const std::optional<std::string>& studentNo,
                                         const std::string& plainPassword,
                                         const std::string& nickname,
                                         const std::string& contact);

    // ---------------- 方法（类图：用户，按图内顺序） ----------------

    // 校验明文密码是否与本对象的 passwordHash 匹配。
    // 只回答"对不对"，不对外暴露摘要值本身。
    bool verifyPassword(const std::string& plainPassword) const;

    // 是否被封禁（REQ-016）。纯判断，内联。
    bool isBanned() const { return accountStatus == AccountStatus::Banned; }

    // 修改密码。新密码长度校验在实现内做（>=6 位）。
    bool changePassword(const std::string& oldPlainPassword,
                        const std::string& newPlainPassword);

    // 修改资料（昵称、联系方式必填）。
    bool updateProfile(const std::string& newNickname,
                       const std::string& newContact);

    // 申请卖家身份。成功后 outApplicationId 写入申请ID。
    bool applyForSeller(const std::string& realName,
                        std::string& outApplicationId);

    // 查询商品（REQ-004 一类）。条件为空表示不过滤。
    // 返回商品ID列表而非对象，保持实体之间用ID关联，不互相持有指针。
    bool searchItems(const std::string& keyword,
                     std::optional<Category> category,
                     std::vector<std::string>& outItemIds);

    // 发起预约（买家）。成功后 outReservationId 写入预约ID。
    bool createReservation(const std::string& itemId,
                           std::string& outReservationId);

    // 取消预约（买家）。
    bool cancelReservation(const std::string& reservationId);

    // 建草稿（卖家）。成功后 outItemId 写入商品ID。
    bool createDraft(const std::string& title,
                     const std::string& description,
                     Category category,
                     long long priceCents,
                     Condition condition,
                     const std::string& tradeMethod,
                     const std::string& tradeLocation,
                     std::string& outItemId);

    // 提交审核（卖家，草稿/驳回 -> 待审核）。
    bool submitForAudit(const std::string& itemId);

    // 下架（卖家，已发布/已确认 -> 已下架）。
    bool takeOffShelf(const std::string& itemId);

    // 确认预约（卖家，商品 -> 已确认）。
    bool confirmReservation(const std::string& reservationId);

    // 审核商品（管理员，REQ-009）。passed=true 走通过，false 走驳回并写 reason。
    bool auditItem(const std::string& itemId,
                   bool passed,
                   const std::string& reason);

    // 违规下架（管理员，REQ-017）。
    bool banItem(const std::string& itemId,
                 const std::string& reason);

    // 封禁用户（管理员，REQ-017）。
    bool banUser(const std::string& targetUserId,
                 const std::string& reason);

    // 解封用户（管理员）。
    bool unbanUser(const std::string& targetUserId);

    // 查询日志（管理员，REQ-021）。返回日志ID列表。
    bool queryLogs(LogType type,
                   long long startMs,
                   long long endMs,
                   std::vector<std::string>& outLogIds);

    // 查看报表（管理员，REQ-024 一类）。
    // 返回「指标名 -> 数值」的成对数据，暂不引入报表专用的结构体。
    bool viewReport(std::vector<std::pair<std::string, long long>>& outRows);

private:
    // 内部工具：明文密码转摘要。类图没有这个方法，属于实现细节。
    // 声明为 static：createNew 是静态方法，需要能直接调用它。
    static std::string hashPassword(const std::string& plainPassword);
};
