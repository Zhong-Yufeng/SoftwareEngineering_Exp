#pragma once
#include<optional>
#include<string>
#include<unordered_map>
#include"UserStore.h"

class InMemoryUserStore : public UserStore
{
    public:
        bool Save(const User& Data) override;
        std::optional<User> FindByID(const std::string& UserID)const override;
        std::optional<User> FindByPhone(const std::string& UserPhone)const override;
        std::optional<User> FindByStdNo(const std::string& UserStdNo)const override;
        
    private:
        std::unordered_map<std::string, User> Users;
};