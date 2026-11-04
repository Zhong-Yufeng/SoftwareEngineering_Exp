#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include"ItemStore.h"

// ============================================================
// InMemoryItemStore —— 把商品放在一个哈希表里的实现
//
// 为什么先用内存版：数据库不是这个项目要练的东西，而且内存版没有任何
// 外部依赖，测试能直接跑。ItemStore 是纯虚类，以后换数据库实现只要
// 新写一个子类，调用方一行都不用改。
//
// 几个不变的约束：
//   1. 不能用 Items[ID] —— unordered_map::operator[] 在 key 不存在时会
//      先默认构造一个 Item，而 Item 的默认构造函数是私有的，编译不过。
//      必须用 find / insert_or_assign / erase。
//   2. 编号由这里分配，因为「不重复」是整个集合的性质。
//   3. 存的是 Item 的拷贝，取出来的也是拷贝，库里的对象不会被外面改坏。
// ============================================================
class InMemoryItemStore : public ItemStore
{
    public:
        bool Save(Item& Goods) override;
        std::optional<Item> FindByID(const std::string& ItemID) const override;
        std::vector<Item> Query(const ItemQuery& Cond) const override;
        bool Remove(const std::string& ItemID) override;

        // 库里现在有多少件商品（测试与平台统计用）
        std::size_t Size() const;

    private:
        // 生成下一个没被占用的编号，形如 item-1
        std::string MakeNextID();

        // 单件商品是否满足查询条件
        bool Matches(const Item& Goods, const ItemQuery& Cond) const;

        std::unordered_map<std::string, Item> Items;
        long long LastAssignedNumber = 0;
};
