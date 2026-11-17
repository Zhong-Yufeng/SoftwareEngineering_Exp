#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"AuditRecordStore.h"

class SqliteStatement;

// AuditRecordStore 的数据库实现：审核记录只增不删
class SqliteAuditRecordStore : public AuditRecordStore
{
    public:
        explicit SqliteAuditRecordStore(SqliteDatabase& Db);

        bool Save(AuditRecord& Record) override;
        std::optional<AuditRecord> FindByID(const std::string& RecordID) const override;
        std::vector<AuditRecord> ListByItem(const std::string& ItemID) const override;
        std::vector<AuditRecord> ListByReviewer(const std::string& ReviewerID) const override;
        std::optional<AuditRecord> FindLatestByItem(const std::string& ItemID) const override;
        std::vector<AuditRecord> All() const override;
        std::size_t Size() const override;

    private:
        static AuditRecord Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
