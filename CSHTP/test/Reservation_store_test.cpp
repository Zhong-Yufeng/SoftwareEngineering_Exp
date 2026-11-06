#include"test.h"
#include"Reservation.h"
#include"InMemoryReservationStore.h"

// ============================================================
// 预约存储层的冒烟测试
//
// 覆盖：
//   编号分配与覆盖保存
//   按编号 / 按（买家+商品）查询
//   按买家 / 按商品列列表
//   两个方向的计数
//   删除单条 / 按商品批量删除
//   结果与库的隔离
//
// 【测试的写法约定】所有记录都走 Reservation::Create 造出来，
// 不手摆字段——这样测的是「真实能造出来的数据」，而不是想象出来的。
// ============================================================

using Test::Check;

namespace
{
    // 造一条合法预约。Create 失败说明测试装置本身有问题，
    // 这里也记一条断言，免得后面拿空对象继续跑。
    Reservation MakeOrder(const std::string& Buyer,
                          const std::string& Item,
                          long long Ms = 1000)
    {
        auto R = Reservation::Create(Buyer, Item, Ms);
        Check(R.has_value(), "测试装置: 造一条预约应当成功");
        return R.value();
    }
}

void RunReservationStoreTests()
{
    // ================= 1. 保存与编号分配 =================
    {
        InMemoryReservationStore Store;

        Reservation A = MakeOrder("u-buyer-1", "item-1");
        Check(A.ReservationID.empty(), "Save 前: 新预约不应有编号");
        Check(Store.Save(A), "Save: 新预约应保存成功");
        Check(A.ReservationID == "res-1", "Save: 第一条应分配 res-1");

        Reservation B = MakeOrder("u-buyer-1", "item-2");
        Check(Store.Save(B), "Save: 第二条应保存成功");
        Check(B.ReservationID == "res-2", "Save: 第二条应分配 res-2");

        Check(Store.Size() == 2, "Save: 库中应有 2 条预约");
    }

    // ================= 2. 编号不被复用 =================
    {
        InMemoryReservationStore Store;

        Reservation A = MakeOrder("u-buyer-1", "item-1");
        Store.Save(A);
        Check(Store.Remove("res-1"), "编号不复用: 应先删掉 res-1");

        Reservation B = MakeOrder("u-buyer-1", "item-1");
        Check(Store.Save(B), "编号不复用: 新预约应保存成功");
        Check(B.ReservationID == "res-2", "编号不复用: 删掉 res-1 后新记录应是 res-2，不是 res-1");
    }

    // ================= 3. 带编号保存与覆盖 =================
    {
        InMemoryReservationStore Store;

        Reservation A = MakeOrder("u-buyer-1", "item-1");
        A.ReservationID = "res-special";
        Check(Store.Save(A), "覆盖: 带编号应保存成功");
        Check(Store.Size() == 1, "覆盖: 第一次保存后应只有 1 条");

        Reservation B = MakeOrder("u-buyer-2", "item-2");
        B.ReservationID = "res-special";
        Check(Store.Save(B), "覆盖: 同编号再存应成功");
        Check(Store.Size() == 1, "覆盖: 同编号两次保存仍应只有 1 条");

        auto Found = Store.FindByID("res-special");
        Check(Found.has_value(), "覆盖: 按编号应能查到");
        Check(Found->BuyerID == "u-buyer-2", "覆盖: 查到的应是后写入的那条");
    }

    // ================= 4. 按编号查询 =================
    {
        InMemoryReservationStore Store;

        Reservation A = MakeOrder("u-buyer-1", "item-1", 1234);
        Store.Save(A);

        auto Found = Store.FindByID("res-1");
        Check(Found.has_value(), "FindByID: 存在的编号应查得到");
        Check(Found->BuyerID == "u-buyer-1", "FindByID: 买家应一致");
        Check(Found->ItemID == "item-1", "FindByID: 商品应一致");
        Check(Found->CreatedAtMs == 1234, "FindByID: 发起时间应一致");

        Check(!Store.FindByID("res-404").has_value(), "FindByID: 不存在的编号应查不到");
        Check(!Store.FindByID("").has_value(), "FindByID: 空编号应查不到");
    }

    // ================= 5. 按（买家 + 商品）查询 =================
    {
        InMemoryReservationStore Store;

        Reservation A = MakeOrder("u-buyer-1", "item-1");
        Store.Save(A);

        Check(Store.FindByBuyerAndItem("u-buyer-1", "item-1").has_value(),
              "FindByBuyerAndItem: 这一对存在时应查得到");
        Check(!Store.FindByBuyerAndItem("u-buyer-2", "item-1").has_value(),
              "FindByBuyerAndItem: 换买家应查不到");
        Check(!Store.FindByBuyerAndItem("u-buyer-1", "item-2").has_value(),
              "FindByBuyerAndItem: 换商品应查不到");
        Check(!Store.FindByBuyerAndItem("", "item-1").has_value(),
              "FindByBuyerAndItem: 空买家应查不到");
        Check(!Store.FindByBuyerAndItem("u-buyer-1", "").has_value(),
              "FindByBuyerAndItem: 空商品应查不到");

        // 同一个买家的第二条预约（不同商品）应各自独立
        Reservation C = MakeOrder("u-buyer-1", "item-3");
        Store.Save(C);
        auto Hit = Store.FindByBuyerAndItem("u-buyer-1", "item-3");
        Check(Hit.has_value() && Hit->ItemID == "item-3",
              "FindByBuyerAndItem: 同一买家的不同商品应分别命中");
    }

    // ================= 6. 按买家列列表 =================
    {
        InMemoryReservationStore Store;

        Store.Save(MakeOrder("u-buyer-1", "item-1"));
        Store.Save(MakeOrder("u-buyer-1", "item-2"));
        Store.Save(MakeOrder("u-buyer-1", "item-3"));
        Store.Save(MakeOrder("u-buyer-2", "item-1"));

        auto Mine = Store.ListByBuyer("u-buyer-1");
        Check(Mine.size() == 3, "ListByBuyer: 该买家应有 3 条");

        bool AllMine = true;
        for (const auto& Order : Mine)
        {
            if (Order.BuyerID != "u-buyer-1")
            {
                AllMine = false;
            }
        }
        Check(AllMine, "ListByBuyer: 返回的每条买家都应是本人");

        Check(Store.ListByBuyer("u-nobody").empty(), "ListByBuyer: 没有预约的买家应返回空");
        Check(Store.ListByBuyer("").empty(), "ListByBuyer: 空买家应返回空，而不是全部");
    }

    // ================= 7. 按商品列列表 =================
    {
        InMemoryReservationStore Store;

        Store.Save(MakeOrder("u-buyer-1", "item-1"));
        Store.Save(MakeOrder("u-buyer-2", "item-1"));
        Store.Save(MakeOrder("u-buyer-3", "item-1"));
        Store.Save(MakeOrder("u-buyer-1", "item-2"));

        auto Orders = Store.ListByItem("item-1");
        Check(Orders.size() == 3, "ListByItem: 该商品应有 3 条预约");

        bool AllSameItem = true;
        for (const auto& Order : Orders)
        {
            if (Order.ItemID != "item-1")
            {
                AllSameItem = false;
            }
        }
        Check(AllSameItem, "ListByItem: 返回的每条商品都应是该商品");

        Check(Store.ListByItem("item-9").empty(), "ListByItem: 没人约的商品应返回空");
        Check(Store.ListByItem("").empty(), "ListByItem: 空商品应返回空，而不是全部");
    }

    // ================= 8. 计数 =================
    {
        InMemoryReservationStore Store;

        Store.Save(MakeOrder("u-buyer-1", "item-1"));
        Store.Save(MakeOrder("u-buyer-1", "item-2"));
        Store.Save(MakeOrder("u-buyer-2", "item-1"));

        Check(Store.CountByBuyer("u-buyer-1") == 2, "CountByBuyer: 买家 1 应有 2 条");
        Check(Store.CountByBuyer("u-buyer-2") == 1, "CountByBuyer: 买家 2 应有 1 条");
        Check(Store.CountByBuyer("u-nobody") == 0, "CountByBuyer: 没有预约的买家应为 0");
        Check(Store.CountByItem("item-1") == 2, "CountByItem: 商品 1 应有 2 条");
        Check(Store.CountByItem("item-2") == 1, "CountByItem: 商品 2 应有 1 条");
        Check(Store.CountByItem("") == 0, "CountByItem: 空商品应为 0");
    }

    // ================= 9. 删除单条 =================
    {
        InMemoryReservationStore Store;

        Store.Save(MakeOrder("u-buyer-1", "item-1"));
        Check(Store.Size() == 1, "Remove: 删之前应有 1 条");

        Check(Store.Remove("res-1"), "Remove: 存在的编号应删除成功");
        Check(Store.Size() == 0, "Remove: 删之后应为 0 条");
        Check(!Store.Remove("res-1"), "Remove: 同一编号再删应返回 false");
        Check(!Store.FindByID("res-1").has_value(), "Remove: 删除后按编号应查不到");
    }

    // ================= 10. 按商品批量删除 =================
    {
        InMemoryReservationStore Store;

        Store.Save(MakeOrder("u-buyer-1", "item-1"));
        Store.Save(MakeOrder("u-buyer-2", "item-1"));
        Store.Save(MakeOrder("u-buyer-3", "item-1"));
        Store.Save(MakeOrder("u-buyer-1", "item-2"));

        Check(Store.RemoveByItem("item-1") == 3, "RemoveByItem: 应删掉 3 条并返回条数");
        Check(Store.ListByItem("item-1").empty(), "RemoveByItem: 该商品应已无预约");
        Check(Store.ListByItem("item-2").size() == 1, "RemoveByItem: 别的商品的预约不应受影响");
        Check(Store.Size() == 1, "RemoveByItem: 库中应只剩 1 条");
        Check(Store.RemoveByItem("item-1") == 0, "RemoveByItem: 再删一次应返回 0");
        Check(Store.RemoveByItem("") == 0, "RemoveByItem: 空商品应返回 0");
    }

    // ================= 11. 结果与库的隔离 =================
    {
        InMemoryReservationStore Store;

        Reservation A = MakeOrder("u-buyer-1", "item-1");
        Store.Save(A);

        // 存进去之后再改原对象，库里的副本不应跟着变
        A.BuyerID = "u-buyer-999";
        auto Found = Store.FindByID("res-1");
        Check(Found.has_value() && Found->BuyerID == "u-buyer-1",
              "隔离: 改原对象不应影响库里的记录");

        // 改查询结果，库里的记录也不应变
        auto Mine = Store.ListByBuyer("u-buyer-1");
        if (!Mine.empty())
        {
            Mine[0].BuyerID = "u-buyer-888";
        }
        auto Again = Store.FindByID("res-1");
        Check(Again.has_value() && Again->BuyerID == "u-buyer-1",
              "隔离: 改查询结果不应影响库里的记录");
    }

    // ================= 12. 初始状态 =================
    {
        InMemoryReservationStore Store;
        Check(Store.Size() == 0, "初始: 新建的库应为空");
        Check(Store.ListByBuyer("u-buyer-1").empty(), "初始: 空库按买家查应返回空");
    }
}
