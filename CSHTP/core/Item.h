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
//   ItemStore::Save    负责分配 ItemID 并入库（调 AssignID）
// 所以 CreateDraft 不产生 ID。
//
// 【「草稿」的含义】创建时内容就必须齐全，所以草稿不再是「内容不全的
// 半成品」，而是「内容齐全、但还没提交审核」的商品。
//
// 【状态模型】4 个状态、5 条状态间的边 + 1 条删除出口，见 enums.h 的
// ItemStatus 注释。商品类回答两个问题：「我能不能从状态 A 走到状态 B」
// （Can* 判定 + 6 个写入接口），以及「站在某个人的位置上，能不能看见我」
// （IsVisibleTo，浏览者的身份由入参给出）。存储层不参与可见性判断，
// 只按数据条件筛选。
//
// 【封装原则】
//   所有字段都是 private，外部只能读（Get*）、只能问（Can*）、只能调
//   （6 个状态写入接口）。看状态的人不必知道字段长什么样，改状态的人
//   不必记得清哪些副作用——两者都由本类保证。
//
// 【成交】没有「交易成功」状态。卖家确认某条预约时，商品记录被从库里
// 删除，预约一并删除，同时生成一条 TradeRecord。所以「成交」不是状态，
// 是动作的结果；任何还存在于库里的商品，都还没有成交。
// ============================================================
class Item
{
    public:
        // ========================================================
        // 创建
        // ========================================================
        // 创建即要求内容齐全（草稿只是「未提交审核」，不是「内容不全」）：
        //   标题、描述、交易地点、联系方式非空；
        //   分类与成色必须是枚举里的合法值（enum class 能被强转成非法数字）；
        //   价格必须大于 0（单位分，整数，没有精度问题）；
        //   预约上限在 1~10 之间；卖家编号非空。
        // 以下字段不由调用方给出，在函数内固定：
        //   ItemID 留空（存储层分配）、Status=Draft、PublishedAtMs=0、
        //   RejectReason 为空。
        // 不合法返回 std::nullopt。
        static std::optional<Item> CreateDraft(const std::string& Title,
                                               const std::string& Description,
                                               Category ItemCategory,
                                               long long PriceCents,
                                               Condition ItemCondition,
                                               const std::string& TradeLocation,
                                               const std::string& Contact,
                                               int MaxReservations,
                                               const std::string& SellerID);

        // ========================================================
        // 只读访问
        // ========================================================
        // 返回 const 引用而不是拷贝：调用方大多只是看一眼、比较一下，
        // 拷贝一份字符串纯属浪费；而 const 引用既便宜又改不动。
        const std::string& GetItemID() const;
        const std::string& GetTitle() const;
        const std::string& GetDescription() const;
        Category GetCategory() const;
        long long GetPriceCents() const;
        Condition GetCondition() const;
        const std::string& GetTradeLocation() const;
        const std::string& GetContact() const;
        ItemStatus GetStatus() const;
        long long GetPublishedAtMs() const;
        const std::optional<std::string>& GetRejectReason() const;
        int GetMaxReservations() const;
        const std::string& GetSellerID() const;

        // ========================================================
        // 状态检查（只读，不改任何字段）
        //
        // 这些函数回答「当前这个对象能不能做某事」，不是「你能不能做」。
        // 后者要先知道调用者是谁：卖家只能动自己的商品，管理员只能审核，
        // 那是服务层的事，商品类看不见「调用者」这个概念。
        // ========================================================

        // 内容是否齐全（规则与 CreateDraft 相同）。
        // 【为什么创建时查过、这里还要有】内容在草稿/驳回阶段是可以改的，
        // 改就可能改坏；这份检查是内容进入审核前的最后一道闸门。
        bool IsContentComplete() const;

        // 允许卖家编辑内容：草稿、驳回。
        // 待审核期间内容冻结（否则管理员审的和卖家改的不是同一份东西），
        // 已发布之后改内容会误导买家。
        bool CanEdit() const;

        // 允许提交审核（只判状态）：草稿、驳回。
        bool CanSubmitForReview() const;

        // 允许撤回审核：待审核。
        bool CanWithdrawReview() const;

        // 允许审核通过：待审核。
        bool CanApprove() const;

        // 允许审核驳回：待审核（理由非空由 Reject 的入参检查负责）。
        bool CanReject() const;

