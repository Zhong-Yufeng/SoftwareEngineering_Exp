#pragma once
#include<optional>
#include<vector>
#include<string>
#include"enums.h"

// ============================================================
// Item —— 商品
//
// 创建分两半：
//   Item::CreateDraft  只挡「必然错误」，装配出一个草稿对象
//   ItemStore::Save    负责分配 ItemID 并入库
// 所以 CreateDraft 不产生 ID；草稿阶段描述、图片、交易方式允许留空。
//
// 【状态模型】4 个状态、5 条边，见 enums.h 的 ItemStatus 注释。
// 商品类只负责回答一个问题：「我能不能从状态 A 走到状态 B」。
// 它不负责回答「谁能看见我」——那需要知道浏览者是谁，而 Item 里没有、
// 也不该有「浏览者」这个字段。可见性由文件末尾的自由函数 CanSeeItem 承担。
//
// 【成交】没有「交易成功」状态。卖家确认某条预约时，商品记录被从库里删除，
// 与之相关的预约一并删除，同时生成一条 TradeRecord 记录成交事实。
// 所以「成交」不是一个状态，而是一个动作的结果；任何还存在于库里的商品，
// 都还没有成交。这条不变式让「查询在售商品」不需要排除任何状态。
// ============================================================
class Item
{
    public:
        // 商品类的变量
        std::string ItemID;                 // 草稿阶段为空，由存储层分配
        std::string Title;
        std::string Description;
        Category ItemCategory;
        long long PriceCents;               // 价格，单位分
        Condition ItemCondition;
        std::vector<std::string> Images;    // 图片，提交审核时才要求至少一张
        std::string TradeMethod;            // 交易方式，如「校内面交」
        std::string TradeLocation;          // 交易地点
        std::string Contact;                // 卖家联系方式，成交后展示给买家
        ItemStatus Status;                  // 创建时固定为 Draft
        long long PublishedAtMs;            // 审核通过时才写，此前为 0
        std::optional<std::string> RejectReason;  // 驳回理由，仅 Rejected 时有值
        int MaxReservations;                // 本商品最多接受几条预约，1~10，默认 10
        std::string SellerID;
        // 商品类的变量

        // 商品类的构造函数
        // 校验：标题非空、卖家非空、价格不为负；其余允许留空（草稿）。
        // 不合法返回 std::nullopt。
        static std::optional<Item> CreateDraft(const std::string& Title,
                                               const std::string& Description,
                                               Category ItemCategory,
                                               long long PriceCents,
                                               Condition ItemCondition,
                                               const std::string& TradeMethod,
                                               const std::string& TradeLocation,
                                               const std::string& Contact,
                                               const std::string& SellerID);
        // 商品类的构造函数

        // ------------------------------------------------------------
        // 状态迁移方法（4 个）
        //
        // 统一口径：
        //   1. 只做「本对象能不能迁」的判断，不查库、不碰别的类；
        //      「卖家配额 50 件」「买家配额 20 条预约」「预约数不超过
        //      MaxReservations」都靠数库里的行数，属于服务层，不在这里。
        //   2. 失败不改动任何字段（要么完整迁移，要么原样不动），
        //      这样调用方拿到 false 时不必回滚。
        //   3. 返回 false 不说明原因，原因由服务层结合自身上下文给出提示语。
        // ------------------------------------------------------------

        // 提交审核：Draft → PendingReview，Rejected → PendingReview
        // 前置：Status 为 Draft 或 Rejected；
        //       标题、描述、交易方式、交易地点、联系方式均非空；
        //       Images 至少一张；MaxReservations 在 1~10 之间。
        // 副作用：Status 置 PendingReview，并清空 RejectReason。
        // 【为什么必须清空】驳回理由的不变式是
        //      (Status == Rejected) ⇔ RejectReason 有值。
        //   重提后若不清空，就会出现「待审核的商品挂着上一次的驳回理由」，
        //   违反不变式，之后 Approve 也会把这条陈旧理由带进已发布状态。
        bool SubmitForReview();

        // 审核通过：PendingReview → Published
        // 前置：Status 为 PendingReview。
        // 副作用：Status 置 Published，PublishedAtMs 写为 NowMs。
        // 时间由外部传入而不是在内部取 clock，是为了让测试能断言具体数值。
        bool Approve(long long NowMs);

        // 审核驳回：PendingReview → Rejected
        // 前置：Status 为 PendingReview，且 Reason 非空字符串。
        // 副作用：Status 置 Rejected，RejectReason 置为 Reason。
        // 【为什么理由必须非空】需求规定驳回时必须填理由；空串和「没填」在
        //   业务上没有区别，所以在这里直接把空串挡掉，不留给界面去校验。
        bool Reject(const std::string& Reason);

        // 是否允许卖家编辑内容（改标题、价格、图片、MaxReservations 等）
        // 只有 Draft 与 Rejected 可编辑：PendingReview 期间内容冻结（否则管理员
        // 审的和卖家改的就不是同一份东西），Published 之后改内容会误导买家。
        bool CanEdit() const;
};

// ============================================================
// 可见性规则
//
// 为什么不写成 Item 的成员函数：可见性的判定条件是「谁在看」，
// 而 Item 只知道「我是谁的商品」。把浏览者塞进 Item 的形参表，
// 等于让商品类去解释角色权限，职责错位；而且商品与预约的可见性
// 规则并不相同，集中成一个自由函数更容易一眼对照。
//
// 规则表（✔ 可见，✘ 不可见）：
//
//   状态            该卖家   其他卖家   买家    管理员
//   草稿              ✔        ✘        ✘       ✘
//   待审核            ✔        ✘        ✘       ✔
//   驳回              ✔        ✘        ✘       ✘
//   已发布            ✔        ✔        ✔       ✘
//
// 实现要点：
//   「卖家」在 Role 上是 Buyer 的超集，所以已发布状态不必区分买家与卖家，
//   只需排除管理员；而 Draft/Rejected 必须靠 ViewerID == SellerID 判定，
//   光看角色是分不出「这是哪个卖家」的。
// ============================================================
bool CanSeeItem(const Item& Goods, Role ViewerRole, const std::string& ViewerID);
