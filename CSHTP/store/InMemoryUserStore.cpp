#include"InMemoryUserStore.h"

bool InMemoryUserStore::Save(const User& Data)
{
    Users.insert_or_assign(Data.UserID, Data);
    return true;
}

std::optional<User> InMemoryUserStore::FindByID(const std::string& UserID) const
{
    auto It = Users.find(UserID);
    if (It == Users.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::optional<User> InMemoryUserStore::FindByPhone(const std::string& UserPhone) const
{
    for (const auto& Pair : Users)
    {
        // 管理员账号不参与按手机号/学号的查找，避免普通注册或登录命中管理员
        if (Pair.second.UserRole == Role::Admin)
        {
            continue;
        }
        if (Pair.second.UserPhone == UserPhone)
        {
            return Pair.second;
        }
    }
    return std::nullopt;
}

std::optional<User> InMemoryUserStore::FindByStdNo(const std::string& UserStdNo) const
{
    for (const auto& Pair : Users)
    {
        if (Pair.second.UserRole == Role::Admin)
        {
            continue;
        }
        if (Pair.second.UserStdNo == UserStdNo)
        {
            return Pair.second;
        }
    }
    return std::nullopt;
}