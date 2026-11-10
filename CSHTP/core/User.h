#pragma once
#include<optional>
#include<vector>
#include<string>
#include"enums.h"

class User
{
    private:
        User() = default;
    public:
        // 用户类的变量
        std::string UserID;
        std::optional<std::string> UserPhone;
        std::optional<std::string> UserStdNo;
        std::string Password;
        std::string Nickname;
        std::string QQNo;
        AccountStatus Status = AccountStatus::Normal;
        std::optional<std::string> Reason;
        Role UserRole = Role::Buyer;
        // 用户类的变量

        // 用户类的构造函数
        static std::optional<User> create(const std::string& UserID,
                                 const std::string& UserPhone,
                                 const std::string& UserStdNo,
                                 const std::string& Password,
                                 const std::string& Nickname,
                                 const std::string& QQNo);

        // 只给后台预置管理员用：管理员没有真实学号和手机号，
        // 手机号、学号、联系方式统一填管理员编号，编号从 "1" 开始由 main 维护
        static std::optional<User> createAdmin(const std::string& AdminID,
                                               const std::string& Password,
                                               const std::string& Nickname);

        // 用户类的方法

            //状态转变：只写自己的字段，能不能转、由谁触发由调用方保证
            bool BecomeSeller();                    // 买家 → 卖家（管理员审核通过后调用）
            bool Ban(const std::string& Reason);    // 正常 → 已封禁，理由必填
            bool Unban();                           // 已封禁 → 正常，清空封禁理由
            //状态转变

        // 用户类的方法

};