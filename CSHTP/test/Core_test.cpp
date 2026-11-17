#include"test.h"
#include"User.h"
#include"Item.h"
#include"Reservation.h"
#include"SellerApplication.h"
#include"AuditRecord.h"
#include"OperationLog.h"

using Test::Check;

namespace
{
    ItemInfo MakeInfo()
    {
        ItemInfo Info;
        Info.Title = "二手教材";
        Info.Description = "九成新，无笔记";
        Info.ItemCategory = Category::Textbook;
        Info.PriceCents = 2000;
        Info.ItemCondition = Condition::LikeNew;
        Info.TradeLocation = "一食堂门口";
        Info.Contact = "QQ 10001";
        return Info;
    }
}

void RunCoreTests()
{
    {
        auto UserA = User::create("13800000001", "241220001", "123456", "小明", "10001");
        Check(UserA.has_value(), "User: 手机号与学号齐全可注册");
        Check(UserA && UserA->UserID.empty(), "User: 编号由存储层分配");
        Check(UserA && UserA->UserRole == Role::Buyer, "User: 新用户是买家");
        Check(UserA && UserA->UserPhone == "13800000001", "User: 手机号被记录");
        Check(UserA && UserA->UserStdNo == "241220001", "User: 学号被记录");

        auto NoAccount = User::create("", "", "123456", "小明", "10001");
        Check(!NoAccount.has_value(), "User: 手机号和学号不能都为空");

        auto NoPhone = User::create("", "241220002", "123456", "小明", "10001");
        Check(!NoPhone.has_value(), "User: 缺手机号拒绝");

        auto NoStdNo = User::create("13800000005", "", "123456", "小明", "10001");
        Check(!NoStdNo.has_value(), "User: 缺学号拒绝");

        auto ShortPassword = User::create("13800000002", "241220003", "12345", "小明", "10001");
        Check(!ShortPassword.has_value(), "User: 密码少于 6 位拒绝");

        auto NoNickname = User::create("13800000003", "241220004", "123456", "", "10001");
        Check(!NoNickname.has_value(), "User: 昵称为空拒绝");

        auto NoContact = User::create("13800000004", "241220005", "123456", "小明", "");
        Check(!NoContact.has_value(), "User: 联系方式为空拒绝");
    }

    {
        auto Admin = User::createAdmin("admin", "admin123", "管理员");
        Check(Admin.has_value(), "User: 管理员可创建");
        Check(Admin && Admin->UserRole == Role::Admin, "User: 管理员角色正确");
        Check(Admin && Admin->UserPhone == "admin", "User: 管理员编号同时占手机号");
    }

    {
        auto UserA = User::create("13800000010", "241220010", "123456", "小明", "10001");
        Check(UserA.has_value(), "User: 状态流转前置");
        if (UserA)
        {
            Check(UserA->BecomeSeller(), "User: 买家可升级为卖家");
            Check(!UserA->BecomeSeller(), "User: 卖家不能再次升级");
            Check(!UserA->Ban(""), "User: 封禁理由为空失败");
            Check(UserA->Ban("违规"), "User: 合法封禁成功");
            Check(UserA->Status == AccountStatus::Banned, "User: 状态为已封禁");
            Check(UserA->Reason.has_value() && UserA->Reason.value() == "违规", "User: 封禁理由记录");
            Check(UserA->Unban(), "User: 解封成功");
            Check(UserA->Status == AccountStatus::Normal && !UserA->Reason.has_value(), "User: 解封后状态正常");
            Check(!UserA->Unban(), "User: 正常用户不能再次解封");
        }
    }

    {
        auto UserA = User::create("13800000020", "241220020", "123456", "小明", "10001");
        Check(UserA.has_value(), "User: 资料修改前置");
        if (UserA)
        {
            Check(!UserA->UpdateProfile("", "10002"), "User: 昵称为空不能修改资料");
            Check(!UserA->UpdateProfile("新昵称", ""), "User: 联系方式为空不能修改资料");
            Check(UserA->UpdateProfile("新昵称", "10002"), "User: 合法资料可修改");
            Check(UserA->Nickname == "新昵称" && UserA->QQNo == "10002", "User: 资料已更新");

            Check(!UserA->ChangePassword("wrong", "654321"), "User: 旧密码错误不能修改");
            Check(!UserA->ChangePassword("123456", "12345"), "User: 新密码少于 6 位拒绝");
            Check(UserA->ChangePassword("123456", "654321"), "User: 旧密码正确可改密码");
            Check(UserA->Password == "654321", "User: 密码已更新");

            Check(UserA->UserID.empty(), "User: 未分编号前不能重复分配");
            Check(UserA->AssignID("user-9"), "User: 编号为空时可分配编号");
            Check(UserA->UserID == "user-9", "User: 编号已写入");
            Check(!UserA->AssignID("user-10"), "User: 已有编号不能再分配");
        }
    }

    {
        const ItemInfo Info = MakeInfo();
        auto Draft = Item::CreateDraft(Info, "seller-1");
        Check(Draft.has_value(), "Item: 合法内容可创建草稿");
        Check(Draft && Draft->Status == ItemStatus::Draft, "Item: 初始状态为草稿");
        Check(Draft && Draft->ItemID.empty(), "Item: 编号由存储层分配");

        ItemInfo Bad = Info;
        Bad.PriceCents = 0;
        Check(!Item::CreateDraft(Bad, "seller-1").has_value(), "Item: 价格必须大于 0");
        Bad = Info;
        Bad.TradeLocation.clear();
        Check(!Item::CreateDraft(Bad, "seller-1").has_value(), "Item: 交易地点不能为空");
        Check(!Item::CreateDraft(Info, "").has_value(), "Item: 卖家编号不能为空");

        if (Draft)
        {
            Item Goods = Draft.value();
            Check(Goods.IsContentComplete(), "Item: 合法内容判定完整");
            Check(Goods.IsVisibleTo(Role::Buyer, "seller-1"), "Item: 草稿对卖家可见");
            Check(!Goods.IsVisibleTo(Role::Buyer, "buyer-1"), "Item: 草稿对买家不可见");
            Check(!Goods.IsVisibleTo(Role::Admin, "admin"), "Item: 草稿对管理员不可见");

            Check(!Goods.Confirm(), "Item: 草稿不能直接确认");

            Check(Goods.SubmitForReview(), "Item: 草稿可提交审核");
            Check(Goods.Status == ItemStatus::PendingReview, "Item: 提交后为待审核");
            Check(Goods.IsVisibleTo(Role::Admin, "admin"), "Item: 待审核对管理员可见");
            Check(!Goods.IsVisibleTo(Role::Buyer, "buyer-1"), "Item: 待审核对普通买家不可见");
            Check(!Goods.SubmitForReview(), "Item: 待审核不能重复提交");

            Check(!Goods.Approve(0), "Item: 审核时间非法失败");
            Check(Goods.Approve(1000), "Item: 管理员通过审核");
            Check(Goods.Status == ItemStatus::Published && Goods.PublishedAtMs == 1000, "Item: 发布状态与时间正确");
            Check(Goods.IsVisibleTo(Role::Buyer, "buyer-1"), "Item: 已发布对买家可见");
            Check(!Goods.Approve(1001), "Item: 已发布不能再审核");

            Check(Goods.Confirm(), "Item: 已发布可确认");
            Check(Goods.Status == ItemStatus::Confirmed, "Item: 确认后为已确认");
        }
    }

    {
        const ItemInfo Info = MakeInfo();
        auto Draft = Item::CreateDraft(Info, "seller-2");
        Check(Draft.has_value(), "Item: 驳回流程前置");
        if (Draft)
        {
            Item Goods = Draft.value();
            Check(Goods.SubmitForReview(), "Item: 提交审核");
            Check(!Goods.Reject(""), "Item: 驳回理由不能为空");
            Check(Goods.Reject("信息不全"), "Item: 待审核可驳回");
            Check(Goods.Status == ItemStatus::Rejected, "Item: 驳回状态正确");
            Check(Goods.RejectReason.has_value(), "Item: 驳回理由记录");
            Check(Goods.SubmitForReview(), "Item: 驳回后可重新提交");
            Check(Goods.Approve(2000), "Item: 重新提交后可发布");
            Check(Goods.Confirm(), "Item: 发布后可确认");
            Check(Goods.CompleteTrade(), "Item: 已确认可完成交易");
            Check(Goods.Status == ItemStatus::Traded, "Item: 完成交易状态正确");
            Check(!Goods.Confirm(), "Item: 交易完成不能再次确认");
        }
    }

    {
        auto Order = Reservation::Create("buyer-1", "item-1", 1000);
        Check(Order.has_value(), "Reservation: 合法参数可创建");
        Check(Order && Order->Status == ReservationStatus::PendingConfirm, "Reservation: 初始待确认");
        if (Order)
        {
            Check(!Order->Confirm(0), "Reservation: 确认时间非法失败");
            Check(Order->Confirm(2000), "Reservation: 待确认可确认");
            Check(Order->Status == ReservationStatus::Succeeded, "Reservation: 确认后已成功");
            Check(Order->ConfirmedAtMs.has_value() && Order->ConfirmedAtMs.value() == 2000, "Reservation: 确认时间记录");
            Check(!Order->Confirm(2001), "Reservation: 已成功不能重复确认");
        }
        Check(!Reservation::Create("", "item-1", 1000).has_value(), "Reservation: 买家为空拒绝");
        Check(!Reservation::Create("buyer-1", "", 1000).has_value(), "Reservation: 商品为空拒绝");
        Check(!Reservation::Create("buyer-1", "item-1", 0).has_value(), "Reservation: 时间非法拒绝");
    }

    {
        auto Application = SellerApplication::Create("buyer-1", "张三", 1000);
        Check(Application.has_value(), "SellerApplication: 合法参数可创建");
        Check(Application && Application->AuditStatus == ApplicationStatus::PendingReview, "SellerApplication: 初始待审核");
        if (Application)
        {
            Check(!Application->Approve("", 2000), "SellerApplication: 审核人为空失败");
            Check(Application->Approve("admin", 2000), "SellerApplication: 待审核可通过");
            Check(Application->AuditStatus == ApplicationStatus::Approved, "SellerApplication: 通过状态正确");
            Check(!Application->Approve("admin", 2001), "SellerApplication: 已审核不能重复通过");
            Check(!Application->Reject("理由", "admin", 2002), "SellerApplication: 已审核不能驳回");
        }

        auto Rejected = SellerApplication::Create("buyer-2", "李四", 1000);
        if (Rejected)
        {
            Check(!Rejected->Reject("", "admin", 2000), "SellerApplication: 驳回理由不能为空");
            Check(Rejected->Reject("资料不全", "admin", 2000), "SellerApplication: 待审核可驳回");
            Check(Rejected->AuditStatus == ApplicationStatus::Rejected, "SellerApplication: 驳回状态正确");
        }
    }

    {
        auto Record = AuditRecord::Create("item-1", "admin", AuditResult::Approved, 1000, "");
        Check(Record.has_value(), "AuditRecord: 通过审核可不填理由");
        Check(Record && !Record->Reason.has_value(), "AuditRecord: 通过时理由为空");
        Check(!AuditRecord::Create("item-1", "admin", AuditResult::Rejected, 1000, "").has_value(),
              "AuditRecord: 驳回必须填写理由");
        auto Rejected = AuditRecord::Create("item-1", "admin", AuditResult::Rejected, 1000, "违规");
        Check(Rejected.has_value() && Rejected->Reason.value() == "违规", "AuditRecord: 驳回理由记录");
    }

    {
        auto Log = OperationLog::Create(LogType::Register, "user-1", "User", "user-1", "", 1000);
        Check(Log.has_value(), "OperationLog: 说明可为空");
        Check(!OperationLog::Create(LogType::Register, "", "User", "user-1", "", 1000).has_value(),
              "OperationLog: 操作人不能为空");
        Check(!OperationLog::Create(LogType::Register, "user-1", "User", "", "", 1000).has_value(),
              "OperationLog: 对象编号不能为空");
        Check(!OperationLog::Create(LogType::Register, "user-1", "User", "user-1", "", 0).has_value(),
              "OperationLog: 时间非法拒绝");
    }
}
