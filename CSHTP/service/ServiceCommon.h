#pragma once
#include<chrono>
#include<functional>
#include<string>

// 业务层统一取时入口，测试可注入固定时钟
using ServiceClock = std::function<long long()>;

inline long long SystemTimeMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

inline void SetError(std::string* Error, const std::string& Message)
{
    if (Error != nullptr)
    {
        *Error = Message;
    }
}

inline void ClearError(std::string* Error)
{
    if (Error != nullptr)
    {
        Error->clear();
    }
}

namespace ServiceObject
{
    constexpr const char* User = "User";
    constexpr const char* Item = "Item";
    constexpr const char* Reservation = "Reservation";
    constexpr const char* SellerApplication = "SellerApplication";
}
