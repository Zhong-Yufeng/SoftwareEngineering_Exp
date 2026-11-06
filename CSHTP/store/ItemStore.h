#pragma once
#include<optional>
#include<vector>
#include<string>
#include"Item.h"

// ============================================================
// 商品查询
//
// 【为什么不是一个 ListByXxx 一个方法】
//   如果写成 ListBySeller / ListByCategory / ListByPriceRange，
//   那么「某个卖家的、属于数码类的、价格 100~500 元的、已发布的商品」
//   就要再加一个方法。条件两两组合就有十几种，方法数量会爆炸。
//   所以把条件收进一个结构体：所有字段都是 optional，为空表示「不限」，
//   多个条件同时给出时取「与」。新增一个条件只是加一个字段。
//
// 【哪些属性可以查、哪些不行】
//   可查：标题、描述、分类、成色、价格、状态、卖家、交易地点、发布时间。
//   不可查：
//     - Contact（联系方式）：只有卖家本人和成交后的买家能看。若它能当
//       查询条件，就能用「138xxxx」反查商品，等于绕过隐私保护。
//     - MaxReservations：预约上限是卖家自己的设置，与买家无关。
//     - RejectReason / ReviewLockedByID：审核过程的内部信息，买家不该知道。
// ============================================================

// 排序方式
// 【为什么排序放在查询里】列表页上「按价格排序」「最新发布」和筛选是
// 同一个动作的两半：先筛后排，一次调用拿到的就是可以直接渲染的列表。
enum class ItemSort
{
    None,           // 不排序，按存储容器的遍历顺序
    PriceAsc,       // 价格从低到高
    PriceDesc,      // 价格从高到低
    PublishedNewest // 审核通过时间从新到旧
};

struct ItemQuery
{
    std::optional<std::string> Keyword;         // 标题或描述中包含该子串
    std::optional<Category> ItemCategory;       // 商品分类
    std::optional<Condition> ItemCondition;     // 商品成色
    std::optional<long long> MinPriceCents;     // 价格下限（含），单位分
    std::optional<long long> MaxPriceCents;     // 价格上限（含），单位分
    std::optional<ItemStatus> Status;           // 商品状态
    std::optional<std::string> SellerID;        // 卖家编号
    std::optional<std::string> TradeLocation;   // 交易地点中包含该子串
    std::optional<long long> PublishedAfterMs;  // 发布时间不早于该时刻

    ItemSort Sort = ItemSort::None;
};

// ============================================================
// ItemStore —— 商品的存取
//
// 「store」只负责三件事：存进去、按条件取出来、删掉。
// 它不判断「谁有资格看」——那是 core 层 Item::IsVisibleTo 的事，调用方拿到
// 列表后自己过滤；也不判断「能不能删」——那是 Item::CanDelete 的事。
// 存储层不解释业务规则，规则只写在 core 里，这样规则只有一份。
// ============================================================
class ItemStore
{
    public:
        virtual ~ItemStore() = default;

        // 保存一件商品（新增或覆盖）。
        //
        // 收非 const 引用而不是 const 引用，只有一个原因：商品新建时没有
        // 编号，编号由存储层分配（「编号不重复」是整个集合的性质，单件商品
        // 自己保证不了），所以这里可能要把编号写回对象里。
        // 已有编号则按该编号覆盖（同一件商品改了状态后再存一次）。
        // 返回 false 表示保存失败。
        virtual bool Save(Item& Goods) = 0;

        // 按编号查。编号是主键，走哈希表，O(1)。
        virtual std::optional<Item> FindByID(const std::string& ItemID) const = 0;

        // 按条件查，返回满足全部非空条件的商品。
        // 返回的是拷贝：调用方改了结果不会污染库，代价是每次查询拷一遍。
        // 商品总量是几十到几百件，这个代价可以接受。
        virtual std::vector<Item> Query(const ItemQuery& Cond) const = 0;

        // 按编号删除。删除用于：卖家删草稿/驳回商品、成交后删除商品。
        // 返回 false 表示本来就没有这件商品。
        virtual bool Remove(const std::string& ItemID) = 0;
};
