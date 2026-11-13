#pragma once
#include<optional>
#include<string>
#include"enums.h"

// 商品的可编辑内容，创建与修改共用
struct ItemInfo
{
    std::string Title;
    std::string Description;
    Category ItemCategory = Category::Textbook;
    long long PriceCents = 0;
    Condition ItemCondition = Condition::BrandNew;
    std::string TradeLocation;
    std::string Contact;
};

class Item
{
    public:
        std::string ItemID;                     // 由存储层分配
        std::string Title;
        std::string Description;
        Category ItemCategory = Category::Textbook;
        long long PriceCents = 0;               // 单位：分
        Condition ItemCondition = Condition::BrandNew;
        std::string TradeLocation;
        std::string Contact;
        ItemStatus Status = ItemStatus::Draft;
        long long PublishedAtMs = 0;
        std::optional<std::string> RejectReason;    // 仅驳回时有值
        std::string SellerID;

        static std::optional<Item> CreateDraft(const ItemInfo& Info, const std::string& SellerID);

        // 只读判定，不改字段
        bool IsContentComplete() const;
        bool IsVisibleTo(Role ViewerRole, const std::string& ViewerID) const;

        // 状态写入：前置条件不满足时返回 false，对象保持不变
        bool SubmitForReview();                     // 草稿/驳回 → 待审核
        bool Approve(long long NowMs);              // 待审核 → 已发布
        bool Reject(const std::string& Reason);     // 待审核 → 驳回
        bool Confirm();                             // 已发布 → 已确认
        bool CompleteTrade();                       // 已确认 → 交易完成

        // ----- 以下函数暂未实现 ----------------------------------------
        bool Modify(const ItemInfo& Info);
        bool OffShelf();
        bool ViolationOffShelf();

    private:
        Item() = default;
};
