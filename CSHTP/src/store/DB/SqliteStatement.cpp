#include"SqliteStatement.h"
#include"SqliteDatabase.h"

SqliteStatement::SqliteStatement(SqliteDatabase& Db, const std::string& Sql)
    : Database(&Db)
{
    if (!Db.IsOpen())
    {
        Fail("数据库尚未打开");
        return;
    }

    const int Result = sqlite3_prepare_v2(Db.Handle(), Sql.c_str(), -1, &Statement, nullptr);
    if (Result != SQLITE_OK)
    {
        Fail(sqlite3_errmsg(Db.Handle()));
        Statement = nullptr;
    }
}

SqliteStatement::~SqliteStatement()
{
    if (Statement != nullptr)
    {
        sqlite3_finalize(Statement);
    }
}

bool SqliteStatement::IsReady() const
{
    return Statement != nullptr && !Error;
}

bool SqliteStatement::HasError() const
{
    return Error;
}

void SqliteStatement::BindText(int Index, const std::string& Value)
{
    if (Statement == nullptr)
    {
        return;
    }

    if (sqlite3_bind_text(Statement, Index, Value.c_str(), static_cast<int>(Value.size()),
                          SQLITE_TRANSIENT) != SQLITE_OK)
    {
        Fail(sqlite3_errmsg(Database->Handle()));
    }
}

void SqliteStatement::BindInt64(int Index, long long Value)
{
    if (Statement == nullptr)
    {
        return;
    }

    if (sqlite3_bind_int64(Statement, Index, static_cast<sqlite3_int64>(Value)) != SQLITE_OK)
    {
        Fail(sqlite3_errmsg(Database->Handle()));
    }
}

void SqliteStatement::BindNull(int Index)
{
    if (Statement == nullptr)
    {
        return;
    }

    if (sqlite3_bind_null(Statement, Index) != SQLITE_OK)
    {
        Fail(sqlite3_errmsg(Database->Handle()));
    }
}

bool SqliteStatement::Step()
{
    if (Statement == nullptr)
    {
        return false;
    }

    const int Result = sqlite3_step(Statement);
    if (Result == SQLITE_ROW)
    {
        return true;
    }
    if (Result == SQLITE_DONE)
    {
        return false;
    }

    Fail(sqlite3_errmsg(Database->Handle()));
    return false;
}

long long SqliteStatement::Int64(int Column) const
{
    if (Statement == nullptr)
    {
        return 0;
    }
    return static_cast<long long>(sqlite3_column_int64(Statement, Column));
}

std::string SqliteStatement::Text(int Column) const
{
    if (Statement == nullptr)
    {
        return std::string();
    }

    const unsigned char* Value = sqlite3_column_text(Statement, Column);
    if (Value == nullptr)
    {
        return std::string();
    }
    return std::string(reinterpret_cast<const char*>(Value));
}

bool SqliteStatement::IsNull(int Column) const
{
    if (Statement == nullptr)
    {
        return true;
    }
    return sqlite3_column_type(Statement, Column) == SQLITE_NULL;
}

void SqliteStatement::Fail(const std::string& Message)
{
    Error = true;
    if (Database != nullptr)
    {
        Database->SetError(Message);
    }
}
