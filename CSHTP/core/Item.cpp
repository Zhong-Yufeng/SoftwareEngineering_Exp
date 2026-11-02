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

    return Draft;
}
