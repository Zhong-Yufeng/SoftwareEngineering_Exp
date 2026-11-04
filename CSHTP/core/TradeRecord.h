#pragma once
#include<string>
#include<optional>
#include"Item.h"
#include"enums.h"

// ============================================================
// TradeRecord —— 成交记录
//
// 【为什么需要这个类】
//   成交后商品记录被删除，买卖双方手里就什么都不剩了：
//   买家不知道跟谁约的、在哪儿交接；卖家也不知道卖给了谁。
//   而用户之间平时不能互查联系方式（隐私保护，REQ 里的明确规定），
//   所以这条「成交那一刻的信息」必须单独存下来，成为双方唯一的信息来源。
//
// 【为什么存快照，而不是只存 ItemID / UserID】
//   这是一份「凭证」，记录的是成交那一刻的样子。如果只存 ID，
//   卖家之后改绑手机号、改昵称，买家再打开这条记录看到的就是新信息，
//   历史成交会被后续修改污染。所以标题、价格、地点、双方联系方式
//   全部以「值」的形式拷一份。代价是冗余，换来的是历史不可篡改。
//
// 【为什么它不负责权限】
//   它只回答「这条记录里有没有你」——IsParticipant。
//   「谁可以列出某人的成交记录」属于查询编排，由服务层调用它来过滤。
//
// 【ItemID 指向已删除的商品，这是预期行为】
//   商品被删了，这个字段就是一个历史编号，用来把同一次成交的日志串起来。
//   真正给人看的信息都在下面的快照字段里。
// ============================================================
class TradeRecord
{
    public:
        // 成交记录类的变量
        std::string TradeID;          // 由存储层分配
        std::string ItemID;           // 已删除商品的编号，仅用于追溯
        std::string BuyerID;
        std::string SellerID;
        long long TradedAtMs;         // 成交时间

        // —— 以下为成交那一刻的快照，不随之后的对象修改而变化 ——
        std::string ItemTitle;        // 商品标题
        long long PriceCents;         // 成交价，单位分
        std::string TradeLocation;    // 交易地点
        std::string SellerContact;    // 卖家联系方式
        std::string BuyerContact;     // 买家联系方式
        // 成交记录类的变量

        // 成交记录类的构造函数
        // 直接拿「即将被删除的那件商品」当快照来源，而不是让调用方传 10 个参数：
        // 参数越少，越不容易把标题拷成描述、把卖家联系方式拷成买家的。
        // 校验：商品编号与卖家编号非空、商品已发布（未成交）、
        //       买家编号非空且不等于卖家（不能自己买自己的东西）、
        //       价格不为负、成交时间为正值。不合法返回 std::nullopt。
        static std::optional<TradeRecord> Create(const Item& Goods,
                                                 const std::string& BuyerID,
                                                 const std::string& BuyerContact,
                                                 long long TradedAtMs);
        // 成交记录类的构造函数

        // 这条记录是否与某人有关（买家或卖家）。
        // 用于「我成交了哪些」「我卖出了哪些」这类查询的权限过滤。
        bool IsParticipant(const std::string& UserID) const;
};
