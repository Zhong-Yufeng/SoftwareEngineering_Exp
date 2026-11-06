#include"test.h"
#include"Item.h"
#include<optional>
#include<string>
#include<vector>

// ============================================================
// Item 状态机的测试
//
// 字段私有之后，测试不能再「把 Status 直接改成待审核」来摆场景，
// 只能走真实路径：CreateDraft → SubmitForReview → ... 这反而是好事，
// 测的是用户真能走的路，而不是一个被手工摆出来的假状态。
//
// 覆盖：
//   1. 草稿态下每个 Can* 的答案
//   2. 提交审核、撤回、通过、驳回的正向路径
//   3. 非法调用被拒，且失败时字段一个都没被改（「要么完整迁移，要么原样不动」）
//   4. 不变式：驳回理由只在驳回态有值
// ============================================================

using Test::Check;

namespace {

std::optional<Item> MakeDraft()
{
    return Item::CreateDraft("高数教材", "第七版，九成新", Category::Textbook,
                             1500, Condition::LikeNew,
                             "三食堂门口", "13800000000", 3, "seller-1");
}

}  // namespace

void RunItemStateTests()
{
    // 前置条件：合法输入必须能造出草稿，否则后面所有断言都无从谈起。
    // 放在开头统一检查一次，后面就能放心用 *MakeDraft() 取值。
    {
        auto Probe = MakeDraft();
        Check(Probe.has_value(), "状态机: 前置——合法输入应能造出草稿");
        if (!Probe)
        {
            return;
        }
    }

    // ============ 1. 草稿态：能做什么、不能做什么 ============
    {
        Item I = *MakeDraft();

        Check(I.GetStatus() == ItemStatus::Draft,       "草稿: 状态应为 Draft");
        Check(I.GetPublishedAtMs() == 0,                "草稿: 发布时间应为 0");

        Check(I.CanEdit(),                              "草稿: 应允许编辑");
        Check(I.CanSubmitForReview(),                   "草稿: 应允许提交审核");
        Check(I.CanDelete(),                            "草稿: 应允许删除");

        Check(!I.CanApprove(),                          "草稿: 不能审核通过");
        Check(!I.CanReject(),                           "草稿: 不能审核驳回");
        Check(!I.CanWithdrawReview(),                   "草稿: 不能撤回（还没提交）");
        Check(!I.CanReserve(),                          "草稿: 不能被预约");

        Check(I.IsContentComplete(),                    "草稿: 内容应判定为齐全");
    }

    // ============ 2. 提交审核：Draft → PendingReview ============
    {
        Item I = *MakeDraft();

        Check(I.SubmitForReview(),                      "提交: 草稿提交应成功");
        Check(I.GetStatus() == ItemStatus::PendingReview, "提交: 状态应变为 PendingReview");
        Check(!I.GetRejectReason().has_value(),         "提交: 不应带驳回理由");

        Check(I.CanApprove(),                           "待审核: 应允许通过");
        Check(I.CanReject(),                            "待审核: 应允许驳回");
        Check(I.CanWithdrawReview(),                    "待审核: 应允许撤回");
        Check(!I.CanEdit(),                             "待审核: 不允许编辑（内容冻结）");
        Check(!I.CanDelete(),                           "待审核: 不允许直接删除");
        Check(!I.CanSubmitForReview(),                  "待审核: 不能重复提交");
        Check(!I.CanReserve(),                          "待审核: 不能被预约");

        // 重复提交必须被拒，且不能把状态改成别的
        Check(!I.SubmitForReview(),                     "待审核: 重复提交应失败");
        Check(I.GetStatus() == ItemStatus::PendingReview,
              "待审核: 重复提交失败后状态不变");
    }

    // ============ 3. 撤回：PendingReview → Draft ============
    {
        Item I = *MakeDraft();
        I.SubmitForReview();

        Check(I.WithdrawReview(),                       "撤回: 待审核时应成功");
        Check(I.GetStatus() == ItemStatus::Draft,       "撤回: 状态应回到 Draft");
        Check(I.CanEdit(),                              "撤回: 回到草稿后应可编辑");

        // 撤回后再提交，应当能走通
        Check(I.SubmitForReview(),                      "撤回后重提: 应成功");
        Check(I.GetStatus() == ItemStatus::PendingReview, "撤回后重提: 应回到待审核");
    }

    // ============ 4. 审核通过：PendingReview → Published ============
    {
        Item I = *MakeDraft();
        I.SubmitForReview();

        Check(I.Approve(12345),                         "通过: 待审核应能通过");
        Check(I.GetStatus() == ItemStatus::Published,   "通过: 状态应为 Published");
        Check(I.GetPublishedAtMs() == 12345,            "通过: 发布时间应等于传入的时间");
        Check(!I.GetRejectReason().has_value(),         "通过: 不应带驳回理由");

        Check(I.CanReserve(),                           "已发布: 可被预约");
        Check(!I.CanApprove(),                          "已发布: 不能再次通过");
        Check(!I.CanReject(),                           "已发布: 不能驳回");
        Check(!I.CanEdit(),                             "已发布: 不允许编辑");
        Check(!I.CanDelete(),                           "已发布: 不允许直接删除");
        Check(!I.CanSubmitForReview(),                  "已发布: 不能提交审核");
        Check(!I.CanWithdrawReview(),                   "已发布: 不能撤回");

        // 失败必须不改字段：状态、发布时间都得原样
        Check(!I.SubmitForReview(),                     "已发布: 提交审核应失败");
        Check(I.GetStatus() == ItemStatus::Published,   "已发布: 失败后状态不变");
        Check(I.GetPublishedAtMs() == 12345,            "已发布: 失败后发布时间不变");
    }

    // ============ 5. 审核驳回：PendingReview → Rejected ============
    {
        Item I = *MakeDraft();
        I.SubmitForReview();

        Check(!I.Reject(""),                            "驳回: 空理由应被拒");
        Check(I.GetStatus() == ItemStatus::PendingReview,
              "驳回: 空理由被拒后状态不变");
        Check(!I.GetRejectReason().has_value(),         "驳回: 空理由被拒后不应留下理由");

        Check(I.Reject("图片看不清"),                   "驳回: 合法理由应成功");
        Check(I.GetStatus() == ItemStatus::Rejected,    "驳回: 状态应为 Rejected");
        Check(I.GetRejectReason().has_value()
              && *I.GetRejectReason() == "图片看不清",  "驳回: 理由应被记下");

        Check(I.CanEdit(),                              "驳回: 应允许编辑");
        Check(I.CanSubmitForReview(),                   "驳回: 应允许重新提交");
        Check(I.CanDelete(),                            "驳回: 应允许删除");
        Check(!I.CanApprove(),                          "驳回: 不能直接通过");

        // 重提必须清空旧理由，否则会违反 (Status==Rejected) ⇔ 理由有值
        Check(I.SubmitForReview(),                      "驳回后重提: 应成功");
        Check(I.GetStatus() == ItemStatus::PendingReview, "驳回后重提: 应回到待审核");
        Check(!I.GetRejectReason().has_value(),         "驳回后重提: 旧理由应被清空");
    }

    // ============ 6. 编号只能分配一次 ============
    {
        Item I = *MakeDraft();

        Check(I.GetItemID().empty(),                    "编号: 新建商品编号应为空");
        Check(!I.AssignID(""),                          "编号: 空编号应被拒");
        Check(I.AssignID("item-1"),                     "编号: 首次分配应成功");
        Check(I.GetItemID() == "item-1",                "编号: 应等于分配的值");
        Check(!I.AssignID("item-2"),                    "编号: 重复分配应被拒");
        Check(I.GetItemID() == "item-1",                "编号: 重复分配失败后编号不变");
    }

    // ============ 7. 可见性：4 个状态 × 4 种视角 ============
    // 规则表见 Item.h 中 IsVisibleTo 的注释，这里逐格验证。
    // 「其他卖家」一列专门用来抓「只看角色不看编号」的实现错误：
    // 它和「该卖家」的 Role 完全一样，只有编号不同。
    {
        Item I = *MakeDraft();

        Check(I.IsVisibleTo(Role::Seller, "seller-1"),   "可见性/草稿: 该卖家可见");
        Check(!I.IsVisibleTo(Role::Seller, "seller-2"),  "可见性/草稿: 其他卖家不可见");
        Check(!I.IsVisibleTo(Role::Buyer, "buyer-1"),    "可见性/草稿: 买家不可见");
        Check(!I.IsVisibleTo(Role::Admin, "admin-1"),    "可见性/草稿: 管理员不可见");
    }
    {
        Item I = *MakeDraft();
        I.SubmitForReview();

        Check(I.IsVisibleTo(Role::Seller, "seller-1"),   "可见性/待审核: 该卖家可见");
        Check(!I.IsVisibleTo(Role::Seller, "seller-2"),  "可见性/待审核: 其他卖家不可见");
        Check(!I.IsVisibleTo(Role::Buyer, "buyer-1"),    "可见性/待审核: 买家不可见");
        Check(I.IsVisibleTo(Role::Admin, "admin-1"),     "可见性/待审核: 管理员可见");
    }
    {
        Item I = *MakeDraft();
        I.SubmitForReview();
        I.Reject("图片不清晰");

        Check(I.IsVisibleTo(Role::Seller, "seller-1"),   "可见性/驳回: 该卖家可见");
        Check(!I.IsVisibleTo(Role::Seller, "seller-2"),  "可见性/驳回: 其他卖家不可见");
        Check(!I.IsVisibleTo(Role::Buyer, "buyer-1"),    "可见性/驳回: 买家不可见");
        Check(!I.IsVisibleTo(Role::Admin, "admin-1"),    "可见性/驳回: 管理员不可见");
    }
    {
        Item I = *MakeDraft();
        I.SubmitForReview();
        I.Approve(1000);

        Check(I.IsVisibleTo(Role::Seller, "seller-1"),   "可见性/已发布: 该卖家可见");
        Check(I.IsVisibleTo(Role::Seller, "seller-2"),   "可见性/已发布: 其他卖家可见");
        Check(I.IsVisibleTo(Role::Buyer, "buyer-1"),     "可见性/已发布: 买家可见");
        Check(!I.IsVisibleTo(Role::Admin, "admin-1"),    "可见性/已发布: 管理员不可见");
    }

    // 同一个人，状态变了结论也要跟着变。这条防的是「把可见性写成
    // 与状态无关的常量」——只看第一组是发现不了这种错误的。
    {
        Item I = *MakeDraft();
        Check(!I.IsVisibleTo(Role::Buyer, "buyer-1"),    "可见性/跨状态: 草稿对买家不可见");
        I.SubmitForReview();
        I.Approve(1000);
        Check(I.IsVisibleTo(Role::Buyer, "buyer-1"),     "可见性/跨状态: 发布后对买家可见");
    }
}
