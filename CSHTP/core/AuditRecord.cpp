#include "AuditRecord.h"

std::optional<AuditRecord> AuditRecord::Create(const std::string& ItemID,
                                               const std::string& ReviewerID,
                                               AuditResult Result,
                                               long long AuditedAtMs,
                                               const std::string& Reason)
{
    // 审核记录必须能回答「谁审的哪件商品」，缺一个就没法追溯
    if (ItemID.empty() || ReviewerID.empty())
    {
        return std::nullopt;
    }

    // 时间必须有效。0 表示「没记」，不是合法审核时间
    if (AuditedAtMs <= 0)
    {
        return std::nullopt;
    }

    // 驳回必须带理由：否则卖家收到的只是一句「被驳回」，
    // 无从修改，这条记录也就失去了它的意义
    if (Result == AuditResult::Rejected && Reason.empty())
    {
        return std::nullopt;
    }

    AuditRecord Record;
    Record.RecordID = "";           // 编号由存储层分配
    Record.ItemID = ItemID;
    Record.ReviewerID = ReviewerID;
    Record.Result = Result;
    Record.AuditedAtMs = AuditedAtMs;

    // 不变式：Reason 只在驳回时有值。
    // 通过时即使调用方传了理由，也一并丢弃——「通过」没有理由可写，
    // 留着会让「有理由 = 被驳回」这条判断失效。
    if (Result == AuditResult::Rejected)
    {
        Record.Reason = Reason;
    }
    else
    {
        Record.Reason = std::nullopt;
    }

    return Record;
}
