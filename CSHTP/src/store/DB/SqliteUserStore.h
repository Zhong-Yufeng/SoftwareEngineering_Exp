#pragma once
#include<vector>
#include<string>
#include<cstddef>
#include"SqliteDatabase.h"
#include"UserStore.h"

class SqliteStatement;

// UserStore 的数据库实现，语义与 InMemoryUserStore 一致
class SqliteUserStore : public UserStore
{
    public:
        explicit SqliteUserStore(SqliteDatabase& Db);

        bool Save(User& Data) override;
        std::optional<User> FindByID(const std::string& UserID) const override;
        std::optional<User> FindByPhone(const std::string& UserPhone) const override;
        std::optional<User> FindByStdNo(const std::string& UserStdNo) const override;
        std::vector<User> All() const override;
        std::size_t Size() const override;

    private:
        static User Load(SqliteStatement& Row);

        SqliteDatabase& Database;
};
