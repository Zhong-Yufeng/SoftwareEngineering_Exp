#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"AuditRecordStore.h"

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
