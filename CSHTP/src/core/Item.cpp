#include"Item.h"

namespace
{
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

    // 内容规则只写一处，创建、修改、可提交判断共用
    bool IsValidInfo(const ItemInfo& Info)
    {
        if (Info.Title.empty() || Info.Description.empty())
        {
            return false;
        }
        if (Info.TradeLocation.empty() || Info.Contact.empty())
        {
            return false;
        }
        if (!IsValidCategory(Info.ItemCategory) || !IsValidCondition(Info.ItemCondition))
        {
            return false;
        }
        return Info.PriceCents > 0;
    }
}

std::optional<Item> Item::CreateDraft(const ItemInfo& Info, const std::string& SellerID)
{
    if (SellerID.empty() || !IsValidInfo(Info))
    {
        return std::nullopt;
    }

    Item Draft;
    Draft.ItemID = "";
    Draft.Title = Info.Title;
    Draft.Description = Info.Description;
    Draft.ItemCategory = Info.ItemCategory;
    Draft.PriceCents = Info.PriceCents;
    Draft.ItemCondition = Info.ItemCondition;
    Draft.TradeLocation = Info.TradeLocation;
    Draft.Contact = Info.Contact;
    Draft.Status = ItemStatus::Draft;
    Draft.PublishedAtMs = 0;
    Draft.RejectReason = std::nullopt;
    Draft.SellerID = SellerID;
    return Draft;
}

bool Item::IsContentComplete() const
{
    ItemInfo Info;
    Info.Title = Title;
    Info.Description = Description;
    Info.ItemCategory = ItemCategory;
    Info.PriceCents = PriceCents;
    Info.ItemCondition = ItemCondition;
    Info.TradeLocation = TradeLocation;
    Info.Contact = Contact;
    return IsValidInfo(Info);
}

bool Item::IsVisibleTo(Role ViewerRole, const std::string& ViewerID) const
{
    const bool IsOwner = !ViewerID.empty() && ViewerID == SellerID;

    switch (Status)
    {
    case ItemStatus::Draft:
    case ItemStatus::Rejected:
        return IsOwner;
    case ItemStatus::PendingReview:
    case ItemStatus::OffShelf:
    case ItemStatus::ViolationOffShelf:
        return IsOwner || ViewerRole == Role::Admin;
    case ItemStatus::Published:
    case ItemStatus::Confirmed:
    case ItemStatus::Traded:
        return true;
    }
    return false;
}

bool Item::SubmitForReview()
{
    if ((Status != ItemStatus::Draft && Status != ItemStatus::Rejected) || !IsContentComplete())
    {
        return false;
    }

    Status = ItemStatus::PendingReview;
    RejectReason = std::nullopt;
    return true;
}

bool Item::Approve(long long NowMs)
{
    if (Status != ItemStatus::PendingReview || NowMs <= 0)
    {
        return false;
    }

    Status = ItemStatus::Published;
    PublishedAtMs = NowMs;
    RejectReason = std::nullopt;
    return true;
}

bool Item::Reject(const std::string& Reason)
{
    if (Status != ItemStatus::PendingReview || Reason.empty())
    {
        return false;
    }

    Status = ItemStatus::Rejected;
    RejectReason = Reason;
    return true;
}

bool Item::Confirm()
{
    if (Status != ItemStatus::Published)
    {
        return false;
    }

    Status = ItemStatus::Confirmed;
    return true;
}

bool Item::CompleteTrade()
{
    if (Status != ItemStatus::Confirmed)
    {
        return false;
    }

    Status = ItemStatus::Traded;
    return true;
}

// ----- 以下函数暂未实现 ----------------------------------------
bool Item::Modify(const ItemInfo&)
{
    // TODO
    return false;
}

bool Item::OffShelf()
{
    // TODO
    return false;
}

bool Item::ViolationOffShelf()
{
    // TODO
    return false;
}
