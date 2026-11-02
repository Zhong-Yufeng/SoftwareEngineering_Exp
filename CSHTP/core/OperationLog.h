#pragma once
#include<optional>
#include<string>
#include<vector>
#include"enums.h"

class OperationLog
{
    public:
        // 操作日志类的变量
        std::string LogID;
        std::string OperatorID;
        long long OperatedAtMs;             // 操作时间
        LogType Type;
        std::string ObjectType;
        std::string ObjectID;
        std::optional<std::string> Description;
        // 操作日志类的变量

        // 操作日志类的方法
        bool Query(LogType Type,
                   long long StartMs,
                   long long EndMs,
                   std::vector<std::string>& OutLogIDs);
        // 操作日志类的方法
};