        // 允许卖家删除：草稿、驳回。
        // 待审核必须先撤回（删除一个正在被别人审的对象，等于把东西从
        // 审核员手上抽走）；已发布不能直接删，只能走成交。
        bool CanDelete() const;

        // 允许被预约：已发布。
        // 【注意】这里只回答状态允不允许。「预约数是否已到
        // MaxReservations」要数库里的预约记录，属于服务层。
        bool CanReserve() const;

        // ========================================================
        // 可见性
        // ========================================================
        // 回答「站在 ViewerID 这个人的位置上，能不能看见这件商品」。
        // 浏览者的角色与编号由调用方传入——它们不是商品的属性，商品自己
        // 无从得知；但「拿这两个数和我自己的状态、卖家编号怎么比」这条
        // 规则属于商品，所以判定写在类里，而不是散在服务层的 if 里。
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
        //   「卖家」在 Role 上是 Buyer 的超集，所以已发布状态不必区分买家与
        //   卖家，只需排除管理员；而草稿/驳回必须靠 ViewerID == SellerID
        //   判定，光看角色分不出「这是哪个卖家的商品」。
        bool IsVisibleTo(Role ViewerRole, const std::string& ViewerID) const;

        // ========================================================
        // 状态写入（6 个）
        //
        // 统一口径：
        //   1. 先查、后写：所有检查都在第一次赋值之前，失败时对象原样不动，
        //      调用方拿到 false 不必回滚。
        //   2. 只改自己的字段，不查库、不碰别的类。
        //   3. 返回 bool 不说明原因，提示语由服务层结合自身上下文给。
        // ========================================================

        // 提交审核：Draft → PendingReview，Rejected → PendingReview
        // 前置：CanSubmitForReview() 且 IsContentComplete()。
        // 副作用：清空 RejectReason。
        // 【为什么必须清空理由】不变式是 (Status==Rejected) ⇔ RejectReason
        //   有值。重提后不清空，就会出现「待审核的商品挂着上一次的驳回
        //   理由」，之后 Approve 还会把这条陈旧理由带进已发布状态。
        bool SubmitForReview();

        // 撤回审核：PendingReview → Draft
        // 前置：CanWithdrawReview()。
        // 副作用：清空 RejectReason。
        // 【为什么撤回的目标是草稿而不是删除】撤回的意图是「我还要改」，
        //   草稿正好可编辑、也可再删；若撤回直接删除，卖家想改一个字就得重写。
        bool WithdrawReview();

        // 审核通过：PendingReview → Published
        // 前置：CanApprove()。
        // 副作用：PublishedAtMs 写为 NowMs。
        // 时间由外部传入而不是内部读时钟，是为了让测试能断言具体数值。
        bool Approve(long long NowMs);

        // 审核驳回：PendingReview → Rejected
        // 前置：CanReject() 且 Reason 非空。
        // 副作用：RejectReason 置为 Reason。
        // 【为什么理由必须非空】需求规定驳回必须填理由；空串和「没填」在
        //   业务上没有区别，所以在这里挡掉，不留给界面去校验。
        bool Reject(const std::string& Reason);

        // ========================================================
        // 存储层专用：分配编号
        // ========================================================
        // 商品编号由存储层分配，而字段是私有的，所以必须留一个口子；
        // 前置是「自己还没有编号」，防止重复调用把已有编号覆盖掉。
        bool AssignID(const std::string& NewID);

    private:
        // 默认构造函数私有：商品对象只能由 CreateDraft 造出来。
        // 拷贝构造仍由编译器隐式生成且公开，这是有意保留的：
        // 存储层要把商品拷进容器、成交记录要读它，这些都需要拷贝。
        Item() = default;

        std::string ItemID;                 
        std::string Title;
        std::string Description;
        Category ItemCategory = Category::Textbook;
        long long PriceCents = 0;           // 价格，单位分
        Condition ItemCondition = Condition::BrandNew;
        std::string TradeLocation;          // 交易地点
        std::string Contact;                // 卖家联系方式，仅卖家本人与成交后的买家可见
        ItemStatus Status = ItemStatus::Draft;
        long long PublishedAtMs = 0;        // 审核通过时才写，此前为 0
        std::optional<std::string> RejectReason;   // 驳回理由，仅 Rejected 时有值
        int MaxReservations = 1;            // 本商品最多接受几条预约，1~10
        std::string SellerID;
};
