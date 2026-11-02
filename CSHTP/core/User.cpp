#include"User.h"

std::optional<User> User::create(const std::string& UserID,
                                 const std::string& UserPhone,
                                 const std::string& UserStdNo,
                                 const std::string& Password,
                                 const std::string& Nickname,
                                 const std::string& QQNo)
{
    // 手机号、学号二选一
    if (UserPhone.empty() && UserStdNo.empty())
    {
        return std::nullopt;
    }

    // 密码至少 6 位
    if (Password.size() < 6)
    {
        return std::nullopt;
    }

    // 昵称、联系方式必填
    if (Nickname.empty() || QQNo.empty())
    {
        return std::nullopt;
    }

    User U;
    U.UserID = UserID;
    if (!UserPhone.empty())
    {
        U.UserPhone = UserPhone;
    }
    if (!UserStdNo.empty())
    {
        U.UserStdNo = UserStdNo;
    }
    U.Password = Password;
    U.Nickname = Nickname;
    U.QQNo = QQNo;
    U.Status = AccountStatus::Normal;

    return U;
}