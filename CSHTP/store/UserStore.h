#pragma once
#include<optional>
#include<string>
#include"User.h"

class UserStore
{
    public:
        // 虚类的析构函数
        virtual ~UserStore() = default;
        // 虚类的析构函数

        //User存储类的接口
        virtual bool Save(const User& Data) = 0;
        virtual std::optional<User> FindByID(const std::string& UserID) const = 0;
        virtual std::optional<User> FindByPhone(const std::string& UserPhone) const = 0;
        virtual std::optional<User> FindByStdNo(const std::string& UserStdNo) const = 0;
        //User存储类的接口
};