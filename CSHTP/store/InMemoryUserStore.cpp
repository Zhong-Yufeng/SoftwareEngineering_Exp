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
        if (Pair.second.UserStdNo == UserStdNo)
        {
            return Pair.second;
        }
    }
    return std::nullopt;
}