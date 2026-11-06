#pragma once
#include<optional>
#include<string>
#include<vector>
#include<cstddef>
#include"AuditRecord.h"

// ============================================================
// AuditRecordStore —— 审核记录的存储接口
//
// 三条设计原则：
//   1. 只增不删。审核记录是「留痕」，能删就等于没留。
//      所以这里没有 Remove，也没有「按商品清空」。
//   2. 查询按商品和按审核人两个方向。前者回答「这件商品被审过几次」，
//      后者回答「这个管理员审了些什么」，是排查误审的两个入口。
//   3. 列表按审核时间升序返回，最后一条就是最近一次。
//      内存版要显式排序；换成 SQL 就是 ORDER BY audited_at_ms。
// ============================================================
class AuditRecordStore
{
    public:
        virtual ~AuditRecordStore() = default;

        // 存一条。编号为空则分配，有编号则覆盖（覆盖只用于测试装置）
        virtual bool Save(AuditRecord& Record) = 0;

        virtual std::optional<AuditRecord> FindByID(const std::string& RecordID) const = 0;

        // 某件商品的全部审核记录，按时间升序
        virtual std::vector<AuditRecord> ListByItem(const std::string& ItemID) const = 0;

        // 某个审核人的全部审核记录，按时间升序
        virtual std::vector<AuditRecord> ListByReviewer(const std::string& ReviewerID) const = 0;

        // 某件商品最近一次审核
        virtual std::optional<AuditRecord> FindLatestByItem(const std::string& ItemID) const = 0;

        // 全部记录，按时间升序（平台统计与管理员翻查用）
        virtual std::vector<AuditRecord> All() const = 0;

        virtual std::size_t Size() const = 0;
};
