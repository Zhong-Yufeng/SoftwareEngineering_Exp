#include"InMemoryOperationLogStore.h"
#include<algorithm>

namespace
{
    // 按操作时间升序。时间相同时用编号兜底：哈希表的遍历顺序不保证
    // 稳定，不兜底的话「同一毫秒发生的两条」在测试里会飘。
    void SortByTime(std::vector<OperationLog>& Entries)
    {
        std::sort(Entries.begin(), Entries.end(),
                  [](const OperationLog& A, const OperationLog& B)
                  {
                      if (A.OperatedAtMs != B.OperatedAtMs)
                      {
                          return A.OperatedAtMs < B.OperatedAtMs;
                      }
                      return A.LogID < B.LogID;
                  });
    }

    // 时间范围两端都包含
    bool InRange(long long AtMs, long long FromMs, long long ToMs)
    {
        return AtMs >= FromMs && AtMs <= ToMs;
    }
}

bool InMemoryOperationLogStore::Save(OperationLog& Log)
{
    // 编号非空说明这条日志已经有主，日志不可改写，一律拒绝
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
    // 空编号在表里查不到，也正好是「找不到」的信号
    auto It = Logs.find(LogID);
    if (It == Logs.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::vector<OperationLog> InMemoryOperationLogStore::ListByType(LogType Type) const
{
    std::vector<OperationLog> Result;
    for (const auto& Entry : Logs)
    {
        if (Entry.second.Type == Type)
        {
            Result.push_back(Entry.second);
        }
    }

    SortByTime(Result);
    return Result;
}

std::vector<OperationLog> InMemoryOperationLogStore::ListByTimeRange(long long FromMs, long long ToMs) const
{
    std::vector<OperationLog> Result;
    if (FromMs > ToMs)
    {
        return Result;
    }

    for (const auto& Entry : Logs)
    {
        if (InRange(Entry.second.OperatedAtMs, FromMs, ToMs))
        {
            Result.push_back(Entry.second);
        }
    }

    SortByTime(Result);
    return Result;
}

std::vector<OperationLog> InMemoryOperationLogStore::SearchByKeyword(const std::string& Keyword) const
{
    std::vector<OperationLog> Result;

    // 空关键词不是「查全部」：列表查询一律不接受通配
    if (Keyword.empty())
    {
        return Result;
    }

    for (const auto& Entry : Logs)
    {
        if (Entry.second.Content.find(Keyword) != std::string::npos)
        {
            Result.push_back(Entry.second);
        }
    }

    SortByTime(Result);
    return Result;
}

std::vector<OperationLog> InMemoryOperationLogStore::All() const
{
    std::vector<OperationLog> Result;
    Result.reserve(Logs.size());
    for (const auto& Entry : Logs)
    {
        Result.push_back(Entry.second);
    }

    SortByTime(Result);
    return Result;
}

std::size_t InMemoryOperationLogStore::Size() const
{
    return Logs.size();
}

std::string InMemoryOperationLogStore::MakeNextID()
{
    // 编号一旦被占用就继续往后找，避免删改后重号
    std::string ID;
    do
    {
        ++LastAssignedNumber;
        ID = "log-" + std::to_string(LastAssignedNumber);
    } while (Logs.find(ID) != Logs.end());
    return ID;
}
