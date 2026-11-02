#pragma once
#include<optional>
#include<string>
#include"enums.h"

class SellerApplication
{
    public:
        // 卖家认证申请类的变量
        std::string ApplicationID;
        std::string ApplicantID;
        std::string RealName;
        long long SubmittedAtMs;            // 提交时间
        ApplicationStatus AuditStatus;
        std::optional<long long> AuditedAtMs;
        std::optional<std::string> RejectReason;
        std::optional<std::string> ReviewerID;
        // 卖家认证申请类的变量

        // 卖家认证申请类的方法
        bool Submit();
        bool Audit(bool Passed, const std::string Reason);
        // 卖家认证申请类的方法
};
