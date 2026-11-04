#include"Item.h"

std::optional<Item> Item::CreateDraft(const std::string& Title,
                                      const std::string& Description,
                                      Category ItemCategory,
                                      long long PriceCents,
                                      Condition ItemCondition,
                                      const std::string& TradeMethod,
                                      const std::string& TradeLocation,
                                      const std::string& Contact,
                                      const std::string& SellerID)
{
    // 标题必须有
    if (Title.empty())
    {
        return std::nullopt;
    }

    // 必须知道是谁发布的，否则这件商品没有归属
    if (SellerID.empty())
    {
        return std::nullopt;
    }

    // 价格不能为负；0 元允许（赠品场景）
    if (PriceCents < 0)
    {
        return std::nullopt;
    }

    Item Draft;

    // ID 留给存储层；发布时间留到提交审核时写
    Draft.ItemID = "";
    Draft.Title = Title;
    Draft.Description = Description;
    Draft.ItemCategory = ItemCategory;
    Draft.PriceCents = PriceCents;
    Draft.ItemCondition = ItemCondition;
    Draft.TradeMethod = TradeMethod;
    Draft.TradeLocation = TradeLocation;
    Draft.Contact = Contact;
    Draft.Status = ItemStatus::Draft;
    Draft.PublishedAtMs = 0;
    Draft.SellerID = SellerID;

    // 驳回理由此刻必须为空：不变式 (Status==Rejected) ⇔ RejectReason 有值，
    // 草稿不是 Rejected，所以这里显式写成 nullopt，不依赖默认构造
    Draft.RejectReason = std::nullopt;

    // 刚建的草稿没人审，锁必须是空的；否则「撤回」会被一个不存在的管理员
    // 永久挡住。不变式：锁只有在 PendingReview 期间才可能非空。
    Draft.ReviewLockedByID = std::nullopt;

    // 预约上限默认 10（需求的上限）。卖家想收更少，在草稿/驳回阶段改这个字段，
    // 范围 1~10 由 SubmitForReview 统一校验，不必在构造函数和编辑两处各写一遍。
    Draft.MaxReservations = 10;

    return Draft;
}
