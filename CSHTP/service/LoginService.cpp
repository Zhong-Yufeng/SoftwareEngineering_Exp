#include"LoginService.h"
#include"ServiceRules.h"

LoginService::LoginService(UserStore& UserStoreRef)
    : Users(UserStoreRef)
{
}

std::optional<User> LoginService::Login(const std::string& Account,
                                        const std::string& Password,
                                        std::string* Error) const
{
    ClearError(Error);

    if (Account.empty() || Password.empty())
    {
        SetError(Error, "账号与密码不能为空");
        return std::nullopt;
    }

    std::optional<User> Target = Users.FindByPhone(Account);
    if (!Target.has_value())
    {
        Target = Users.FindByStdNo(Account);
    }
    if (!Target.has_value() || Target->Password != Password)
    {
        SetError(Error, "账号或密码错误");
        return std::nullopt;
    }
    if (Target->Status == AccountStatus::Banned)
    {
        SetError(Error, ServiceRules::BannedMessage(*Target));
        return std::nullopt;
    }

    return Target;
}
