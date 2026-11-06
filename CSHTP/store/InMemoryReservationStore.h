#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"ReservationStore.h"

// ============================================================
// InMemoryReservationStore —— 把预约放在一个哈希表里的实现
//
// 与 InMemoryItemStore 的差别只有一处：预约的字段是公开的（它没有状态机，
// 不需要封装），所以 Save 里可以直接写 R.ReservationID，不必像商品那样
// 留一个 AssignID 的口子。
//
// 几个不变的约束：
//   1. 编号由这里分配，因为「不重复」是整个集合的性质。
//   2. 存的是拷贝，取出来的也是拷贝，库里的记录不会被外面改坏。
//   3. 按买家/商品查是线性遍历：内存数据量只有几十到几百条，
//      换成二级索引只是为了快一点，但会多出「索引和主表怎么同步」的问题，
//      不值得。真到数据库那一层，加索引是一行 SQL 的事。
// ============================================================
class InMemoryReservationStore : public ReservationStore
{
    public:
        bool Save(Reservation& Order) override;
        std::optional<Reservation> FindByID(const std::string& ReservationID) const override;
        std::optional<Reservation> FindByBuyerAndItem(const std::string& BuyerID,
                                                      const std::string& ItemID) const override;
        std::vector<Reservation> ListByBuyer(const std::string& BuyerID) const override;
        std::vector<Reservation> ListByItem(const std::string& ItemID) const override;
        std::size_t CountByBuyer(const std::string& BuyerID) const override;
        std::size_t CountByItem(const std::string& ItemID) const override;
        bool Remove(const std::string& ReservationID) override;
        std::size_t RemoveByItem(const std::string& ItemID) override;

        // 库里现在有多少条预约（测试与平台统计用）
        std::size_t Size() const;

    private:
        // 生成下一个没被占用的编号，形如 res-1
        std::string MakeNextID();

        std::unordered_map<std::string, Reservation> Orders;
        long long LastAssignedNumber = 0;
};
