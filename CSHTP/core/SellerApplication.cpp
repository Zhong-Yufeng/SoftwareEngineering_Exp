#include"SellerApplication.h"

std::optional<SellerApplication> SellerApplication::Create(const std::string& ApplicantID,
                                                           const std::string& RealName,
                                                           long long NowMs)
{
    if (ApplicantID.empty() || RealName.empty() || NowMs <= 0)
    {
        return std::nullopt;
    }

    SellerApplication Application;
    Application.ApplicationID = "";
    Application.ApplicantID = ApplicantID;
    Application.RealName = RealName;
    Application.SubmittedAtMs = NowMs;
    return Application;
}

bool SellerApplication::Approve(const std::string& Reviewer, long long NowMs)
{
    if (AuditStatus != ApplicationStatus::PendingReview
        || Reviewer.empty() || NowMs <= 0)
    {
        return false;
    }

    AuditStatus = ApplicationStatus::Approved;
    AuditedAtMs = NowMs;
    ReviewerID = Reviewer;
    RejectReason = std::nullopt;
    return true;
}

bool SellerApplication::Reject(const std::string& Reason, const std::string& Reviewer, long long NowMs)
{
    if (AuditStatus != ApplicationStatus::PendingReview
        || Reason.empty() || Reviewer.empty() || NowMs <= 0)
    {
        return false;
    }

    AuditStatus = ApplicationStatus::Rejected;
    AuditedAtMs = NowMs;
    ReviewerID = Reviewer;
    RejectReason = Reason;
    return true;
}
