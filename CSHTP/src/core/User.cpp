#include"User.h"

std::optional<User> User::create(const std::string& UserPhone,
                                 const std::string& UserStdNo,
                                 const std::string& Password,
                                 const std::string& Nickname,
                                 const std::string& QQNo)
{
    // 手机号是唯一登录账号，手机号与学号都必须填写
    if (UserPhone.empty() || UserStdNo.empty())
    {
        return std::nullopt;
    }
    if (Password.size() < 6 || Nickname.empty() || QQNo.empty())
    {
        return std::nullopt;
    }

    User U;
    U.UserID = "";
    U.UserPhone = UserPhone;
    U.UserStdNo = UserStdNo;
    U.Password = Password;
    U.Nickname = Nickname;
    U.QQNo = QQNo;
    U.Status = AccountStatus::Normal;
    U.UserRole = Role::Buyer;
    return U;
}

std::optional<User> User::createAdmin(const std::string& AdminID,
                                      const std::string& Password,
                                      const std::string& Nickname)
{
    if (AdminID.empty() || Password.size() < 6 || Nickname.empty())
    {
        return std::nullopt;
    }

    User Admin;
    // 编号统一由存储层分配，这里只填账号：管理员没有真实资料，工号同时占手机号、学号、联系方式
    Admin.UserID = "";
    Admin.UserPhone = AdminID;
    Admin.UserStdNo = AdminID;
    Admin.Password = Password;
    Admin.Nickname = Nickname;
    Admin.QQNo = AdminID;
    Admin.Status = AccountStatus::Normal;
    Admin.UserRole = Role::Admin;
    return Admin;
}

bool User::BecomeSeller()
{
    if (UserRole != Role::Buyer)
    {
        return false;
    }

    UserRole = Role::Seller;
    return true;
}

bool User::Ban(const std::string& BanReason)
{
    if (Status != AccountStatus::Normal || BanReason.empty())
    {
        return false;
    }

    Status = AccountStatus::Banned;
    Reason = BanReason;
    return true;
}

bool User::Unban()
{
    if (Status != AccountStatus::Banned)
    {
        return false;
    }

    Status = AccountStatus::Normal;
    Reason.reset();
    return true;
}

bool User::ChangePassword(const std::string& OldPassword, const std::string& NewPassword)
{
    // 旧密码必须匹配，新密码沿用注册时的长度下限
    if (OldPassword != Password || NewPassword.size() < 6)
    {
        return false;
    }

    Password = NewPassword;
    return true;
}

bool User::UpdateProfile(const std::string& NewNickname, const std::string& NewQQNo)
{
    // 注册时昵称与联系方式就是必填，改资料时保持同一规则
    if (NewNickname.empty() || NewQQNo.empty())
    {
        return false;
    }

    Nickname = NewNickname;
    QQNo = NewQQNo;
    return true;
}

bool User::AssignID(const std::string& NewID)
{
    // 编号只允许在为空时补写一次，已有编号不允许覆盖
    if (!UserID.empty() || NewID.empty())
    {
        return false;
    }

    UserID = NewID;
    return true;
}
