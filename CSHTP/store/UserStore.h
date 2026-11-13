#pragma once
#include<optional>
#include<string>
#include<vector>
#include<cstddef>
#include"User.h"

class UserStore
{
    public:
        virtual ~UserStore() = default;

        // 编号为空时由存储层分配
        virtual bool Save(User& Data) = 0;
        virtual std::optional<User> FindByID(const std::string& UserID) const = 0;
        virtual std::optional<User> FindByPhone(const std::string& UserPhone) const = 0;
        virtual std::optional<User> FindByStdNo(const std::string& UserStdNo) const = 0;
        virtual std::vector<User> All() const = 0;
        virtual std::size_t Size() const = 0;
};
