#pragma once
#include<optional>
#include<string>
#include"enums.h"

class SqliteUserStore;

// 用户：买家、卖家、管理员共用，权限由 Role 区分
class User
{
    public:
        std::string UserID;                     // 由存储层分配
        std::string UserPhone;
        std::string UserStdNo;
        std::string Password;
        std::string Nickname;
        std::string QQNo;
        AccountStatus Status = AccountStatus::Normal;
        std::optional<std::string> Reason;      // 仅封禁时有值
        Role UserRole = Role::Buyer;

        static std::optional<User> create(const std::string& UserPhone,
                                          const std::string& UserStdNo,
                                          const std::string& Password,
                                          const std::string& Nickname,
                                          const std::string& QQNo);

        static std::optional<User> createAdmin(const std::string& AdminID,
                                               const std::string& Password,
                                               const std::string& Nickname);

        bool BecomeSeller();
        bool Ban(const std::string& BanReason);
        bool Unban();

        // ----- 以下函数暂未实现 ----------------------------------------
        bool ChangePassword(const std::string& OldPassword, const std::string& NewPassword);
        bool UpdateProfile(const std::string& NewNickname, const std::string& NewQQNo);
        bool AssignID(const std::string& NewID);

    private:
        friend class SqliteUserStore;
        User() = default;
};
