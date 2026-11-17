#include"LogService.h"

LogService::LogService(OperationLogStore& LogStore, ServiceClock ClockSource)
    : Logs(LogStore), Clock(ClockSource)
{
}

bool LogService::Record(LogType Type,
                        const std::string& OperatorID,
                        const std::string& ObjectType,
                        const std::string& ObjectID,
                        const std::string& Description,
                        std::string* Error)
{
    ClearError(Error);

    const auto Log = OperationLog::Create(Type, OperatorID, ObjectType, ObjectID,
                                          Description, Clock());
    if (!Log.has_value())
    {
        SetError(Error, "日志参数不完整");
        return false;
    }

    OperationLog Saved = Log.value();
    if (!Logs.Save(Saved))
    {
        SetError(Error, "日志写入失败");
        return false;
    }
    return true;
}

std::optional<OperationLog> LogService::FindByID(const std::string& LogID) const
{
    return Logs.FindByID(LogID);
}

std::vector<OperationLog> LogService::Query(const LogQuery& Cond) const
{
    return Logs.Query(Cond);
}

std::size_t LogService::CountByType(LogType Type) const
{
    return Logs.CountByType(Type);
}

long long LogService::NowMs() const
{
    return Clock();
}
