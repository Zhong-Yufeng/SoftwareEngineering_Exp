#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"TradeRecordStore.h"

// ============================================================
// InMemoryTradeRecordStore —— 把成交记录放在一个哈希表里的实现
//
// 与 InMemoryReservationStore 几乎同构：记录字段是公开的，没有状态机，
// 所以 Save 里可以直接写 TradeID，不必留 AssignID 的口子。
//
// 几个不变的约束：
//   1. 编号由这里分配（trade-1、trade-2……），因为「不重复」是集合的性质。
//   2. 存的是拷贝，取出来的也是拷贝，库里的记录不会被外面改坏。
//   3. 按买家/卖家/时间查是线性遍历：成交记录只有几十到几百条，
//      换二级索引只是为了快一点，却要多维护一份「索引和主表怎么同步」，
//      不值得。真到数据库那一层，加索引是一行 SQL 的事。
// ============================================================
class InMemoryTradeRecordStore : public TradeRecordStore
{
    public:
        bool Save(TradeRecord& Record) override;
        std::optional<TradeRecord> FindByID(const std::string& TradeID) const override;
        std::vector<TradeRecord> Query(const TradeQuery& Cond) const override;
        std::vector<TradeRecord> All() const override;
        std::size_t Size() const override;

    private:
        // 生成下一个没被占用的编号，形如 trade-1
        std::string MakeNextID();

        std::unordered_map<std::string, TradeRecord> Records;
        long long LastAssignedNumber = 0;
};
