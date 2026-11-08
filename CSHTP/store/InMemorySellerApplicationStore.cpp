#include"InMemorySellerApplicationStore.h"
#include<algorithm>

namespace
{
    // 按提交时间升序。时间相同时用编号兜底：哈希表的遍历顺序不保证
    // 稳定，不兜底的话「同一毫秒提交的两条」在测试里会飘。
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
    // 编号为空说明是新建
    if (Application.ApplicationID.empty())
    {
        // 一人只能有一条申请记录，不论结论是待审、通过还是驳回。
        // 这里挡在分配编号之前：被拒绝的申请不该留下编号，也不该改入参
        for (const auto& Entry : Applications)
        {
            if (Entry.second.ApplicantID == Application.ApplicantID)
            {
                return false;
            }
        }

        Application.ApplicationID = MakeNextID();
    }

    // 有编号就是新增或覆盖。覆盖是有意的：审核结论要写回同一条申请，
    // 不能把审核过的记录再插一条新的
    Applications.insert_or_assign(Application.ApplicationID, Application);
    return true;
}

std::optional<SellerApplication> InMemorySellerApplicationStore::FindByID(const std::string& ApplicationID) const
{
    // 空编号在表里查不到，也正好是「找不到」的信号
    auto It = Applications.find(ApplicationID);
    if (It == Applications.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::vector<SellerApplication> InMemorySellerApplicationStore::ListByApplicant(const std::string& ApplicantID) const
{
    std::vector<SellerApplication> Result;

    // 空申请人不是「查全部」：列表查询一律不接受通配
    if (ApplicantID.empty())
    {
        return Result;
    }

    for (const auto& Entry : Applications)
    {
        if (Entry.second.ApplicantID == ApplicantID)
        {
            Result.push_back(Entry.second);
        }
    }

    SortByTime(Result);
    return Result;
}

std::optional<SellerApplication> InMemorySellerApplicationStore::FindLatestByApplicant(const std::string& ApplicantID) const
{
    if (ApplicantID.empty())
    {
        return std::nullopt;
    }

    std::optional<SellerApplication> Latest;
    for (const auto& Entry : Applications)
    {
        const SellerApplication& Application = Entry.second;
        if (Application.ApplicantID != ApplicantID)
        {
            continue;
        }

        if (!Latest.has_value() || Application.SubmittedAtMs > Latest->SubmittedAtMs)
        {
            Latest = Application;
        }
    }
    return Latest;
}

std::optional<SellerApplication> InMemorySellerApplicationStore::FindPendingByApplicant(const std::string& ApplicantID) const
{
    if (ApplicantID.empty())
    {
        return std::nullopt;
    }

    std::optional<SellerApplication> Pending;
    for (const auto& Entry : Applications)
    {
        const SellerApplication& Application = Entry.second;
        if (Application.ApplicantID != ApplicantID)
        {
            continue;
        }

        // 待审才是「还等着处理」的那一条；通过/驳回已经结束了
        if (Application.AuditStatus != ApplicationStatus::PendingReview)
        {
            continue;
        }

        if (!Pending.has_value() || Application.SubmittedAtMs > Pending->SubmittedAtMs)
        {
            Pending = Application;
        }
    }
    return Pending;
}

bool InMemorySellerApplicationStore::HasApplication(const std::string& ApplicantID) const
{
    // 空申请人不是「查全部」：一律当作「没有」
    if (ApplicantID.empty())
    {
        return false;
    }

    for (const auto& Entry : Applications)
    {
        if (Entry.second.ApplicantID == ApplicantID)
        {
            return true;
        }
    }
    return false;
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

std::size_t InMemorySellerApplicationStore::CountByStatus(ApplicationStatus Status) const
{
    std::size_t Count = 0;
    for (const auto& Entry : Applications)
    {
        if (Entry.second.AuditStatus == Status)
        {
            ++Count;
        }
    }
    return Count;
}

std::size_t InMemorySellerApplicationStore::Size() const
{
    return Applications.size();
}

std::string InMemorySellerApplicationStore::MakeNextID()
{
    // 编号一旦被占用就继续往后找，避免删改后重号
    std::string ID;
    do
    {
        ++LastAssignedNumber;
        ID = "app-" + std::to_string(LastAssignedNumber);
    } while (Applications.find(ID) != Applications.end());
    return ID;
}
