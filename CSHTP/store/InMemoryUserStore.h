#pragma once
#include<optional>
#include<string>
#include<vector>
#include<cstddef>
#include<unordered_map>
#include"UserStore.h"

class InMemoryUserStore : public UserStore
{
    public:
        bool Save(User& Data) override;
        std::optional<User> FindByID(const std::string& UserID) const override;
        std::optional<User> FindByPhone(const std::string& UserPhone) const override;
        std::optional<User> FindByStdNo(const std::string& UserStdNo) const override;
        std::vector<User> All() const override;
        std::size_t Size() const override;

    private:
        std::string MakeNextID();

        std::unordered_map<std::string, User> Users;
        long long LastAssignedNumber = 0;
};
