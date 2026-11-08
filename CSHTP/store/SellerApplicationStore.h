#pragma once
#include<optional>
#include<string>
#include<vector>
#include<cstddef>
#include"SellerApplication.h"

// ============================================================
// SellerApplicationStore —— 卖家申请的存取
//
// 和 AuditRecordStore 一样，这里只做「存、取」，不解释业务：
//   1. 只增不删。申请是留痕，能删就等于没留。
//   2. 每个申请人只能有一条申请记录：不论结论是待审核、通过还是驳回，
//      同一申请人再存第二条一律拒绝（Save 返回 false）。这条规则放在这里，
//      是因为单看一条申请看不到别人的记录。
//   3. 允许覆盖保存。审核结论要写回原记录，所以 Save 对同一编号是更新，
//      不是插入第二条。
//   4. 列表按提交时间升序返回。
// ============================================================
class SellerApplicationStore
{
    public:
        virtual ~SellerApplicationStore() = default;

        // 存一条。编号为空则分配，有编号则覆盖（覆盖用于写回审核结论）。
        // 编号为空且该申请人已有记录时返回 false，不分配编号、不改变入参。
        virtual bool Save(SellerApplication& Application) = 0;

        virtual std::optional<SellerApplication> FindByID(const std::string& ApplicationID) const = 0;

        // 某个申请人的全部申请，按提交时间升序。
        // 因为一人只有一条，实际最多返回一条，保留列表接口是为了和别的 Store 一致
        virtual std::vector<SellerApplication> ListByApplicant(const std::string& ApplicantID) const = 0;

        // 某个申请人最近一次申请，不区分结论；没申请过返回 std::nullopt
        virtual std::optional<SellerApplication> FindLatestByApplicant(const std::string& ApplicantID) const = 0;

        // 某个申请人当前待审核的申请，没有则返回 std::nullopt
        virtual std::optional<SellerApplication> FindPendingByApplicant(const std::string& ApplicantID) const = 0;

        // 某个申请人是否已有申请记录（不论结论）。
        // 业务层「申请成为卖家」前先查它：true 就直接拒绝，连 Create 都不用做
        virtual bool HasApplication(const std::string& ApplicantID) const = 0;

        // 某个状态的全部申请，按提交时间升序（管理员待审列表用 PendingReview）
        virtual std::vector<SellerApplication> ListByStatus(ApplicationStatus Status) const = 0;

        // 某个状态的申请数（平台统计用）
        virtual std::size_t CountByStatus(ApplicationStatus Status) const = 0;

        virtual std::size_t Size() const = 0;
};
