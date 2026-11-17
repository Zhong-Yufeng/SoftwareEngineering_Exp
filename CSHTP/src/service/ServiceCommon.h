#pragma once
#include<chrono>
#include<functional>
#include<string>
#include"Transaction.h"

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

// 服务层事务作用域：构造时开启，析构时还没有提交就回滚。
// 一个用例内跨多张表的写入放在同一个作用域里，保证要么全部落库、要么全部不落。
class TransactionScope
{
    public:
        explicit TransactionScope(Transaction& Target)
            : Tx(Target), Begun(Tx.Begin())
        {
        }

        ~TransactionScope()
        {
            if (!Committed)
            {
                Tx.Rollback();
            }
        }

        TransactionScope(const TransactionScope&) = delete;
        TransactionScope& operator=(const TransactionScope&) = delete;

        bool Ready() const
        {
            return Begun;
        }

        bool Commit()
        {
            if (!Begun || !Tx.Commit())
            {
                return false;
            }
            Committed = true;
            return true;
        }

    private:
        Transaction& Tx;
        bool Begun = false;
        bool Committed = false;
};

namespace ServiceObject
{
    constexpr const char* User = "User";
    constexpr const char* Item = "Item";
    constexpr const char* Reservation = "Reservation";
    constexpr const char* SellerApplication = "SellerApplication";
}
