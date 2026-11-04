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

// 至少一张图，且不允许用空字符串凑数：
// 只判 Images.empty() 的话，传入 {"", ""} 也能通过，那不是一张图。
bool HasAtLeastOneImage(const std::vector<std::string>& Images)
{
    if (Images.empty())
    {
        return false;
    }
    for (const std::string& Path : Images)
    {
        if (Path.empty())
        {
            return false;
        }
    }
    return true;
}

}  // namespace

std::optional<Item> Item::CreateDraft(const std::string& Title,
                                      const std::string& Description,
                                      Category ItemCategory,
                                      long long PriceCents,
                                      Condition ItemCondition,
                                      const std::vector<std::string>& Images,
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

    if (!HasAtLeastOneImage(Images))
    {
        return std::nullopt;
    }

    // 预约上限：需求规定卖家可自己调，范围 1~10
    if (MaxReservations < 1 || MaxReservations > 10)
    {
        return std::nullopt;
    }

    Item Draft;

    // ID 留给存储层；发布时间留到审核通过时写
    Draft.ItemID = "";
    Draft.Title = Title;
    Draft.Description = Description;
    Draft.ItemCategory = ItemCategory;
    Draft.PriceCents = PriceCents;
    Draft.ItemCondition = ItemCondition;
    Draft.Images = Images;
    Draft.TradeLocation = TradeLocation;
    Draft.Contact = Contact;
    Draft.Status = ItemStatus::Draft;
    Draft.PublishedAtMs = 0;
    Draft.MaxReservations = MaxReservations;
    Draft.SellerID = SellerID;

    // 驳回理由此刻必须为空：不变式 (Status==Rejected) ⇔ RejectReason 有值，
    // 草稿不是 Rejected，所以这里显式写成 nullopt，不依赖默认构造
    Draft.RejectReason = std::nullopt;

    // 刚建的草稿没人审，锁必须是空的；否则「撤回」会被一个不存在的管理员
    // 永久挡住。不变式：锁只有在 PendingReview 期间才可能非空。
    Draft.ReviewLockedByID = std::nullopt;

    return Draft;
}
