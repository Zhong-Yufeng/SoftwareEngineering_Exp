#include"Item.h"

namespace {

// enum class 的底层类型是整数，可以被 static_cast 成枚举里根本不存在的值
// （例如 static_cast<Category>(999) 是合法的 C++）。所以「种类必须是合法的
// 选项」这件事，只能靠显式列举所有合法值来保证，光看类型是看不出来的。
bool IsValidCategory(Category ItemCategory)
{
    switch (ItemCategory)
    {
    case Category::Textbook:
    case Category::Digital:
    case Category::Clothing:
    case Category::DailyGoods:
        return true;
    }
    return false;
}

bool IsValidCondition(Condition ItemCondition)
{
    switch (ItemCondition)
    {
    case Condition::BrandNew:
    case Condition::LikeNew:
    case Condition::Flawed:
        return true;
    }
    return false;
}

}  // namespace

// ============================================================
// 创建
// ============================================================
std::optional<Item> Item::CreateDraft(const std::string& Title,
                                      const std::string& Description,
                                      Category ItemCategory,
                                      long long PriceCents,
                                      Condition ItemCondition,
                                      const std::string& TradeLocation,
                                      const std::string& Contact,
                                      int MaxReservations,
                                      const std::string& SellerID)
{
    // 必填的文本字段：缺一个，这件商品就没法给别人看
    if (Title.empty())
    {
        return std::nullopt;
    }
    if (Description.empty())
    {
        return std::nullopt;
    }
    if (TradeLocation.empty())
    {
        return std::nullopt;
    }
    if (Contact.empty())
    {
        return std::nullopt;
    }

    // 必须知道是谁发布的，否则这件商品没有归属
    if (SellerID.empty())
    {
        return std::nullopt;
    }

    // 两个「下拉选择」字段：值必须来自枚举，不能是强转出来的野值
    if (!IsValidCategory(ItemCategory))
    {
        return std::nullopt;
    }
    if (!IsValidCondition(ItemCondition))
    {
        return std::nullopt;
    }

    // 价格必须大于 0：单位是分，不会有小数误差，所以可以直接比较整数。
    // 0 元不放行——0 和「卖家还没填」在界面上长得一样，允许 0 等于给
    // 「忘记填价格」开了一条后路。若以后真要支持免费赠送，应当加一个
    // 独立的「免费」标记，而不是用价格 0 来兼职表达。
    if (PriceCents <= 0)
    {
        return std::nullopt;
    }

    // 预约上限：需求规定卖家可自己调，范围 1~10
    if (MaxReservations < 1 || MaxReservations > 10)
    {
        return std::nullopt;
    }

    // 全部检查通过，这时才开始写字段。
    // 「检查在前、赋值在后」这个顺序在全类统一，好处是任何一步返回
    // nullopt/false 时，对象都还是原样，调用方不必回滚半成品。
    Item Draft;

    // ID 留给存储层；发布时间留到审核通过时写
    Draft.ItemID = "";
    Draft.Title = Title;
    Draft.Description = Description;
    Draft.ItemCategory = ItemCategory;
    Draft.PriceCents = PriceCents;
    Draft.ItemCondition = ItemCondition;
    Draft.TradeLocation = TradeLocation;
    Draft.Contact = Contact;
    Draft.Status = ItemStatus::Draft;
    Draft.PublishedAtMs = 0;
    Draft.MaxReservations = MaxReservations;
    Draft.SellerID = SellerID;

    // 驳回理由此刻必须为空：不变式 (Status==Rejected) ⇔ RejectReason 有值，
    // 草稿不是 Rejected，所以这里显式写成 nullopt，不依赖默认构造
    Draft.RejectReason = std::nullopt;

    return Draft;
}

// ============================================================
// 只读访问
// ============================================================
const std::string& Item::GetItemID() const              { return ItemID; }
const std::string& Item::GetTitle() const               { return Title; }
const std::string& Item::GetDescription() const         { return Description; }
Category Item::GetCategory() const                      { return ItemCategory; }
long long Item::GetPriceCents() const                   { return PriceCents; }
Condition Item::GetCondition() const                    { return ItemCondition; }
const std::string& Item::GetTradeLocation() const       { return TradeLocation; }
const std::string& Item::GetContact() const             { return Contact; }
ItemStatus Item::GetStatus() const                      { return Status; }
long long Item::GetPublishedAtMs() const                { return PublishedAtMs; }
const std::optional<std::string>& Item::GetRejectReason() const
{
    return RejectReason;
}
int Item::GetMaxReservations() const                    { return MaxReservations; }
const std::string& Item::GetSellerID() const            { return SellerID; }

// ============================================================
// 状态检查
//
// 这一节全是 const 函数：只读 this，不改任何字段。所以它们既能在业务
// 逻辑里用，也能在界面里直接调（比如决定按钮要不要置灰）。
// ============================================================

