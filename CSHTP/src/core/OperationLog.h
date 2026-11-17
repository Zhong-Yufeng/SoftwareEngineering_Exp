#pragma once
#include<optional>
#include<string>
#include"enums.h"

class SqliteOperationLogStore;

// 操作日志：只创建、不修改、不删除（REQ-021）
class OperationLog
{
    public:
        std::string LogID;              // 由存储层分配
        LogType Type = LogType::Register;
        long long OperatedAtMs = 0;
        std::string OperatorID;         // 操作人 UserID
        std::string ObjectType;         // 如 "User"、"Item"、"Reservation"
        std::string ObjectID;
        std::string Description;        // 可空
        std::string Deleted;            // 可空

        static std::optional<OperationLog> Create(LogType Type,
                                                  const std::string& OperatorID,
                                                  const std::string& ObjectType,
                                                  const std::string& ObjectID,
                                                  const std::string& Description,
                                                  long long NowMs);

    private:
        friend class SqliteOperationLogStore;
        OperationLog() = default;
};
