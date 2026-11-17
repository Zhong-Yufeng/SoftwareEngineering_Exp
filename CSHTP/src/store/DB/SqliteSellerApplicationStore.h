#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"SellerApplicationStore.h"

class SqliteStatement;

// SellerApplicationStore 的数据库实现：一个申请人最多一条记录，只增不删
class SqliteSellerApplicationStore : public SellerApplicationStore
{
    public:
        explicit SqliteSellerApplicationStore(SqliteDatabase& Db);

        bool Save(SellerApplication& Application) override;
        std::optional<SellerApplication> FindByID(const std::string& ApplicationID) const override;
        std::optional<SellerApplication> FindByApplicant(const std::string& ApplicantID) const override;
        std::optional<SellerApplication> FindPendingByApplicant(const std::string& ApplicantID) const override;
        bool HasApplication(const std::string& ApplicantID) const override;
        std::vector<SellerApplication> ListByStatus(ApplicationStatus Status) const override;
        std::vector<SellerApplication> All() const override;
        std::size_t Size() const override;

    private:
        static SellerApplication Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
