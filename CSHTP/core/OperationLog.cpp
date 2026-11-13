#include"OperationLog.h"

std::optional<OperationLog> OperationLog::Create(LogType Type,
                                                 const std::string& OperatorID,
                                                 const std::string& ObjectType,
                                                 const std::string& ObjectID,
                                                 const std::string& Description,
                                                 long long NowMs)
{
    if (OperatorID.empty() || ObjectType.empty() || ObjectID.empty() || NowMs <= 0)
    {
        return std::nullopt;
    }

    OperationLog Log;
    Log.LogID = "";
    Log.Type = Type;
    Log.OperatedAtMs = NowMs;
    Log.OperatorID = OperatorID;
    Log.ObjectType = ObjectType;
    Log.ObjectID = ObjectID;
    Log.Description = Description;
    return Log;
}
