#include"InMemoryUserStore.h"
#include<algorithm>

bool InMemoryUserStore::Save(User& Data)
{
    // 一个账号只能对应一个用户：手机号与学号都不能指向另一个已有用户
    for (const auto& Entry : Users)
    {
        const bool SameUser = !Data.UserID.empty() && Entry.first == Data.UserID;
        if (SameUser)
        {
            continue;
        }
        if (!Data.UserPhone.empty() && Entry.second.UserPhone == Data.UserPhone)
        {
            return false;
        }
        if (!Data.UserStdNo.empty() && Entry.second.UserStdNo == Data.UserStdNo)
        {
            return false;
        }
    }

    // 编号由系统统一分配，调用方不给编号
    if (Data.UserID.empty())
    {
        Data.UserID = MakeNextID();
    }

    Users.insert_or_assign(Data.UserID, Data);
    return true;
}

std::optional<User> InMemoryUserStore::FindByID(const std::string& UserID) const
{
    const auto It = Users.find(UserID);
    if (It == Users.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::optional<User> InMemoryUserStore::FindByPhone(const std::string& UserPhone) const
{
    if (UserPhone.empty())
    {
        return std::nullopt;
    }

    for (const auto& Entry : Users)
    {
        if (Entry.second.UserPhone == UserPhone)
        {
            return Entry.second;
        }
    }
    return std::nullopt;
}

std::optional<User> InMemoryUserStore::FindByStdNo(const std::string& UserStdNo) const
{
    if (UserStdNo.empty())
    {
        return std::nullopt;
    }

    for (const auto& Entry : Users)
    {
        if (Entry.second.UserStdNo == UserStdNo)
        {
            return Entry.second;
        }
    }
    return std::nullopt;
}

std::vector<User> InMemoryUserStore::All() const
{
    std::vector<User> Out;
    Out.reserve(Users.size());
    for (const auto& Entry : Users)
    {
        Out.push_back(Entry.second);
    }

    std::sort(Out.begin(), Out.end(),
              [](const User& A, const User& B) { return A.UserID < B.UserID; });
    return Out;
}

std::size_t InMemoryUserStore::Size() const
{
    return Users.size();
}

std::string InMemoryUserStore::MakeNextID()
{
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "user-" + std::to_string(LastAssignedNumber);
        if (Users.find(Candidate) == Users.end())
        {
            return Candidate;
        }
    }
}
