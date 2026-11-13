#include"AuditRecord.h"

std::optional<AuditRecord> AuditRecord::Create(const std::string& ItemID,
                                               const std::string& ReviewerID,
                                               AuditResult Result,
                                               long long AuditedAtMs,
                                               const std::string& Reason)
{
    if (ItemID.empty() || ReviewerID.empty() || AuditedAtMs <= 0)
    {
        return std::nullopt;
    }
    if (Result == AuditResult::Rejected && Reason.empty())
    {
        return std::nullopt;
    }

    AuditRecord Record;
    Record.RecordID = "";
    Record.ItemID = ItemID;
    Record.ReviewerID = ReviewerID;
    Record.Result = Result;
    Record.AuditedAtMs = AuditedAtMs;
    Record.Reason = (Result == AuditResult::Rejected) ? std::optional<std::string>(Reason)
                                                      : std::nullopt;
    return Record;
}