bool Item::IsContentComplete() const
{
    // 规则与 CreateDraft 完全一致。两处写了两遍相同的判断，这是有代价的
    // ——规则一改就得改两处。之所以接受这个代价：创建处不依赖已有对象，
    // 这里依赖已有对象，两者没法简单地复用同一个函数（创建时字段还没装配）。
    if (Title.empty() || Description.empty())
    {
        return false;
    }
    if (TradeLocation.empty() || Contact.empty() || SellerID.empty())
    {
        return false;
    }
    if (!IsValidCategory(ItemCategory) || !IsValidCondition(ItemCondition))
    {
        return false;
    }
    if (PriceCents <= 0)
    {
        return false;
    }
    if (MaxReservations < 1 || MaxReservations > 10)
    {
        return false;
    }
    return true;
}

bool Item::CanEdit() const
{
    return Status == ItemStatus::Draft || Status == ItemStatus::Rejected;
}

bool Item::CanSubmitForReview() const
{
    // 只判状态；内容是否齐全由 IsContentComplete 单独负责，
    // 这样「不能提交」之后调用方还能再问一句「是状态不对还是内容不全」。
    return Status == ItemStatus::Draft || Status == ItemStatus::Rejected;
}

bool Item::CanWithdrawReview() const
{
    return Status == ItemStatus::PendingReview;
}

bool Item::CanApprove() const
{
    return Status == ItemStatus::PendingReview;
}

bool Item::CanReject() const
{
    return Status == ItemStatus::PendingReview;
}

bool Item::CanDelete() const
{
    return Status == ItemStatus::Draft || Status == ItemStatus::Rejected;
}

bool Item::CanReserve() const
{
    return Status == ItemStatus::Published;
}

// ============================================================
// 状态写入
//
// 每个函数的形状都一样：
//   1. 检查（调上面的 Can*，必要时加参数检查）；
//   2. 不通过就 return false，一个字段都还没碰；
//   3. 通过后连续赋值，包括那些「必须跟着一起改」的副作用字段。
// 副作用（清理由、写时间）之所以放在这里而不是让调用方自己做，
// 是因为它们不是业务判断，而是这条边自带的后果；放外面就会被漏写。
// ============================================================

bool Item::SubmitForReview()
{
    if (!CanSubmitForReview() || !IsContentComplete())
    {
        return false;
    }

    Status = ItemStatus::PendingReview;
    RejectReason = std::nullopt;        // 重提时清掉上一次的驳回理由
    return true;
}

bool Item::WithdrawReview()
{
    if (!CanWithdrawReview())
    {
        return false;
    }

    Status = ItemStatus::Draft;
    RejectReason = std::nullopt;
    return true;
}

bool Item::Approve(long long NowMs)
{
    if (!CanApprove())
    {
        return false;
    }

    Status = ItemStatus::Published;
    PublishedAtMs = NowMs;
    return true;
}

bool Item::Reject(const std::string& Reason)
{
    if (!CanReject() || Reason.empty())
    {
        return false;
    }

    Status = ItemStatus::Rejected;
    RejectReason = Reason;
    return true;
}

// ============================================================
// 存储层专用
// ============================================================
bool Item::AssignID(const std::string& NewID)
{
    // 只在「自己还没有编号」时允许分配一次。
    // 允许覆盖的话，同一个对象在两次 Save 之间换了编号，预约表里指向
    // 旧编号的记录就全成了孤儿。
    if (NewID.empty() || !ItemID.empty())
    {
        return false;
    }

    ItemID = NewID;
    return true;
}

// ============================================================
// 可见性
// ============================================================
bool Item::IsVisibleTo(Role ViewerRole, const std::string& ViewerID) const
{
    // 成员函数直接读私有字段，不必再走 Get*——Get* 是给类外准备的，
    // 类内用自己人和用普通变量没有区别。
    //
    // 「是不是自己的商品」只能靠编号比较：角色上卖家与买家没有区别，
    // 光看 Role 分不出「这是哪个卖家的商品」。
    const bool IsOwner = (ViewerID == SellerID);

    switch (Status)
    {
        case ItemStatus::Draft:
        case ItemStatus::Rejected:
            // 未提交审核的东西不该被任何人看见，管理员也一样——
            // 审核的对象是「提交上来的稿子」，不是「卖家硬盘里的草稿」。
            return IsOwner;

        case ItemStatus::PendingReview:
            // 卖家要能看到「审核中」，管理员要能看到才能审。
            return IsOwner || ViewerRole == Role::Admin;

        case ItemStatus::Published:
            // 已发布对所有买家、所有卖家可见；管理员的职责在审核台，
            // 不需要在商品列表里再看到一遍。
            return ViewerRole != Role::Admin;
    }

    // 强转成非法枚举值时走到这里。枚举能被 static_cast 成任意整数，
    // 所以兜底返回「不可见」比返回 true 安全。
    return false;
}
