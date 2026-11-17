#include"InMemorySellerApplicationStore.h"
#include<algorithm>

namespace
{
    void SortByTime(std::vector<SellerApplication>& Applications)
    {
        std::sort(Applications.begin(), Applications.end(),
                  [](const SellerApplication& A, const SellerApplication& B)
                  {
                      if (A.SubmittedAtMs != B.SubmittedAtMs)
                      {
                          return A.SubmittedAtMs < B.SubmittedAtMs;
                      }
                      return A.ApplicationID < B.ApplicationID;
                  });
    }
}

bool InMemorySellerApplicationStore::Save(SellerApplication& Application)
{
    if (Application.ApplicationID.empty())
    {
        if (HasApplication(Application.ApplicantID))
        {
            return false;
        }
        Application.ApplicationID = MakeNextID();
    }

    Applications.insert_or_assign(Application.ApplicationID, Application);
    return true;
}

std::optional<SellerApplication> InMemorySellerApplicationStore::FindByID(const std::string& ApplicationID) const
{
    const auto It = Applications.find(ApplicationID);
    if (It == Applications.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::optional<SellerApplication> InMemorySellerApplicationStore::FindByApplicant(const std::string& ApplicantID) const
{
    if (ApplicantID.empty())
    {
        return std::nullopt;
    }

    for (const auto& Entry : Applications)
    {
        if (Entry.second.ApplicantID == ApplicantID)
        {
            return Entry.second;
        }
    }
    return std::nullopt;
}

std::optional<SellerApplication> InMemorySellerApplicationStore::FindPendingByApplicant(const std::string& ApplicantID) const
{
    const auto Application = FindByApplicant(ApplicantID);
    if (!Application.has_value() || Application->AuditStatus != ApplicationStatus::PendingReview)
    {
        return std::nullopt;
    }
    return Application;
}

bool InMemorySellerApplicationStore::HasApplication(const std::string& ApplicantID) const
{
    return FindByApplicant(ApplicantID).has_value();
}

std::vector<SellerApplication> InMemorySellerApplicationStore::ListByStatus(ApplicationStatus Status) const
{
    std::vector<SellerApplication> Result;
    for (const auto& Entry : Applications)
    {
        if (Entry.second.AuditStatus == Status)
        {
            Result.push_back(Entry.second);
        }
    }

    SortByTime(Result);
    return Result;
}

std::vector<SellerApplication> InMemorySellerApplicationStore::All() const
{
    std::vector<SellerApplication> Result;
    Result.reserve(Applications.size());
    for (const auto& Entry : Applications)
    {
        Result.push_back(Entry.second);
    }

    SortByTime(Result);
    return Result;
}

std::size_t InMemorySellerApplicationStore::Size() const
{
    return Applications.size();
}

std::string InMemorySellerApplicationStore::MakeNextID()
{
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "app-" + std::to_string(LastAssignedNumber);
        if (Applications.find(Candidate) == Applications.end())
        {
            return Candidate;
        }
    }
}
