#include"SellerApplication.h"

std::optional<SellerApplication> SellerApplication::Create(const std::string& ApplicantID,
                                                           const std::string& RealName,
                                                           long long NowMs)
{
    // 申请人决定「这是谁的申请」，姓名是审核员核对的依据，缺一不可
    if (ApplicantID.empty() || RealName.empty())
    {
        return std::nullopt;
    }

    // 创建即提交，提交时间必须是有效时间
    if (NowMs <= 0)
    {
        return std::nullopt;
    }

    SellerApplication Application;
    Application.ApplicantID = ApplicantID;
    Application.RealName = RealName;
    Application.SubmittedAtMs = NowMs;
    return Application;
}

bool SellerApplication::Approve(const std::string& Reviewer, long long NowMs)
{
    // 只有待审核的单子才能改判；两个终态都不行
    if (AuditStatus != ApplicationStatus::PendingReview)
    {
        return false;
    }

    // 审核人必须有身份，否则「谁批的」无从查起
    if (Reviewer.empty())
    {
        return false;
    }

    if (NowMs <= 0)
    {
        return false;
    }

    AuditStatus = ApplicationStatus::Approved;
    AuditedAtMs = NowMs;
    ReviewerID = Reviewer;

    // 通过的申请不该残留驳回理由
    RejectReason = std::nullopt;
    return true;
}

bool SellerApplication::Reject(const std::string& Reason, const std::string& Reviewer, long long NowMs)
{
    if (AuditStatus != ApplicationStatus::PendingReview)
    {
        return false;
    }

    // 驳回必须带理由：否则申请人只收到一句「被驳回」，不知道改什么
    if (Reason.empty())
    {
        return false;
    }

    if (Reviewer.empty())
    {
        return false;
    }

    if (NowMs <= 0)
    {
        return false;
    }

    AuditStatus = ApplicationStatus::Rejected;
    AuditedAtMs = NowMs;
    ReviewerID = Reviewer;
    RejectReason = Reason;
    return true;
}
