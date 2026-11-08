#include"OperationLog.h"
#include<ctime>

namespace
{
    // 四个 Create 共用的前置校验：时间必须有效，参与拼接的字符串必须非空
    bool ValidTime(long long NowMs)
    {
        return NowMs > 0;
    }

    std::string PassedWord(bool Passed)
    {
        return Passed ? "通过" : "驳回";
    }
}

std::string OperationLog::FormatTime(long long NowMs)
{
    std::time_t Seconds = static_cast<std::time_t>(NowMs / 1000);
    std::tm Local{};

#ifdef _WIN32
    localtime_s(&Local, &Seconds);
#else
    localtime_r(&Seconds, &Local);
#endif

    char Buffer[32] = {};
    std::strftime(Buffer, sizeof(Buffer), "%Y-%m-%d %H:%M:%S", &Local);
    return Buffer;
}

std::optional<OperationLog> OperationLog::CreateRegister(const std::string& UserID,
                                                         long long NowMs)
{
    // 没有用户 ID 就说不清是谁注册的
    if (UserID.empty() || !ValidTime(NowMs))
    {
        return std::nullopt;
    }

    OperationLog Log;
    Log.Type = LogType::Register;
    Log.OperatedAtMs = NowMs;
    Log.Content = "用户" + UserID + "在" + FormatTime(NowMs) + "注册了账号";
    return Log;
}

std::optional<OperationLog> OperationLog::CreateItemSold(const std::string& BuyerID,
                                                         const std::string& SellerID,
                                                         const std::string& ItemID,
                                                         const std::string& ItemName,
                                                         long long NowMs)
{
    // 成交要能追溯到买卖双方和具体商品，任何一项缺失都说明调用方传错了
    if (BuyerID.empty() || SellerID.empty() || ItemID.empty() || ItemName.empty()
        || !ValidTime(NowMs))
    {
        return std::nullopt;
    }

    OperationLog Log;
    Log.Type = LogType::ItemSold;
    Log.OperatedAtMs = NowMs;
    Log.Content = "买家" + BuyerID + "和卖家" + SellerID + "在" + FormatTime(NowMs)
                  + "成功交易" + ItemID + "+" + ItemName;
    return Log;
}

std::optional<OperationLog> OperationLog::CreateItemAudit(const std::string& AdminID,
                                                          const std::string& SellerID,
                                                          const std::string& ItemID,
                                                          const std::string& ItemName,
                                                          bool Passed,
                                                          long long NowMs)
{
    if (AdminID.empty() || SellerID.empty() || ItemID.empty() || ItemName.empty()
        || !ValidTime(NowMs))
    {
        return std::nullopt;
    }

    OperationLog Log;
    Log.Type = LogType::ItemAudited;
    Log.OperatedAtMs = NowMs;
    Log.Content = "管理员" + AdminID + "在" + FormatTime(NowMs) + "审核"
                  + PassedWord(Passed) + "了卖家" + SellerID + "的商品"
                  + ItemID + "+" + ItemName;
    return Log;
}

std::optional<OperationLog> OperationLog::CreateSellerAudit(const std::string& AdminID,
                                                            const std::string& UserID,
                                                            bool Passed,
                                                            long long NowMs)
{
    if (AdminID.empty() || UserID.empty() || !ValidTime(NowMs))
    {
        return std::nullopt;
    }

    OperationLog Log;
    Log.Type = LogType::SellerAudited;
    Log.OperatedAtMs = NowMs;
    Log.Content = "管理员" + AdminID + "在" + FormatTime(NowMs) + "审核"
                  + PassedWord(Passed) + "了用户" + UserID + "的卖家申请";
    return Log;
}
