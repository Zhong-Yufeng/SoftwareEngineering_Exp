#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstddef>
#include"OperationLogStore.h"

// 日志只增、不改、不删
class InMemoryOperationLogStore : public OperationLogStore
{
    public:
        bool Save(OperationLog& Log) override;
        std::optional<OperationLog> FindByID(const std::string& LogID) const override;
        std::vector<OperationLog> Query(const LogQuery& Cond) const override;
        std::size_t CountByType(LogType Type) const override;
        std::size_t Size() const override;

    private:
        std::string MakeNextID();

        std::unordered_map<std::string, OperationLog> Logs;
        long long LastAssignedNumber = 0;
};
