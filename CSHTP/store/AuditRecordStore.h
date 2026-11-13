#pragma once
#include<optional>
#include<vector>
#include<string>
#include<cstddef>
#include"AuditRecord.h"

// 审核记录只增不删
class AuditRecordStore
{
    public:
        virtual ~AuditRecordStore() = default;

        virtual bool Save(AuditRecord& Record) = 0;
        virtual std::optional<AuditRecord> FindByID(const std::string& RecordID) const = 0;
        virtual std::vector<AuditRecord> ListByItem(const std::string& ItemID) const = 0;
        virtual std::vector<AuditRecord> ListByReviewer(const std::string& ReviewerID) const = 0;
        virtual std::optional<AuditRecord> FindLatestByItem(const std::string& ItemID) const = 0;
        virtual std::vector<AuditRecord> All() const = 0;
        virtual std::size_t Size() const = 0;
};
