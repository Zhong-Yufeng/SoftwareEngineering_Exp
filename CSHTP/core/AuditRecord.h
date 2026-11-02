#pragma once
#include<optional>
#include<string>
#include"enums.h"

class AuditRecord
{
    public:
        // 审核记录类的变量
        std::string RecordID;
        std::string ItemID;
        std::string ReviewerID;
        AuditResult Result;
        long long AuditedAtMs;              // 审核时间
        std::optional<std::string> Reason;
        // 审核记录类的变量
};
