#include"test.h"
#include"Item.h"
#include<optional>
#include<string>
#include<vector>
#include<type_traits>

// ============================================================
// Item::CreateDraft 的冒烟测试
//
// 覆盖三件事：
//   1. 商品只能经由 CreateDraft 造出来，不能凭空构造再手填字段
//   2. 合法输入能造出对象，且「不由调用方给」的字段被正确初始化
//   3. 每一类非法输入都被挡住：必填项、价格、图片、枚举野值、预约上限
//
// 写法上用一个 MakeItem 造「模板参数」，每次只替换一个字段，
// 这样一条断言只考一个变量，失败时能立刻定位是哪条规则破了。
//
// 字段全部私有之后，断言一律走 Get* 访问器：测试和业务代码享受同一套
// 规则——能看见什么，就只能断言什么。
// ============================================================

using Test::Check;

namespace {

std::optional<Item> MakeItem(const std::string& Title = "高数教材",
                             const std::string& Description = "第七版，九成新",
                             Category ItemCategory = Category::Textbook,
                             long long PriceCents = 1500,
                             Condition ItemCondition = Condition::LikeNew,
                             const std::string& TradeLocation = "三食堂门口",
                             const std::string& Contact = "13800000000",
                             int MaxReservations = 3,
                             const std::string& SellerID = "seller-1")
{
    return Item::CreateDraft(Title, Description, ItemCategory, PriceCents,
                             ItemCondition, TradeLocation, Contact,
                             MaxReservations, SellerID);
}

}  // namespace

void RunItemCreateTests()
{
    // ============ 1. 只能经由 CreateDraft 构造 ============
    // 这是编译期属性，用类型特征在运行期问出来：
    // 若哪天有人把 Item 的构造函数改成公开，这一条立刻变红。
    Check(!std::is_default_constructible<Item>::value,
          "CreateDraft: Item 不允许默认构造，只能由 CreateDraft 创建");

    // ============ 2. 合法输入的字段装配 ============
    {
        auto A = MakeItem();
        Check(A.has_value(), "CreateDraft: 全部合法，应成功");
        if (A)
        {
            Check(A->GetTitle() == "高数教材",       "CreateDraft: Title 应等于入参");
            Check(A->GetDescription() == "第七版，九成新", "CreateDraft: Description 应等于入参");
            Check(A->GetCategory() == Category::Textbook, "CreateDraft: 分类应等于入参");
            Check(A->GetPriceCents() == 1500,        "CreateDraft: 价格应等于入参");
            Check(A->GetCondition() == Condition::LikeNew, "CreateDraft: 成色应等于入参");
            Check(A->GetTradeLocation() == "三食堂门口", "CreateDraft: 地点应等于入参");
            Check(A->GetContact() == "13800000000",  "CreateDraft: 联系方式应等于入参");
            Check(A->GetMaxReservations() == 3,      "CreateDraft: 预约上限应等于入参");
            Check(A->GetSellerID() == "seller-1",    "CreateDraft: 卖家应等于入参");
        }
    }

    // ============ 3. 不由调用方给的字段，必须被固定 ============
    {
        auto A = MakeItem();
        if (A)
        {
            Check(A->GetItemID().empty(),                       "CreateDraft: 编号应留空给存储层");
            Check(A->GetStatus() == ItemStatus::Draft,          "CreateDraft: 初始状态应为草稿");
            Check(A->GetPublishedAtMs() == 0,                   "CreateDraft: 发布时间应为 0");
            Check(!A->GetRejectReason().has_value(),            "CreateDraft: 驳回理由应为空");
        }
        else
        {
            Check(false, "CreateDraft: 前提失败——合法输入没能造出对象");
        }
    }

    // ============ 4. 必填文本字段 ============
    Check(!MakeItem("").has_value(),                       "CreateDraft: 空标题应被拒");
    Check(!MakeItem("高数教材", "").has_value(),            "CreateDraft: 空描述应被拒");
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                    Condition::LikeNew, "", "13800000000")
               .has_value(),                               "CreateDraft: 空交易地点应被拒");
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                    Condition::LikeNew, "三食堂门口", "")
               .has_value(),                               "CreateDraft: 空联系方式应被拒");
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                    Condition::LikeNew, "三食堂门口", "13800000000", 3, "")
               .has_value(),                               "CreateDraft: 空卖家应被拒");

    // ============ 5. 价格边界（单位分，整数比较）============
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 0, Condition::LikeNew)
               .has_value(),                               "CreateDraft: 价格 0 应被拒");
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, -1, Condition::LikeNew)
               .has_value(),                               "CreateDraft: 负价格应被拒");
    {
        auto A = MakeItem("高数教材", "第七版", Category::Textbook, 1, Condition::LikeNew);
        Check(A.has_value(),                               "CreateDraft: 价格 1 分（边界值）应成功");
    }

    // ============ 6. 预约上限边界 ============
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                    Condition::LikeNew, "三食堂门口", "13800000000", 0)
               .has_value(),                               "CreateDraft: 上限 0 应被拒");
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                    Condition::LikeNew, "三食堂门口", "13800000000", 11)
               .has_value(),                               "CreateDraft: 上限 11 应被拒");
    {
        auto One = MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                            Condition::LikeNew, "三食堂门口", "13800000000", 1);
        Check(One.has_value(),                             "CreateDraft: 上限 1（边界值）应成功");
    }
    {
        auto Ten = MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                            Condition::LikeNew, "三食堂门口", "13800000000", 10);
        Check(Ten.has_value(),                             "CreateDraft: 上限 10（边界值）应成功");
    }

    // ============ 8. 枚举野值（static_cast 强转出来的非法值）============
    Check(!MakeItem("高数教材", "第七版", static_cast<Category>(999), 1500,
                    Condition::LikeNew).has_value(),
          "CreateDraft: 强转出来的非法分类应被拒");
    Check(!MakeItem("高数教材", "第七版", Category::Textbook, 1500,
                    static_cast<Condition>(999)).has_value(),
          "CreateDraft: 强转出来的非法成色应被拒");

    // ============ 9. 四种分类与三种成色都能用 ============
    Check(MakeItem("书", "d", Category::Textbook,  100, Condition::BrandNew).has_value(),
          "CreateDraft: 教材 + 全新 应成功");
    Check(MakeItem("书", "d", Category::Digital,   100, Condition::LikeNew).has_value(),
          "CreateDraft: 数码 + 九成新 应成功");
    Check(MakeItem("书", "d", Category::Clothing,  100, Condition::Flawed).has_value(),
          "CreateDraft: 衣物 + 有瑕疵 应成功");
    Check(MakeItem("书", "d", Category::DailyGoods, 100, Condition::BrandNew).has_value(),
          "CreateDraft: 日用品 + 全新 应成功");
}
