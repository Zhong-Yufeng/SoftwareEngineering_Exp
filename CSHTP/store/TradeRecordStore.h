#pragma once
#include<optional>
#include<string>
#include<vector>
#include<cstddef>
#include"TradeRecord.h"

// ============================================================
// TradeRecordStore —— 成交记录的存取
//
// 三条设计原则：
//   1. 只增不删。成交记录是买卖双方唯一的凭证，也是平台流水的来源，
//      能删就等于没成交过。所以这里没有 Remove。
//   2. 不判断权限。它只按数据字段筛，「这条记录里有没有你」由
//      TradeRecord::IsParticipant 回答，是否允许查由服务层决定。
//   3. 查询条件收进一个结构体：所有字段都是 optional，为空表示「不限」，
//      多个条件同时给出时取「与」。写成 ListByBuyer / ListBySeller /
//      ListByTime 会把条件组合的数量炸开，新增一个条件只是加一个字段。
//
// 【为什么默认按成交时间从新到旧】
//   哈希表的遍历顺序不可预测，而「我的成交记录」唯一的阅读方式就是
//   最近的在最上面。默认排序让调用方不写 Sort 也能直接渲染。
// ============================================================

// 排序方式
enum class TradeSort
{
    None,           // 不排序，按存储容器的遍历顺序
    TradedNewest,   // 成交时间从新到旧（默认）
    TradedOldest,   // 成交时间从旧到新
    PriceAsc,       // 成交价从低到高
    PriceDesc       // 成交价从高到低
};

struct TradeQuery
{
    // 精确匹配。注意空串是「一个具体的值」，不是「不限」：
    // 传空串查不到任何记录，这是有意的，避免误传空 ID 时把整个库拉出来。
    std::optional<std::string> TradeID;         // 成交编号
    std::optional<std::string> ItemID;          // 商品编号（指向已删除的商品）
    std::optional<std::string> BuyerID;         // 买家
    std::optional<std::string> SellerID;        // 卖家

    // 「我相关的成交」：买家是本人或卖家是本人，一个条件顶两个。
    // 它只是数据条件，不是权限判定——判定由服务层结合登录用户做。
    std::optional<std::string> ParticipantID;

    // 模糊匹配：子串出现在字段中即可
    std::optional<std::string> Keyword;         // 商品标题
    std::optional<std::string> TradeLocation;   // 交易地点

    // 数值区间，两端都包含
    std::optional<long long> MinPriceCents;     // 成交价下限
    std::optional<long long> MaxPriceCents;     // 成交价上限
    std::optional<long long> TradedAfterMs;     // 成交时间不早于该时刻
    std::optional<long long> TradedBeforeMs;    // 成交时间不晚于该时刻

    TradeSort Sort = TradeSort::TradedNewest;
};

class TradeRecordStore
{
    public:
        virtual ~TradeRecordStore() = default;

        // 存一条（新增或覆盖）。
        // 收非 const 引用：新建的记录没有编号，编号由存储层分配
        //（「编号不重复」是整个集合的性质，单条记录保证不了）。
        // 覆盖只用于测试装置，正常流程里成交记录写完就不再改。
        // 返回 false 表示保存失败。
        virtual bool Save(TradeRecord& Record) = 0;

        // 按编号查。编号是主键，走哈希表，O(1)。
        virtual std::optional<TradeRecord> FindByID(const std::string& TradeID) const = 0;

        // 按条件查，返回满足全部非空条件的记录，已按 Cond.Sort 排好。
        // 返回拷贝：调用方改了结果不会污染库。
        virtual std::vector<TradeRecord> Query(const TradeQuery& Cond) const = 0;

        // 全部记录，按成交时间从新到旧（平台统计、管理员翻查用）。
        virtual std::vector<TradeRecord> All() const = 0;

        // 库里现在有多少条成交记录
        virtual std::size_t Size() const = 0;
};
