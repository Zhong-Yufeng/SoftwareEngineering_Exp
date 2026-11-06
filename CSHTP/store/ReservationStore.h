#pragma once
#include<optional>
#include<vector>
#include<string>
#include<cstddef>
#include"Reservation.h"

// ============================================================
// ReservationStore —— 预约的存取
//
// 和 ItemStore 一样，这里只做「存、取、删」三件事，不解释业务规则。
// 「同一个买家同一件商品只能约一次」「一件商品最多 MaxReservations 条」
// 「一个买家最多 20 条」这三条，都要先查库再决定，属于服务层的编排；
// 存储层只提供能查出这些数字的工具。
//
// 【为什么要 FindByBuyerAndItem 这样一个方法】
//   预约的「业务主键」不是 ReservationID，而是（买家，商品）这一对。
//   「这条是不是已经约过了」的判定，本质是按这对值查有没有记录。
//   如果只提供 ListByBuyer，调用方得自己遍历一遍再比对商品编号，
//   等于把「按哪两列查」这件事散到每个调用点。
//
// 【为什么 CountByXxx 和 ListByXxx 并存】
//   内存版两者都是 O(n)，看起来重复。但接口上要分清：
//   配额检查只关心数量，不关心内容；将来换成数据库实现，
//   一个是 SELECT *，一个是 SELECT COUNT(*)，代价差很远。
//   现在就把这个区别留在接口里，以后换实现不用改调用方。
//
// 【删除的两个入口】
//   Remove(ID)     ：买家主动取消，或卖家确认成交时删这一条。
//   RemoveByItem   ：商品被删 / 成交时，把这件商品剩下的预约一次清空。
//                    返回删掉的条数，服务层用它写日志（作废了几条）。
// ============================================================
class ReservationStore
{
    public:
        virtual ~ReservationStore() = default;

        // 保存一条预约（新增或覆盖）。
        // 收非 const 引用：新建的预约没有编号，编号由存储层分配
        //（「编号不重复」是整个集合的性质，单条记录保证不了）。
        // 返回 false 表示保存失败。
        virtual bool Save(Reservation& Order) = 0;

        // 按编号查。编号是主键，走哈希表，O(1)。
        virtual std::optional<Reservation> FindByID(const std::string& ReservationID) const = 0;

        // 按「买家 + 商品」查。用于判断「这个人是不是已经约过这件商品」。
        // 没有记录返回 std::nullopt。
        virtual std::optional<Reservation> FindByBuyerAndItem(const std::string& BuyerID,
                                                              const std::string& ItemID) const = 0;

        // 某个买家的全部预约（「我的预约」页）。
        virtual std::vector<Reservation> ListByBuyer(const std::string& BuyerID) const = 0;

        // 某件商品的全部预约（卖家查看谁想买）。
        virtual std::vector<Reservation> ListByItem(const std::string& ItemID) const = 0;

        // 数量查询，供配额检查使用。
        virtual std::size_t CountByBuyer(const std::string& BuyerID) const = 0;
        virtual std::size_t CountByItem(const std::string& ItemID) const = 0;

        // 按编号删除一条。返回 false 表示本来就没有这条。
        virtual bool Remove(const std::string& ReservationID) = 0;

        // 删除某件商品的全部预约，返回删掉的条数。
        virtual std::size_t RemoveByItem(const std::string& ItemID) = 0;
};
