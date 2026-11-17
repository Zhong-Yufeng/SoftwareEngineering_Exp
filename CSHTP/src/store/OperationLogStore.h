#pragma once
#include<optional>
#include<vector>
#include<string>
#include<cstddef>
#include"OperationLog.h"

// REQ-021 日志查询条件：按类型、操作人、操作对象、时间窗组合过滤
// 空串与 0 一律表示「该维度不限」，因此默认构造的 LogQuery 等价于查询全部日志
struct LogQuery
{
    std::optional<LogType> Type;
    std::string OperatorID;
    std::string ObjectType;
    std::string ObjectID;
    long long FromMs = 0;
    long long ToMs = 0;
    std::string Keyword;   // 匹配操作人、对象编号与描述
};

// 日志只增、不改、不删
class OperationLogStore
{
    public:
        virtual ~OperationLogStore() = default;

        // 编号必须为空，由存储层分配
        virtual bool Save(OperationLog& Log) = 0;
        virtual std::optional<OperationLog> FindByID(const std::string& LogID) const = 0;

        // 结果按操作时间倒序，同时间按编号倒序
        virtual std::vector<OperationLog> Query(const LogQuery& Cond) const = 0;

        virtual std::size_t CountByType(LogType Type) const = 0;
        virtual std::size_t Size() const = 0;
};
