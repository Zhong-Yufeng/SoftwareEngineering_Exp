#pragma once
#include<cstddef>
#include<string>
#include<optional>
#include<unordered_map>
#include<vector>
#include"SellerApplicationStore.h"

// ============================================================
// InMemorySellerApplicationStore —— 把卖家申请放在哈希表里
//
// 与 InMemoryAuditRecordStore 的差别有两处：
//   1. Save 会更新已有编号的记录：申请提交后还要写回审核结论。
//   2. Save 会拒绝同一申请人的第二条记录：一人只能有一条申请。
//
// 几个不变的约束：
//   1. 编号由这里分配（app-1、app-2……），因为「不重复」是集合的性质，
//      调用方自己拼编号迟早撞。
//   2. 存的是拷贝，取出来的也是拷贝，库里的记录不会被外面改坏。
//   3. 按申请人/状态查是线性遍历：申请只有几十到几百条，换二级索引
//      只是为了快一点，却要多维护一份索引同步，不值得。
// ============================================================
class InMemorySellerApplicationStore : public SellerApplicationStore
{
    public:
        bool Save(SellerApplication& Application) override;
        std::optional<SellerApplication> FindByID(const std::string& ApplicationID) const override;
        std::vector<SellerApplication> ListByApplicant(const std::string& ApplicantID) const override;
        std::optional<SellerApplication> FindLatestByApplicant(const std::string& ApplicantID) const override;
        std::optional<SellerApplication> FindPendingByApplicant(const std::string& ApplicantID) const override;
        bool HasApplication(const std::string& ApplicantID) const override;
        std::vector<SellerApplication> ListByStatus(ApplicationStatus Status) const override;
        std::size_t CountByStatus(ApplicationStatus Status) const override;
        std::size_t Size() const override;

    private:
        // 生成下一个没被占用的编号，形如 app-1
        std::string MakeNextID();

        std::unordered_map<std::string, SellerApplication> Applications;
        long long LastAssignedNumber = 0;
};
