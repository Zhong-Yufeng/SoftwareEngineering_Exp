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
    // 注册出来的一律是买家；卖家由申请审核通过后提权，管理员走 createAdmin
    U.UserRole = Role::Buyer;

    return U;
}

std::optional<User> User::createAdmin(const std::string& AdminID,
                                      const std::string& Password,
                                      const std::string& Nickname)
{
    // 管理员没有真实资料，编号就是全部标识，所以编号不能为空
    if (AdminID.empty() || Nickname.empty() || Password.size() < 6)
    {
        return std::nullopt;
    }

    User Admin;
    Admin.UserID = AdminID;
    // 手机号、学号、联系方式统一填管理员编号：登录按手机号/学号查都能命中，
    // 也不占用真实的手机号、学号位置
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
    // 只有买家能提权；卖家与管理员都不动
    if (UserRole != Role::Buyer)
    {
        return false;
    }

    UserRole = Role::Seller;
    return true;
}

bool User::Ban(const std::string& Reason)
{
    // 只有正常账号能被封禁，理由必填（界面要展示给被封禁的人看）
    if (Status != AccountStatus::Normal || Reason.empty())
    {
        return false;
    }

    Status = AccountStatus::Banned;
    this->Reason = Reason;
    return true;
}

bool User::Unban()
{
    // 只有已封禁的账号需要解封
    if (Status != AccountStatus::Banned)
    {
        return false;
    }

    Status = AccountStatus::Normal;
    this->Reason.reset();
    return true;
}