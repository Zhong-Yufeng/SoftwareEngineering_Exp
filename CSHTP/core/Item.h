#pragma once
#include<optional>
#include<vector>
#include<string>
#include"enums.h"

// ============================================================
// Item —— 商品
//
// 创建分两半：
//   Item::CreateDraft  只挡「必然错误」，装配出一个草稿对象
//   ItemStore::Save    负责分配 ItemID 并入库
// 所以 CreateDraft 不产生 ID；草稿阶段描述、图片、交易方式允许留空。
// ============================================================
class Item
{
    public:
        // 商品类的变量
        std::string ItemID;                 // 草稿阶段为空，由存储层分配
        std::string Title;
        std::string Description;
        Category ItemCategory;
        long long PriceCents;               // 价格，单位分
        Condition ItemCondition;
        std::vector<std::string> Images;    // 图片，提交审核时才要求至少一张
        std::string TradeMethod;
        std::string TradeLocation;
        std::string Contact;
        ItemStatus Status;                  // 草稿阶段固定为 Draft
        long long PublishedAtMs;            // 提交审核时才写，草稿为 0
        std::string SellerID;
        // 商品类的变量

        // 商品类的构造函数
        // 校验：标题非空、卖家非空、价格不为负；其余允许留空（草稿）。
        // 不合法返回 std::nullopt。
        static std::optional<Item> CreateDraft(const std::string& Title,
                                               const std::string& Description,
                                               Category ItemCategory,
                                               long long PriceCents,
                                               Condition ItemCondition,
                                               const std::string& TradeMethod,
                                               const std::string& TradeLocation,
                                               const std::string& Contact,
                                               const std::string& SellerID);
        // 商品类的构造函数
};
