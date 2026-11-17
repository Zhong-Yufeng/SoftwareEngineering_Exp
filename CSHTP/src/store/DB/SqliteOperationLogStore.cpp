#include"SqliteOperationLogStore.h"
#include"SqliteStatement.h"
#include<functional>

namespace
{
    const char* const SelectColumns =
        "SELECT LogID, Type, OperatedAtMs, OperatorID, ObjectType, ObjectID, Description, Deleted "
        "FROM operation_logs ";

    // 追查日志时新的在前：时间倒序，同时间按编号倒序
    const char* const OrderByTimeDesc = " ORDER BY OperatedAtMs DESC, LogID DESC;";
}

SqliteOperationLogStore::SqliteOperationLogStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteOperationLogStore::Save(OperationLog& Log)
{
    // 日志编号必须为空：只由存储层分配
    if (!Log.LogID.empty())
    {
        return false;
    }

    if (!Database.NextID("log", "log-", Log.LogID))
    {
        return false;
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO operation_logs(LogID, Type, OperatedAtMs, OperatorID, "
                           "ObjectType, ObjectID, Description, Deleted) "
                           "VALUES(?, ?, ?, ?, ?, ?, ?, ?);");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Log.LogID);
    Insert.BindInt64(2, static_cast<long long>(Log.Type));
    Insert.BindInt64(3, Log.OperatedAtMs);
    Insert.BindText(4, Log.OperatorID);
    Insert.BindText(5, Log.ObjectType);
    Insert.BindText(6, Log.ObjectID);
    Insert.BindText(7, Log.Description);
    Insert.BindText(8, Log.Deleted);

    if (Insert.Step() || Insert.HasError())
    {
        return false;
    }
    return true;
}

std::optional<OperationLog> SqliteOperationLogStore::FindByID(const std::string& LogID) const
{
    if (LogID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE LogID = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, LogID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::vector<OperationLog> SqliteOperationLogStore::Query(const LogQuery& Cond) const
{
    std::vector<OperationLog> Result;

    std::string Sql = SelectColumns;
    std::vector<std::string> Where;
    std::vector<std::function<void(SqliteStatement&, int)>> Binds;

    if (Cond.Type.has_value())
    {
        const long long Value = static_cast<long long>(Cond.Type.value());
        Where.push_back("Type = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (!Cond.OperatorID.empty())
    {
        const std::string Value = Cond.OperatorID;
        Where.push_back("OperatorID = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Value);
                        });
    }

    if (!Cond.ObjectType.empty())
    {
        const std::string Value = Cond.ObjectType;
        Where.push_back("ObjectType = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Value);
                        });
    }

    if (!Cond.ObjectID.empty())
    {
        const std::string Value = Cond.ObjectID;
        Where.push_back("ObjectID = ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindText(Index, Value);
                        });
    }

    if (Cond.FromMs > 0)
    {
        const long long Value = Cond.FromMs;
        Where.push_back("OperatedAtMs >= ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (Cond.ToMs > 0)
    {
        const long long Value = Cond.ToMs;
        Where.push_back("OperatedAtMs <= ?");
        Binds.push_back([Value](SqliteStatement& Statement, int Index)
                        {
                            Statement.BindInt64(Index, Value);
                        });
    }

    if (!Cond.Keyword.empty())
    {
        // 用 instr 做区分大小写的子串匹配，与内存版的 std::string::find 保持一致
        Where.push_back("(instr(OperatorID, ?) > 0 OR instr(ObjectType, ?) > 0 "
                        "OR instr(ObjectID, ?) > 0 OR instr(Description, ?) > 0)");
        const std::string Keyword = Cond.Keyword;
        for (int Repeat = 0; Repeat < 4; ++Repeat)
        {
            Binds.push_back([Keyword](SqliteStatement& Statement, int Index)
                            {
                                Statement.BindText(Index, Keyword);
                            });
        }
    }

    for (std::size_t Index = 0; Index < Where.size(); ++Index)
    {
        Sql += (Index == 0 ? "WHERE " : " AND ");
        Sql += Where[Index];
    }
    Sql += OrderByTimeDesc;

    // 时间窗颠倒时与内存版一致：直接返回空
    if (Cond.FromMs > 0 && Cond.ToMs > 0 && Cond.FromMs > Cond.ToMs)
    {
        return Result;
    }

    SqliteStatement QueryStatement(Database, Sql);
    if (!QueryStatement.IsReady())
    {
        return Result;
    }

    int BindIndex = 1;
    for (const auto& Binder : Binds)
    {
        Binder(QueryStatement, BindIndex);
        ++BindIndex;
    }

    while (QueryStatement.Step())
    {
        Result.push_back(Load(QueryStatement));
    }
    return Result;
}

std::size_t SqliteOperationLogStore::CountByType(LogType Type) const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM operation_logs WHERE Type = ?;");
    if (!Query.IsReady())
    {
        return 0;
    }

    Query.BindInt64(1, static_cast<long long>(Type));
    if (!Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

std::size_t SqliteOperationLogStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM operation_logs;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

OperationLog SqliteOperationLogStore::Load(SqliteStatement& Row)
{
    OperationLog Log;
    Log.LogID = Row.Text(0);
    Log.Type = static_cast<LogType>(Row.Int64(1));
    Log.OperatedAtMs = Row.Int64(2);
    Log.OperatorID = Row.Text(3);
    Log.ObjectType = Row.Text(4);
    Log.ObjectID = Row.Text(5);
    Log.Description = Row.Text(6);
    Log.Deleted = Row.Text(7);
    return Log;
}
