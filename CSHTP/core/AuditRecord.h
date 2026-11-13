#pragma once
#include<optional>
#include<string>
#include"enums.h"

// 一次商品审核的留痕，商品每次审核一条
class AuditRecord
{
    public:
        std::string RecordID;               // 由存储层分配
        std::string ItemID;
        std::string ReviewerID;
        AuditResult Result = AuditResult::Approved;
        long long AuditedAtMs = 0;
        std::optional<std::string> Reason;  // 仅驳回时有值

        static std::optional<AuditRecord> Create(const std::string& ItemID,
                                                 const std::string& ReviewerID,
                                                 AuditResult Result,
                                                 long long AuditedAtMs,
                                                 const std::string& Reason);

    private:
        AuditRecord() = default;
};
