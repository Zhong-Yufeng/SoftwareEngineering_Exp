#include"InMemoryAuditRecordStore.h"
#include<algorithm>

namespace
{
    void SortByTime(std::vector<AuditRecord>& Records)
    {
        std::sort(Records.begin(), Records.end(),
                  [](const AuditRecord& A, const AuditRecord& B)
                  {
                      if (A.AuditedAtMs != B.AuditedAtMs)
                      {
                          return A.AuditedAtMs < B.AuditedAtMs;
                      }
                      return A.RecordID < B.RecordID;
                  });
    }
}

bool InMemoryAuditRecordStore::Save(AuditRecord& Record)
{
    if (Record.RecordID.empty())
    {
        Record.RecordID = MakeNextID();
    }

    Records.insert_or_assign(Record.RecordID, Record);
    return true;
}

std::optional<AuditRecord> InMemoryAuditRecordStore::FindByID(const std::string& RecordID) const
{
    const auto It = Records.find(RecordID);
    if (It == Records.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::vector<AuditRecord> InMemoryAuditRecordStore::ListByItem(const std::string& ItemID) const
{
    std::vector<AuditRecord> Out;
    if (ItemID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Records)
    {
        if (Entry.second.ItemID == ItemID)
        {
            Out.push_back(Entry.second);
        }
    }

    SortByTime(Out);
    return Out;
}

std::vector<AuditRecord> InMemoryAuditRecordStore::ListByReviewer(const std::string& ReviewerID) const
{
    std::vector<AuditRecord> Out;
    if (ReviewerID.empty())
    {
        return Out;
    }

    for (const auto& Entry : Records)
    {
        if (Entry.second.ReviewerID == ReviewerID)
        {
            Out.push_back(Entry.second);
        }
    }

    SortByTime(Out);
    return Out;
}

std::optional<AuditRecord> InMemoryAuditRecordStore::FindLatestByItem(const std::string& ItemID) const
{
    if (ItemID.empty())
    {
        return std::nullopt;
    }

    std::optional<AuditRecord> Latest;
    for (const auto& Entry : Records)
    {
        const AuditRecord& Record = Entry.second;
        if (Record.ItemID != ItemID)
        {
            continue;
        }

        if (!Latest.has_value() || Record.AuditedAtMs > Latest->AuditedAtMs)
        {
            Latest = Record;
        }
    }
    return Latest;
}

std::vector<AuditRecord> InMemoryAuditRecordStore::All() const
{
    std::vector<AuditRecord> Out;
    Out.reserve(Records.size());

    for (const auto& Entry : Records)
    {
        Out.push_back(Entry.second);
    }

    SortByTime(Out);
    return Out;
}

std::size_t InMemoryAuditRecordStore::Size() const
{
    return Records.size();
}

std::string InMemoryAuditRecordStore::MakeNextID()
{
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "aud-" + std::to_string(LastAssignedNumber);
        if (Records.find(Candidate) == Records.end())
        {
            return Candidate;
        }
    }
}
