#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"OperationLogStore.h"

class SqliteStatement;

// OperationLogStore 的数据库实现：日志只增、不改、不删（REQ-021）
class SqliteOperationLogStore : public OperationLogStore
{
    public:
        explicit SqliteOperationLogStore(SqliteDatabase& Db);

        bool Save(OperationLog& Log) override;
        std::optional<OperationLog> FindByID(const std::string& LogID) const override;
        std::vector<OperationLog> Query(const LogQuery& Cond) const override;
        std::size_t CountByType(LogType Type) const override;
        std::size_t Size() const override;

    private:
        static OperationLog Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
