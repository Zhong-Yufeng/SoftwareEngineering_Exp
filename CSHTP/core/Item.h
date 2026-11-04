#pragma once
#include<optional>
#include<vector>
#include<string>
#include"enums.h"

// ============================================================
// Item —— 商品
//
// 创建分两半：
//   Item::CreateDraft  校验内容齐全，装配出一个草稿对象
//   ItemStore::Save    负责分配 ItemID 并入库
// 所以 CreateDraft 不产生 ID。
//
// 【「草稿」的含义】创建时内容就必须齐全，所以草稿不再是「内容不全的
// 半成品」，而是「内容齐全、但还没提交审核」的商品。草稿与已发布之间
// 差的不是完整度，而是管理员那一次审核。
//
// 【状态模型】4 个状态、5 条状态间的边 + 1 条删除出口，见 enums.h 的 ItemStatus 注释。
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
        std::vector<std::string> Images;    // 图片，创建时即要求至少一张
        // 没有「交易方式」字段：平台只支持线下当面交易，不提供邮寄或担保。
        // 它是一个常量而不是每件商品的属性，所以不该占一个字段。
        std::string TradeLocation;          // 交易地点
        std::string Contact;                // 卖家联系方式，仅卖家本人与
                                            // 成交后的买家可见
        ItemStatus Status;                  // 创建时固定为 Draft
        long long PublishedAtMs;            // 审核通过时才写，此前为 0
        std::optional<std::string> RejectReason;  // 驳回理由，仅 Rejected 时有值
        // 审核占用锁：有值 = 正在被该管理员审核，nullopt = 没人审。
        // 只有 PendingReview 期间才可能被锁；用 optional 而不是 bool，
        // 因为「谁锁的」在排查「商品为什么撤不回」时是关键信息。
        std::optional<std::string> ReviewLockedByID;
        int MaxReservations;                // 本商品最多接受几条预约，1~10，默认 10
        std::string SellerID;
        // 商品类的变量

        // 商品类的构造函数
        // 创建即要求内容齐全（草稿只是「未提交审核」，不是「内容不全」）：
        //   标题、描述、交易地点、联系方式非空；
        //   分类与成色必须是枚举里的合法值（enum class 能被强转成非法数字）；
        //   价格必须大于 0（单位分，整数，没有精度问题）；
        //   图片至少一张，且路径不能是空串（防止用空串凑数）；
        //   预约上限在 1~10 之间；
        //   卖家编号非空。
        // 以下字段不由调用方给出，在函数内固定：
        //   ItemID 留空（存储层分配）、Status=Draft、PublishedAtMs=0、
        //   RejectReason 与 ReviewLockedByID 为空。
        // 不合法返回 std::nullopt。
        static std::optional<Item> CreateDraft(const std::string& Title,
                                               const std::string& Description,
                                               Category ItemCategory,
                                               long long PriceCents,
                                               Condition ItemCondition,
                                               const std::vector<std::string>& Images,
                                               const std::string& TradeLocation,
                                               const std::string& Contact,
                                               int MaxReservations,
                                               const std::string& SellerID);
        // 商品类的构造函数

        // ------------------------------------------------------------
        // 状态迁移方法（6 个）
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
        //       标题、描述、交易地点、联系方式均非空；
        //       Images 至少一张；MaxReservations 在 1~10 之间。
        // 【为什么创建时已经校验过，这里还要再查一遍】字段是 public 的，
        //   创建之后卖家在草稿/驳回阶段仍可能把图片清空、把标题改空。
        //   创建函数管的是「第一次写下时是对的」，这里管的是「提交的那一刻
        //   是对的」——它才是内容进入审核前的最后一道闸门。
        // 副作用：Status 置 PendingReview，并清空 RejectReason 与审核占用锁。
        // 【为什么必须清空理由】驳回理由的不变式是
        //      (Status == Rejected) ⇔ RejectReason 有值。
        //   重提后若不清空，就会出现「待审核的商品挂着上一次的驳回理由」，
        //   违反不变式，之后 Approve 也会把这条陈旧理由带进已发布状态。
        bool SubmitForReview();

        // 撤回审核：PendingReview → Draft
        // 前置：Status 为 PendingReview，且当前没有管理员占用审核
        //       （ReviewLockedByID 为空）。
        // 副作用：Status 置 Draft，清空 RejectReason 与审核占用锁。
        // 【为什么撤回的目标是草稿而不是删除】撤回的意图是「我还要改」，
        //   草稿正好可编辑、也可再删；若撤回直接删除，卖家想改一个字就得重写。
        // 【为什么被锁就不能撤】管理员正在看这份内容并准备下结论，此时内容
        //   若回退成草稿并允许编辑，管理员批的就不是同一份东西了。锁定是
        //   「审核期间内容冻结」这条规则的具体实现。
        bool WithdrawReview();

        // 管理员开始审核：给商品上锁
        // 前置：Status 为 PendingReview，且当前未被占用；ReviewerID 非空。
        // 副作用：ReviewLockedByID 置为 ReviewerID。
        // 返回值 false 表示「已被别的管理员占用」，调用方据此提示「他人审核中」。
        bool LockForReview(const std::string& ReviewerID);

        // 管理员结束审核（放弃/取消审核，未下结论）时释放锁
        // 前置：ReviewLockedByID 有值。
        // 副作用：ReviewLockedByID 清空。
        // 【注意】通过/驳回/撤回会自动释放锁，这三个方法内部不必再调它；
        //   它是为「打开审核界面后中途关掉」这种没下结论的情况准备的。
        bool ReleaseReview();

        // 审核通过：PendingReview → Published
        // 前置：Status 为 PendingReview。
        // 副作用：Status 置 Published，PublishedAtMs 写为 NowMs，清空审核占用锁。
        // 时间由外部传入而不是在内部取 clock，是为了让测试能断言具体数值。
        bool Approve(long long NowMs);

        // 审核驳回：PendingReview → Rejected
        // 前置：Status 为 PendingReview，且 Reason 非空字符串。
        // 副作用：Status 置 Rejected，RejectReason 置为 Reason，清空审核占用锁。
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
