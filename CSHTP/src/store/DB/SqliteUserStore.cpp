#include"SqliteUserStore.h"
#include"SqliteStatement.h"

namespace
{
    const char* const SelectColumns =
        "SELECT UserID, UserPhone, UserStdNo, Password, Nickname, QQNo, Status, Reason, UserRole "
        "FROM users ";
}

SqliteUserStore::SqliteUserStore(SqliteDatabase& Db)
    : Database(Db)
{
}

bool SqliteUserStore::Save(User& Data)
{
    // 一个账号只能对应一个用户：手机号与学号都不能指向另一个已有用户
    if (!Data.UserPhone.empty())
    {
        const auto Existing = FindByPhone(Data.UserPhone);
        if (Existing.has_value() && Existing->UserID != Data.UserID)
        {
            Database.SetError("手机号已被其他账号使用");
            return false;
        }
    }
    if (!Data.UserStdNo.empty())
    {
        const auto Existing = FindByStdNo(Data.UserStdNo);
        if (Existing.has_value() && Existing->UserID != Data.UserID)
        {
            Database.SetError("学号已被其他账号使用");
            return false;
        }
    }

    // 编号由存储层分配，调用方不给编号
    if (Data.UserID.empty())
    {
        if (!Database.NextID("user", "user-", Data.UserID))
        {
            return false;
        }
    }

    SqliteStatement Insert(Database,
                           "INSERT INTO users(UserID, UserPhone, UserStdNo, Password, Nickname, QQNo, "
                           "Status, Reason, UserRole) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT(UserID) DO UPDATE SET "
                           "UserPhone = excluded.UserPhone, UserStdNo = excluded.UserStdNo, "
                           "Password = excluded.Password, Nickname = excluded.Nickname, "
                           "QQNo = excluded.QQNo, Status = excluded.Status, "
                           "Reason = excluded.Reason, UserRole = excluded.UserRole;");
    if (!Insert.IsReady())
    {
        return false;
    }

    Insert.BindText(1, Data.UserID);
    Insert.BindText(2, Data.UserPhone);
    Insert.BindText(3, Data.UserStdNo);
    Insert.BindText(4, Data.Password);
    Insert.BindText(5, Data.Nickname);
    Insert.BindText(6, Data.QQNo);
    Insert.BindInt64(7, static_cast<long long>(Data.Status));
    if (Data.Reason.has_value())
    {
        Insert.BindText(8, Data.Reason.value());
    }
    else
    {
        Insert.BindNull(8);
    }
    Insert.BindInt64(9, static_cast<long long>(Data.UserRole));

    if (Insert.Step())
    {
        return false;
    }
    if (Insert.HasError())
    {
        return false;
    }

    return true;
}

std::optional<User> SqliteUserStore::FindByID(const std::string& UserID) const
{
    if (UserID.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE UserID = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, UserID);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::optional<User> SqliteUserStore::FindByPhone(const std::string& UserPhone) const
{
    if (UserPhone.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE UserPhone = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, UserPhone);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::optional<User> SqliteUserStore::FindByStdNo(const std::string& UserStdNo) const
{
    if (UserStdNo.empty())
    {
        return std::nullopt;
    }

    SqliteStatement Query(Database, std::string(SelectColumns) + "WHERE UserStdNo = ?;");
    if (!Query.IsReady())
    {
        return std::nullopt;
    }

    Query.BindText(1, UserStdNo);
    if (!Query.Step())
    {
        return std::nullopt;
    }
    return Load(Query);
}

std::vector<User> SqliteUserStore::All() const
{
    std::vector<User> Result;
    SqliteStatement Query(Database, std::string(SelectColumns) + "ORDER BY UserID;");
    if (!Query.IsReady())
    {
        return Result;
    }

    while (Query.Step())
    {
        Result.push_back(Load(Query));
    }
    return Result;
}

std::size_t SqliteUserStore::Size() const
{
    SqliteStatement Query(Database, "SELECT COUNT(*) FROM users;");
    if (!Query.IsReady() || !Query.Step())
    {
        return 0;
    }
    return static_cast<std::size_t>(Query.Int64(0));
}

User SqliteUserStore::Load(SqliteStatement& Row)
{
    User Data;
    Data.UserID = Row.Text(0);
    Data.UserPhone = Row.Text(1);
    Data.UserStdNo = Row.Text(2);
    Data.Password = Row.Text(3);
    Data.Nickname = Row.Text(4);
    Data.QQNo = Row.Text(5);
    Data.Status = static_cast<AccountStatus>(Row.Int64(6));
    if (!Row.IsNull(7))
    {
        Data.Reason = Row.Text(7);
    }
    Data.UserRole = static_cast<Role>(Row.Int64(8));
    return Data;
}
