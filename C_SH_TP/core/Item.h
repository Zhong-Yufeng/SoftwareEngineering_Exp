#pragma once

#include <string>
#include <vector>

#include "enums.h"

// ============================================================
// Item —— 商品（UML 类图：商品 <<实体>>）
//
// 设计定位：纯数据盒子。字段全公开，不含任何业务流程方法
// （提交审核、下架、违规下架、完成交易这些"主动行为"一律不在本类）。
//
// 本类只保留一个创建入口 createDraft：它是静态工厂，不依赖 this、
// 不需要已存在的对象，作用只是把入参装配成一个初始商品，不做查询、
// 不写数据库、不做状态迁移。
//
// 状态迁移的合法性由操作者校验，见 User 中：
//   提交审核 / 下架 / 确认预约 / 审核商品 / 违规下架
//
// 关联（外键）：
//   用户 1 -- 0..* 商品     : sellerId
//   商品 1 -- 0..* 预约     : Reservation::itemId
//   商品 1 -- 0..* 审核记录 : AuditRecord::itemId
//
// 类图约束（OCL，本类不校验，由操作方保证）：
//   当前状态=已确认   => 存在且仅存在一条 已成功 预约（REQ-012 / REQ-023）
//   当前状态=交易完成 => 不可再发起或确认预约（REQ-029）
//
// 两个类型选择及理由：
//   priceCents    用 long long（单位：分），金额不用二进制浮点，避免误差
//   publishedAtMs 用 long long 毫秒时间戳，便于比较、排序、存库
//
// 用 struct 而不是 class：它只有数据，public 是它的本意。
// ============================================================
struct Item {
    std::string itemId;                          // 商品ID {PK}
    std::string title;                           // 标题
    std::string description;                     // 描述
    Category category = Category::Textbook;      // 分类
    long long priceCents = 0;                    // 价格（单位：分）
    Condition condition = Condition::LikeNew;    // 成色
    std::vector<std::string> images;             // 图片 {URL, 1..*}
    std::string tradeMethod;                     // 交易方式
    std::string tradeLocation;                   // 交易地点
    std::string contact;                         // 联系方式 {默认取卖家联系方式}
    ItemStatus status = ItemStatus::Draft;       // 当前状态
    long long publishedAtMs = 0;                 // 发布时间 {系统维护}
    std::string sellerId;                        // 发布者ID（用户1--0..*商品 导出）

    // ---------------- 创建入口 ----------------
    // 静态工厂：没有 this，不要求对象已存在，只负责"把参数装进结构体"。
    // 与业务流程方法的区别：不查库、不写库、不校验跨对象规则（例如
    // "该商品是否已有预约"），这些仍由 User 侧的用例承担。
    // itemId / publishedAtMs 不在此生成：ID 归存储层，发布时间在提交审核时写。
    static Item createDraft(const std::string& title,
                            const std::string& description,
                            Category category,
                            long long priceCents,
                            Condition condition,
                            const std::string& tradeMethod,
                            const std::string& tradeLocation,
                            const std::string& sellerId,
                            const std::string& contact);
};
