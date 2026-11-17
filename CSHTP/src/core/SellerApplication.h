#pragma once
#include<optional>
#include<string>
#include"enums.h"

class SqliteSellerApplicationStore;

// 卖家认证申请：一人一条，创建即待审核
class SellerApplication
{
    public:
        std::string ApplicationID;                  // 由存储层分配
        std::string ApplicantID;
        std::string RealName;
        ApplicationStatus AuditStatus = ApplicationStatus::PendingReview;
        long long SubmittedAtMs = 0;
        std::optional<long long> AuditedAtMs;
        std::optional<std::string> RejectReason;    // 仅驳回时有值
        std::optional<std::string> ReviewerID;

        static std::optional<SellerApplication> Create(const std::string& ApplicantID,
                                                       const std::string& RealName,
                                                       long long NowMs);

        bool Approve(const std::string& Reviewer, long long NowMs);
        bool Reject(const std::string& Reason, const std::string& Reviewer, long long NowMs);

    private:
        friend class SqliteSellerApplicationStore;
        SellerApplication() = default;
};
