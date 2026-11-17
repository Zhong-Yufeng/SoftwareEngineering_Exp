#pragma once
#include<cstddef>
#include<optional>
#include<string>
#include<vector>
#include"ServiceCommon.h"
#include"OperationLogStore.h"

class LogService
{
    public:
        LogService(OperationLogStore& LogStore, ServiceClock Clock = SystemTimeMs);

        // ==================== 系统内部（各服务写日志用，不对外暴露） ====================

        bool Record(LogType Type,
                    const std::string& OperatorID,
                    const std::string& ObjectType,
                    const std::string& ObjectID,
                    const std::string& Description,
                    std::string* Error = nullptr);

        long long NowMs() const;

        // ==================== 公共 ====================

        std::optional<OperationLog> FindByID(const std::string& LogID) const;

        // ==================== 管理员查询（REQ-021） ====================
        // 本类不做身份校验：日志只允许管理员查询，入口是 AdminService::QueryLogs，
        // 那里先校验管理员身份，再调用这里。买家与卖家没有日志查询入口。

        std::vector<OperationLog> Query(const LogQuery& Cond) const;

        std::size_t CountByType(LogType Type) const;

    private:
        OperationLogStore& Logs;
        ServiceClock Clock;
};
