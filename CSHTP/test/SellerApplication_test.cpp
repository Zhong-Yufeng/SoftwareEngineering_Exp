#include"test.h"
#include"SellerApplication.h"
#include"InMemorySellerApplicationStore.h"

// ============================================================
// 卖家申请与存储层的冒烟测试
//
// 覆盖：
//   SellerApplication::Create 的校验与初始状态（创建即待审核）
//   Approve / Reject 的校验规则、字段落位与终态
//   InMemorySellerApplicationStore 的编号分配、覆盖保存、按编号查
//   一人只能有一条申请的约束、按申请人查、按状态查与计数
//   结果与库的隔离
//
// 【测试的写法约定】申请都走 Create 造出来，Create 成功就是「已提交、
// 待审核」，不再有草稿这一步；审核走 Approve / Reject。
// 时间显式给 1000 / 2000 / 3000，便于断言。
// ============================================================

using Test::Check;

namespace
{
    // 造一条待审核的申请。Create 失败说明测试装置本身有问题，
    // 直接让 Check 报出来，后面的失败才是业务问题
    SellerApplication MakePending(const std::string& ApplicantID,
                                  const std::string& RealName,
                                  long long SubmittedAtMs)
    {
        auto Created = SellerApplication::Create(ApplicantID, RealName, SubmittedAtMs);
        Check(Created.has_value(), "测试装置: Create 应当成功");
        return Created.value();
    }
}

