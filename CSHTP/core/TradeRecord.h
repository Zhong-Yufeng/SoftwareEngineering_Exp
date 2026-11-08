#pragma once
#include<string>
#include<optional>
#include"Item.h"
#include"enums.h"

class TradeRecord
{
    public:
        // 成交记录类的变量
        std::string TradeID;          // 由存储层分配
        std::string ItemID;           // 已删除商品的编号，仅用于追溯
        std::string BuyerID;
        std::string SellerID;
        long long TradedAtMs;         // 成交时间

        std::string ItemTitle;        // 商品标题
        long long PriceCents;         // 成交价，单位分
        std::string TradeLocation;    // 交易地点
        std::string SellerContact;    // 卖家联系方式
        std::string BuyerContact;     // 买家联系方式

        static std::optional<TradeRecord> Create(const Item& Goods,
                                                 const std::string& BuyerID,
                                                 const std::string& BuyerContact,
                                                 long long TradedAtMs);
        // 成交记录类的构造函数

        // 这条记录是否与某人有关（买家或卖家）。
        // 用于「我成交了哪些」「我卖出了哪些」这类查询的权限过滤。
        bool IsParticipant(const std::string& UserID) const;

    private:
        // 默认构造函数私有：成交记录只能由 Create 造出来。
        // 拷贝构造仍由编译器隐式生成且公开，这是有意保留的：
        // 存储层要把记录拷进容器，查询也要返回拷贝。
        TradeRecord() = default;
};
