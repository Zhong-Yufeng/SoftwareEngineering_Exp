#include"test.h"
#include"AuditRecord.h"
#include"InMemoryAuditRecordStore.h"

// ============================================================
// 审核记录的测试
//
// 覆盖两层：
//   AuditRecord::Create 的校验与不变式（通过不带理由、驳回必带理由）
//   InMemoryAuditRecordStore 的存储、两种维度的查询、时间排序、最近一条
// ============================================================

using Test::Check;

namespace
{
    AuditRecord MakeRecord(const std::string& Item,
                           const std::string& Reviewer,
                           AuditResult Result,
                           long long Ms,
                           const std::string& Reason = "")
    {
        auto R = AuditRecord::Create(Item, Reviewer, Result, Ms, Reason);
        Check(R.has_value(), "测试装置: 造一条审核记录应当成功");
        return R.value();
    }
}

void RunAuditRecordTests()
{
    // ================= 1. Create 的校验 =================
    {
        Check(!AuditRecord::Create("", "admin-1", AuditResult::Approved, 1000, "").has_value(),
              "Create: 空商品编号应被拒");
        Check(!AuditRecord::Create("item-1", "", AuditResult::Approved, 1000, "").has_value(),
              "Create: 空审核人应被拒");
        Check(!AuditRecord::Create("item-1", "admin-1", AuditResult::Approved, 0, "").has_value(),
              "Create: 时间为 0 应被拒");
        Check(!AuditRecord::Create("item-1", "admin-1", AuditResult::Approved, -5, "").has_value(),
              "Create: 时间为负应被拒");
    }

    // ================= 2. 驳回必须带理由 =================
    {
        Check(!AuditRecord::Create("item-1", "admin-1", AuditResult::Rejected, 1000, "").has_value(),
              "Create/驳回: 空理由应被拒");

        auto R = AuditRecord::Create("item-1", "admin-1", AuditResult::Rejected, 1000, "描述与实物不符");
        Check(R.has_value(), "Create/驳回: 带理由应成功");
        if (R)
        {
            Check(R->Result == AuditResult::Rejected, "Create/驳回: 结论应为驳回");
            Check(R->Reason.has_value() && *R->Reason == "描述与实物不符",
                  "Create/驳回: 理由应被记下");
            Check(R->RecordID.empty(), "Create/驳回: 编号应留空给存储层");
            Check(R->AuditedAtMs == 1000, "Create/驳回: 时间应一致");
        }
    }

    // ================= 3. 通过时理由被丢弃 =================
    {
        auto R = AuditRecord::Create("item-1", "admin-1", AuditResult::Approved, 1000, "顺手写点啥");
        Check(R.has_value(), "Create/通过: 应成功");
        if (R)
        {
            Check(R->Result == AuditResult::Approved, "Create/通过: 结论应为通过");
            Check(!R->Reason.has_value(), "Create/通过: 即使传了理由也应被清空");
        }
    }

    // ================= 4. 保存与编号 =================
    {
        InMemoryAuditRecordStore Store;

        AuditRecord A = MakeRecord("item-1", "admin-1", AuditResult::Rejected, 1000, "标题太短");
        Check(Store.Save(A), "Save: 第一条应保存成功");
        Check(A.RecordID == "aud-1", "Save: 第一条应分配 aud-1");

        AuditRecord B = MakeRecord("item-1", "admin-1", AuditResult::Approved, 2000);
        Check(Store.Save(B), "Save: 第二条应保存成功");
        Check(B.RecordID == "aud-2", "Save: 第二条应分配 aud-2");

        Check(Store.Size() == 2, "Save: 库中应有 2 条");
    }

    // ================= 5. 带编号覆盖 =================
    {
        InMemoryAuditRecordStore Store;

        AuditRecord A = MakeRecord("item-1", "admin-1", AuditResult::Approved, 1000);
        A.RecordID = "aud-special";
        Check(Store.Save(A), "覆盖: 带编号应保存成功");
        Check(Store.Size() == 1, "覆盖: 第一次保存后应只有 1 条");

        AuditRecord B = MakeRecord("item-2", "admin-2", AuditResult::Approved, 2000);
        B.RecordID = "aud-special";
        Check(Store.Save(B), "覆盖: 同编号再存应成功");
        Check(Store.Size() == 1, "覆盖: 同编号两次保存仍应只有 1 条");

        auto Found = Store.FindByID("aud-special");
        Check(Found.has_value() && Found->ItemID == "item-2",
              "覆盖: 查到的应是后写入的那条");
    }

    // ================= 6. 按编号查 =================
    {
        InMemoryAuditRecordStore Store;

        Store.Save(MakeRecord("item-1", "admin-1", AuditResult::Rejected, 1234, "信息不全"));

        auto Found = Store.FindByID("aud-1");
        Check(Found.has_value(), "FindByID: 存在的编号应查得到");
        Check(Found->ItemID == "item-1", "FindByID: 商品应一致");
        Check(Found->ReviewerID == "admin-1", "FindByID: 审核人应一致");
        Check(Found->AuditedAtMs == 1234, "FindByID: 时间应一致");
        Check(Found->Reason.has_value() && *Found->Reason == "信息不全", "FindByID: 理由应一致");

        Check(!Store.FindByID("aud-404").has_value(), "FindByID: 不存在的编号应查不到");
        Check(!Store.FindByID("").has_value(), "FindByID: 空编号应查不到");
    }

    // ================= 7. 按商品列出，且按时间升序 =================
    {
        InMemoryAuditRecordStore Store;

        // 故意乱序存：先存时间晚的，再存时间早的
        Store.Save(MakeRecord("item-1", "admin-1", AuditResult::Rejected, 3000, "第二次驳回"));
        Store.Save(MakeRecord("item-1", "admin-2", AuditResult::Rejected, 1000, "第一次驳回"));
        Store.Save(MakeRecord("item-1", "admin-1", AuditResult::Approved, 2000));
        Store.Save(MakeRecord("item-2", "admin-1", AuditResult::Approved, 1500));

        auto History = Store.ListByItem("item-1");
        Check(History.size() == 3, "ListByItem: 该商品应有 3 条记录");
        Check(History.size() == 3 && History[0].AuditedAtMs == 1000, "ListByItem: 第一条应是最早的");
        Check(History.size() == 3 && History[1].AuditedAtMs == 2000, "ListByItem: 第二条应是中间的");
        Check(History.size() == 3 && History[2].AuditedAtMs == 3000, "ListByItem: 最后一条应是最新的");

        bool AllSameItem = true;
        for (const auto& Record : History)
        {
            if (Record.ItemID != "item-1")
            {
                AllSameItem = false;
            }
        }
        Check(AllSameItem, "ListByItem: 每条记录都应是该商品");

        Check(Store.ListByItem("item-9").empty(), "ListByItem: 没审过的商品应返回空");
        Check(Store.ListByItem("").empty(), "ListByItem: 空商品应返回空，而不是全部");
    }

    // ================= 8. 按审核人列出 =================
    {
        InMemoryAuditRecordStore Store;

        Store.Save(MakeRecord("item-1", "admin-1", AuditResult::Approved, 1000));
        Store.Save(MakeRecord("item-2", "admin-1", AuditResult::Approved, 3000));
        Store.Save(MakeRecord("item-3", "admin-2", AuditResult::Approved, 2000));

        auto Mine = Store.ListByReviewer("admin-1");
        Check(Mine.size() == 2, "ListByReviewer: admin-1 应有 2 条");
        Check(Mine.size() == 2 && Mine[0].AuditedAtMs == 1000, "ListByReviewer: 应按时间升序");
        Check(Mine.size() == 2 && Mine[1].AuditedAtMs == 3000, "ListByReviewer: 最后一条最新");

        Check(Store.ListByReviewer("admin-9").empty(), "ListByReviewer: 没审过的管理员应返回空");
        Check(Store.ListByReviewer("").empty(), "ListByReviewer: 空审核人应返回空");
    }

    // ================= 9. 最近一次审核 =================
    {
        InMemoryAuditRecordStore Store;

        Check(!Store.FindLatestByItem("item-1").has_value(), "FindLatestByItem: 空库应返回空");

        Store.Save(MakeRecord("item-1", "admin-1", AuditResult::Rejected, 1000, "第一次驳回"));
        Store.Save(MakeRecord("item-1", "admin-2", AuditResult::Approved, 5000));
        Store.Save(MakeRecord("item-1", "admin-1", AuditResult::Rejected, 3000, "第二次驳回"));

        auto Latest = Store.FindLatestByItem("item-1");
        Check(Latest.has_value(), "FindLatestByItem: 有记录时应查得到");
        Check(Latest.has_value() && Latest->AuditedAtMs == 5000, "FindLatestByItem: 应取时间最大的那条");
        Check(Latest.has_value() && Latest->ReviewerID == "admin-2", "FindLatestByItem: 应是 admin-2 那次");
        Check(Latest.has_value() && !Latest->Reason.has_value(),
              "FindLatestByItem: 通过那次不应带理由");

        Check(!Store.FindLatestByItem("item-9").has_value(), "FindLatestByItem: 没审过的商品应返回空");
        Check(!Store.FindLatestByItem("").has_value(), "FindLatestByItem: 空商品应返回空");
    }

    // ================= 10. 全部记录 =================
    {
        InMemoryAuditRecordStore Store;

        Store.Save(MakeRecord("item-2", "admin-1", AuditResult::Approved, 2000));
        Store.Save(MakeRecord("item-1", "admin-2", AuditResult::Rejected, 1000, "不行"));

        auto All = Store.All();
        Check(All.size() == 2, "All: 应返回全部 2 条");
        Check(All.size() == 2 && All[0].AuditedAtMs == 1000, "All: 应按时间升序");
        Check(All.size() == 2 && All[1].AuditedAtMs == 2000, "All: 最后一条最新");
    }

    // ================= 11. 隔离 =================
    {
        InMemoryAuditRecordStore Store;

        AuditRecord A = MakeRecord("item-1", "admin-1", AuditResult::Approved, 1000);
        Store.Save(A);

        A.ReviewerID = "admin-999";
        auto Found = Store.FindByID("aud-1");
        Check(Found.has_value() && Found->ReviewerID == "admin-1",
              "隔离: 改原对象不应影响库里的记录");

        auto All = Store.All();
        if (!All.empty())
        {
            All[0].ItemID = "item-999";
        }
        auto Again = Store.FindByID("aud-1");
        Check(Again.has_value() && Again->ItemID == "item-1",
              "隔离: 改查询结果不应影响库里的记录");
    }

    // ================= 12. 初始状态 =================
    {
        InMemoryAuditRecordStore Store;
        Check(Store.Size() == 0, "初始: 新建的库应为空");
        Check(Store.All().empty(), "初始: 空库 All 应返回空");
    }
}
