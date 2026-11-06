#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"AuditRecordStore.h"

// ============================================================
// InMemoryAuditRecordStore —— 把审核记录放在哈希表里
//
// 和别的内存存储的差别：这里所有列表查询都要排序。
// 因为哈希表的遍历顺序是不可预测的，而「按时间排好的审核历史」
// 是这个表唯一的阅读方式——不排序，界面上就会出现乱序的审核流水。
// ============================================================
class InMemoryAuditRecordStore : public AuditRecordStore
{
    public:
        bool Save(AuditRecord& Record) override;
        std::optional<AuditRecord> FindByID(const std::string& RecordID) const override;
        std::vector<AuditRecord> ListByItem(const std::string& ItemID) const override;
        std::vector<AuditRecord> ListByReviewer(const std::string& ReviewerID) const override;
        std::optional<AuditRecord> FindLatestByItem(const std::string& ItemID) const override;
        std::vector<AuditRecord> All() const override;
        std::size_t Size() const override;

    private:
        std::string MakeNextID();

        std::unordered_map<std::string, AuditRecord> Records;
        long long LastAssignedNumber = 0;
};
