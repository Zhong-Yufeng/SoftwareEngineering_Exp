#include"InMemoryOperationLogStore.h"
#include<algorithm>

namespace
{
    // 追查日志时新的在前，因此按时间倒序
    void SortByTimeDesc(std::vector<OperationLog>& Entries)
    {
        std::sort(Entries.begin(), Entries.end(),
                  [](const OperationLog& A, const OperationLog& B)
                  {
                      if (A.OperatedAtMs != B.OperatedAtMs)
                      {
                          return A.OperatedAtMs > B.OperatedAtMs;
                      }
                      return A.LogID > B.LogID;
                  });
    }

    bool MatchesKeyword(const OperationLog& Log, const std::string& Keyword)
    {
        return Log.OperatorID.find(Keyword) != std::string::npos
            || Log.ObjectType.find(Keyword) != std::string::npos
            || Log.ObjectID.find(Keyword) != std::string::npos
            || Log.Description.find(Keyword) != std::string::npos;
    }

    bool Matches(const OperationLog& Log, const LogQuery& Cond)
    {
        if (Cond.Type.has_value() && Log.Type != Cond.Type.value())
        {
            return false;
        }
        if (!Cond.OperatorID.empty() && Log.OperatorID != Cond.OperatorID)
        {
            return false;
        }
        if (!Cond.ObjectType.empty() && Log.ObjectType != Cond.ObjectType)
        {
            return false;
        }
        if (!Cond.ObjectID.empty() && Log.ObjectID != Cond.ObjectID)
        {
            return false;
        }
        if (Cond.FromMs > 0 && Log.OperatedAtMs < Cond.FromMs)
        {
            return false;
        }
        if (Cond.ToMs > 0 && Log.OperatedAtMs > Cond.ToMs)
        {
            return false;
        }
        if (!Cond.Keyword.empty() && !MatchesKeyword(Log, Cond.Keyword))
        {
            return false;
        }
        return true;
    }
}

bool InMemoryOperationLogStore::Save(OperationLog& Log)
{
    if (!Log.LogID.empty())
    {
        return false;
    }

    Log.LogID = MakeNextID();
    Logs.insert_or_assign(Log.LogID, Log);
    return true;
}

std::optional<OperationLog> InMemoryOperationLogStore::FindByID(const std::string& LogID) const
{
    const auto It = Logs.find(LogID);
    if (It == Logs.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::vector<OperationLog> InMemoryOperationLogStore::Query(const LogQuery& Cond) const
{
    std::vector<OperationLog> Result;
    if (Cond.FromMs > 0 && Cond.ToMs > 0 && Cond.FromMs > Cond.ToMs)
    {
        return Result;
    }

    for (const auto& Entry : Logs)
    {
        if (Matches(Entry.second, Cond))
        {
            Result.push_back(Entry.second);
        }
    }

    SortByTimeDesc(Result);
    return Result;
}

std::size_t InMemoryOperationLogStore::CountByType(LogType Type) const
{
    std::size_t Count = 0;
    for (const auto& Entry : Logs)
    {
        if (Entry.second.Type == Type)
        {
            ++Count;
        }
    }
    return Count;
}

std::size_t InMemoryOperationLogStore::Size() const
{
    return Logs.size();
}

std::string InMemoryOperationLogStore::MakeNextID()
{
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "log-" + std::to_string(LastAssignedNumber);
        if (Logs.find(Candidate) == Logs.end())
        {
            return Candidate;
        }
    }
}