void RunSellerApplicationTests()
{
    // ================= 1. Create 的校验与初始状态 =================
    {
        Check(!SellerApplication::Create("", "张三", 1000).has_value(),
              "Create: 申请人为空应返回空");
        Check(!SellerApplication::Create("u001", "", 1000).has_value(),
              "Create: 姓名为空应返回空");
        Check(!SellerApplication::Create("", "", 1000).has_value(),
              "Create: 两项都空应返回空");
        Check(!SellerApplication::Create("u001", "张三", 0).has_value(),
              "Create: 时间为 0 应返回空");
        Check(!SellerApplication::Create("u001", "张三", -1).has_value(),
              "Create: 时间为负应返回空");
    }
    {
        auto Created = SellerApplication::Create("u001", "张三", 1000);
        Check(Created.has_value(), "Create: 字段齐全应成功");

        const SellerApplication& Application = Created.value();
        Check(Application.ApplicationID.empty(), "Create: 编号应留空等存储层分配");
        Check(Application.ApplicantID == "u001", "Create: 应记下申请人");
        Check(Application.RealName == "张三", "Create: 应记下姓名");
        Check(Application.AuditStatus == ApplicationStatus::PendingReview,
              "Create: 创建即待审核");
        Check(Application.SubmittedAtMs == 1000, "Create: 应写入提交时间");
        Check(!Application.AuditedAtMs.has_value(), "Create: 审核时间应为空");
        Check(!Application.RejectReason.has_value(), "Create: 驳回理由应为空");
        Check(!Application.ReviewerID.has_value(), "Create: 审核人应为空");
    }

    // ================= 2. Approve / Reject 的失败路径 =================
    {
        SellerApplication Application = MakePending("u001", "张三", 1000);
        Check(!Application.Approve("", 2000), "Approve: 审核人为空应被拒绝");
        Check(!Application.Approve("admin", 0), "Approve: 审核时间为 0 应被拒绝");
        Check(!Application.Approve("admin", -1), "Approve: 审核时间为负应被拒绝");
        Check(!Application.Reject("信息不全", "", 2000), "Reject: 审核人为空应被拒绝");
        Check(!Application.Reject("", "admin", 2000), "Reject: 理由为空应被拒绝");
        Check(!Application.Reject("信息不全", "admin", 0), "Reject: 审核时间为 0 应被拒绝");
        Check(Application.AuditStatus == ApplicationStatus::PendingReview,
              "审核失败后: 状态应保持待审核");
        Check(!Application.AuditedAtMs.has_value(), "审核失败后: 审核时间应仍为空");
        Check(!Application.ReviewerID.has_value(), "审核失败后: 审核人应仍为空");
        Check(!Application.RejectReason.has_value(), "审核失败后: 驳回理由应仍为空");
    }

    // ================= 3. 通过的字段落位与终态 =================
    {
        SellerApplication Application = MakePending("u001", "张三", 1000);
        Check(Application.Approve("admin-1", 2000), "Approve: 应成功");
        Check(Application.AuditStatus == ApplicationStatus::Approved,
              "Approve: 状态应为通过");
        Check(Application.AuditedAtMs.has_value() && Application.AuditedAtMs.value() == 2000,
              "Approve: 应写入审核时间");
        Check(Application.ReviewerID.has_value() && Application.ReviewerID.value() == "admin-1",
              "Approve: 应写入审核人");
        Check(!Application.RejectReason.has_value(), "Approve: 不应留下驳回理由");

        Check(!Application.Approve("admin-2", 3000), "Approve: 终态不应再次审核");
        Check(!Application.Reject("改判", "admin-2", 3000), "Approve 后: 不应改判为驳回");
        Check(Application.AuditStatus == ApplicationStatus::Approved,
              "改判失败后: 状态应保持通过");
        Check(Application.AuditedAtMs.value() == 2000, "改判失败后: 审核时间不应被覆盖");
        Check(Application.ReviewerID.value() == "admin-1", "改判失败后: 审核人不应被覆盖");
    }

    // ================= 4. 驳回的字段落位与终态 =================
    {
        SellerApplication Application = MakePending("u001", "张三", 1000);
        Check(Application.Reject("姓名与学号不匹配", "admin-2", 2000),
              "Reject: 应成功");
        Check(Application.AuditStatus == ApplicationStatus::Rejected,
              "Reject: 状态应为驳回");
        Check(Application.RejectReason.has_value()
              && Application.RejectReason.value() == "姓名与学号不匹配",
              "Reject: 应记下理由");
        Check(Application.AuditedAtMs.has_value() && Application.AuditedAtMs.value() == 2000,
              "Reject: 应写入审核时间");
        Check(Application.ReviewerID.has_value() && Application.ReviewerID.value() == "admin-2",
              "Reject: 应写入审核人");

        Check(!Application.Approve("admin-3", 3000), "Reject 后: 不应改判为通过");
        Check(!Application.Reject("再驳一次", "admin-3", 3000), "Reject 后: 不应再次驳回");
        Check(Application.AuditStatus == ApplicationStatus::Rejected,
              "改判失败后: 状态应保持驳回");
        Check(Application.RejectReason.value() == "姓名与学号不匹配",
              "改判失败后: 驳回理由不应被覆盖");
        Check(Application.AuditedAtMs.value() == 2000, "改判失败后: 审核时间不应被覆盖");
    }

    // ================= 5. 编号分配与覆盖保存 =================
    {
        InMemorySellerApplicationStore Store;
        SellerApplication First = MakePending("u001", "张三", 1000);
        Check(Store.Save(First), "Save: 保存应成功");
        Check(First.ApplicationID == "app-1", "Save: 首个编号应为 app-1");

        SellerApplication Second = MakePending("u002", "李四", 2000);
        Check(Store.Save(Second), "Save: 换个申请人保存应成功");
        Check(Second.ApplicationID == "app-2", "Save: 第二个编号应为 app-2");
        Check(Store.Size() == 2, "Save: 应存下两条");
    }
    {
        InMemorySellerApplicationStore Store;
        SellerApplication Application = MakePending("u001", "张三", 1000);
        Store.Save(Application);

        // 审核结论要写回同一条，不能变成第二条记录
        auto Found = Store.FindByID(Application.ApplicationID);
        Check(Found.has_value(), "FindByID: 应能查到刚存的申请");
        SellerApplication Audited = Found.value();
        Check(Audited.Reject("信息不全", "admin", 2000), "测试装置: 驳回应成功");
        Check(Store.Save(Audited), "Save: 覆盖保存应成功");
        Check(Store.Size() == 1, "Save: 覆盖不应新增记录");
        Check(Audited.ApplicationID == "app-1", "Save: 覆盖时编号不应变化");

        auto Reloaded = Store.FindByID(Application.ApplicationID);
        Check(Reloaded.has_value(), "FindByID: 覆盖后仍应能查到");
        Check(Reloaded->AuditStatus == ApplicationStatus::Rejected,
              "FindByID: 应读到写回的审核结论");
        Check(Reloaded->RejectReason.value() == "信息不全", "FindByID: 应读到驳回理由");
        Check(Reloaded->ReviewerID.value() == "admin", "FindByID: 应读到审核人");
    }
    {
        InMemorySellerApplicationStore Store;
        Check(!Store.FindByID("app-404").has_value(), "FindByID: 不存在的编号应返回空");
        Check(!Store.FindByID("").has_value(), "FindByID: 空编号应返回空");
    }

    // ================= 6. 一人只能有一条申请 =================
    {
        InMemorySellerApplicationStore Store;
        SellerApplication First = MakePending("u001", "张三", 1000);
        Check(Store.Save(First), "一人一条: 首条应能保存");

        SellerApplication Second = MakePending("u001", "张三", 2000);
        Check(!Store.Save(Second), "一人一条: 同一申请人第二条应被拒绝");
        Check(Second.ApplicationID.empty(), "一人一条: 被拒绝的申请不应分到编号");
        Check(Store.Size() == 1, "一人一条: 库里应仍只有一条");
        Check(Store.HasApplication("u001"), "HasApplication: 已申请过应为 true");
        Check(!Store.HasApplication("u002"), "HasApplication: 没申请过应为 false");
        Check(!Store.HasApplication(""), "HasApplication: 空申请人应为 false");
    }
    {
        // 已经驳回也算「申请过」，不能重新交一份
        InMemorySellerApplicationStore Store;
        SellerApplication First = MakePending("u001", "张三", 1000);
        Store.Save(First);
        SellerApplication Audited = First;
        Check(Audited.Reject("信息不全", "admin", 2000), "测试装置: 驳回应成功");
        Check(Store.Save(Audited), "一人一条: 写回审核结论应成功");

        SellerApplication Again = MakePending("u001", "张三", 3000);
        Check(!Store.Save(Again), "一人一条: 驳回后也不应再申请");
        Check(Store.HasApplication("u001"), "HasApplication: 驳回后仍算申请过");
        Check(Store.Size() == 1, "一人一条: 库里应仍只有一条");
    }
    {
        // 已经通过同样不能再申请
        InMemorySellerApplicationStore Store;
        SellerApplication First = MakePending("u001", "张三", 1000);
        Store.Save(First);
        SellerApplication Audited = First;
        Check(Audited.Approve("admin", 2000), "测试装置: 通过应成功");
        Store.Save(Audited);

        SellerApplication Again = MakePending("u001", "张三", 3000);
        Check(!Store.Save(Again), "一人一条: 通过后也不应再申请");
    }

    // ================= 7. 按申请人查询 =================
    {
        InMemorySellerApplicationStore Store;
        Check(Store.ListByApplicant("u001").empty(), "ListByApplicant: 没申请过应返回空");
        Check(Store.ListByApplicant("").empty(), "ListByApplicant: 空申请人应返回空而非全部");
        Check(!Store.FindLatestByApplicant("u001").has_value(),
              "FindLatestByApplicant: 没申请过应返回空");
        Check(!Store.FindLatestByApplicant("").has_value(),
              "FindLatestByApplicant: 空申请人应返回空");
        Check(!Store.FindPendingByApplicant("u001").has_value(),
              "FindPendingByApplicant: 没申请过应返回空");
        Check(!Store.FindPendingByApplicant("").has_value(),
              "FindPendingByApplicant: 空申请人应返回空");
    }
    {
        InMemorySellerApplicationStore Store;
        SellerApplication Mine = MakePending("u001", "张三", 1000);
        SellerApplication Other = MakePending("u002", "李四", 2000);
        Store.Save(Mine);
        Store.Save(Other);

        auto Found = Store.ListByApplicant("u001");
        Check(Found.size() == 1, "ListByApplicant: 只应取到本人的申请");
        Check(Found[0].ApplicationID == Mine.ApplicationID,
              "ListByApplicant: 应取到本人的那一条");

        auto Latest = Store.FindLatestByApplicant("u001");
        Check(Latest.has_value() && Latest->ApplicationID == Mine.ApplicationID,
              "FindLatestByApplicant: 应取本人的申请");

        auto Pending = Store.FindPendingByApplicant("u001");
        Check(Pending.has_value() && Pending->ApplicationID == Mine.ApplicationID,
              "FindPendingByApplicant: 应找到待审核的那条");

        // 审核完就不再是待审核
        SellerApplication Audited = Pending.value();
        Check(Audited.Reject("信息不全", "admin", 3000), "测试装置: 驳回应成功");
        Store.Save(Audited);
        Check(!Store.FindPendingByApplicant("u001").has_value(),
              "FindPendingByApplicant: 已驳回的不应再算待审核");
    }

    // ================= 8. 按状态查询与计数 =================
    {
        InMemorySellerApplicationStore Store;

        SellerApplication PendingOne = MakePending("u001", "张三", 1000);
        Store.Save(PendingOne);
        SellerApplication PendingTwo = MakePending("u002", "李四", 2000);
        Store.Save(PendingTwo);

        SellerApplication Approved = MakePending("u003", "王五", 3000);
        Check(Approved.Approve("admin", 4000), "测试装置: 通过应成功");
        Store.Save(Approved);

        SellerApplication Rejected = MakePending("u004", "赵六", 5000);
        Check(Rejected.Reject("信息不全", "admin", 6000), "测试装置: 驳回应成功");
        Store.Save(Rejected);

        auto PendingList = Store.ListByStatus(ApplicationStatus::PendingReview);
        Check(PendingList.size() == 2, "ListByStatus: 待审核应有两条");
        Check(PendingList[0].SubmittedAtMs == 1000 && PendingList[1].SubmittedAtMs == 2000,
              "ListByStatus: 应按提交时间升序");

        Check(Store.CountByStatus(ApplicationStatus::PendingReview) == 2,
              "CountByStatus: 待审核应为 2");
        Check(Store.CountByStatus(ApplicationStatus::Approved) == 1,
              "CountByStatus: 通过应为 1");
        Check(Store.CountByStatus(ApplicationStatus::Rejected) == 1,
              "CountByStatus: 驳回应为 1");
        Check(Store.Size() == 4, "Size: 应存下四条");
    }

    // ================= 9. 结果与库的隔离 =================
    {
        InMemorySellerApplicationStore Store;
        SellerApplication Application = MakePending("u001", "张三", 1000);
        Store.Save(Application);

        // 改外面查到的副本，库里的不应跟着变
        auto Found = Store.FindByID(Application.ApplicationID);
        Check(Found.has_value(), "测试装置: 应能查到刚存的申请");
        Check(Found->Approve("admin", 2000), "隔离: 改副本应成功");

        auto Reloaded = Store.FindByID(Application.ApplicationID);
        Check(Reloaded.has_value(), "隔离: 覆盖前仍应能查到");
        Check(Reloaded->AuditStatus == ApplicationStatus::PendingReview,
              "隔离: 改查询结果不应影响库里的记录");
        Check(Application.AuditStatus == ApplicationStatus::PendingReview,
              "隔离: 原对象与查询结果应是两份拷贝");
    }
}
