#pragma once
#include<optional>
#include<string>
#include"enums.h"

// ============================================================
// AuditRecord —— 一次审核行为的留痕
//
// 它和 Item 的分工：
//   Item 记「现在是什么状态」；
//   AuditRecord 记「谁在什么时候把它改成了这个状态」。
//
// 为什么需要它：商品被驳回后，卖家会问「为什么」；被误驳回了，
// 平台要查「谁审的、什么时候审的」。这些信息商品自己存不下——
// 一件商品会被审很多次，每次的结论都要单独留一条。
//
// 它没有状态机，也没有跨对象约束，所以只需要一个工厂函数。
// ============================================================
class AuditRecord
{
    public:
        // 审核记录类的变量
        std::string RecordID;
        std::string ItemID;
        std::string ReviewerID;
        AuditResult Result;
        long long AuditedAtMs;              // 审核时间
        std::optional<std::string> Reason;
        // 审核记录类的变量

        // 造一条审核记录。编号留空，由存储层分配。
        // 校验的是「这条记录本身是否自洽」，不查商品在不在——
        // 那是服务层的事（它手里才有商品）。
        static std::optional<AuditRecord> Create(const std::string& ItemID,
                                                 const std::string& ReviewerID,
                                                 AuditResult Result,
                                                 long long AuditedAtMs,
                                                 const std::string& Reason);

    private:
        // 默认构造函数私有：审核记录只能由 Create 造出来。
        // 拷贝构造仍由编译器隐式生成且公开，这是有意保留的：
        // 存储层要把记录拷进容器，查询也要返回拷贝。
        AuditRecord() = default;
};
