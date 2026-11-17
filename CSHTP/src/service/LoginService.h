#pragma once
#include<optional>
#include<string>
#include"ServiceCommon.h"
#include"UserStore.h"

// 登录服务：所有角色共用同一个登录入口，账号固定为手机号；
// 管理员账号的手机号就是它的工号，所以同一个函数就能查到。
// 注册按角色分开（买家自助注册在 BuyerService，管理员账号发放由 AdminService 负责），
// 只有登录是统一的，因此单独成类。
class LoginService
{
    public:
        LoginService(UserStore& UserStoreRef);

        std::optional<User> Login(const std::string& Account,
                                  const std::string& Password,
                                  std::string* Error = nullptr) const;

    private:
        UserStore& Users;
};
