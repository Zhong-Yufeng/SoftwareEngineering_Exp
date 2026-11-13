#pragma once
#include<cstddef>
#include<optional>
#include<string>
#include<vector>
#include"ServiceCommon.h"
#include"UserStore.h"
#include"SellerApplicationStore.h"
#include"ItemStore.h"
#include"AuditRecordStore.h"
#include"ReservationStore.h"
#include"TradeRecordStore.h"
#include"LogService.h"

// REQ-022 平台报表：时间窗内的发布量、预约量、完成交易量
struct PlatformStats
{
    std::size_t PublishedCount = 0;
    std::size_t ReservationCount = 0;
    std::size_t CompletedTradeCount = 0;
};

// 管理员用例：卖家认证审核、商品审核与违规处置、封禁/解封、平台报表、记录查询。
class AdminService
{
    public:
        AdminService(UserStore& UserStoreRef,
                     SellerApplicationStore& ApplicationStore,
                     ItemStore& ItemStoreRef,
                     AuditRecordStore& AuditStore,
                     ReservationStore& ReservationStoreRef,
                     TradeRecordStore& TradeRecordStoreRef,
                     LogService& LogStore,
                     ServiceClock Clock = SystemTimeMs);

        // ==================== 管理员账号 ====================

        // 管理员账号由系统发放，不接受自助注册（REQ-004）；
        // 编号同时占手机号、学号与联系方式，所以不会和买家账号撞车。
        // 登录不在这里：所有角色的登录统一走 LoginService
        std::optional<User> IssueAdminAccount(const std::string& AdminID,
                                              const std::string& Password,
                                              const std::string& Nickname,
                                              std::string* Error = nullptr);

        // ==================== 卖家认证审核 ====================

        // 审核前先按编号取一条申请，看清内容再决定
        std::optional<SellerApplication> FindApplication(const std::string& AdminID,
                                                         const std::string& ApplicationID,
                                                         std::string* Error = nullptr) const;

        // 只能处理待审核的申请；已通过或已驳回的再次处理会失败
        bool ApproveApplication(const std::string& AdminID,
                                const std::string& ApplicationID,
                                std::string* Error = nullptr);

        bool RejectApplication(const std::string& AdminID,
                               const std::string& ApplicationID,
                               const std::string& Reason,
                               std::string* Error = nullptr);

        std::vector<SellerApplication> ListApplications(
            std::optional<ApplicationStatus> Status = std::nullopt) const;

        // ==================== 商品审核与违规处置 ====================

        bool ApproveItem(const std::string& AdminID,
                         const std::string& ItemID,
                         std::string* Error = nullptr);

        bool RejectItem(const std::string& AdminID,
                        const std::string& ItemID,
                        const std::string& Reason,
                        std::string* Error = nullptr);

        std::vector<AuditRecord> ListAuditRecords(const std::string& AdminID,
                                                  const std::string& ItemID,
                                                  std::string* Error = nullptr) const;

        std::optional<Item> FindItem(const std::string& AdminID,
                                     const std::string& ItemID,
                                     std::string* Error = nullptr) const;

        std::vector<Item> QueryItems(const ItemQuery& Cond,
                                     const std::string& AdminID) const;

        std::vector<Reservation> ListItemReservations(const std::string& AdminID,
                                                      const std::string& ItemID,
                                                      std::string* Error = nullptr) const;

        std::vector<TradeRecord> ListItemTrades(const std::string& AdminID,
                                                const std::string& ItemID,
                                                std::string* Error = nullptr) const;

        // ==================== 用户 ====================

        // 管理员查看全部账号；非管理员调用返回空列表
        std::vector<User> ListUsers(const std::string& AdminID,
                                    std::string* Error = nullptr) const; 

        // ==================== 操作日志 ====================

        // REQ-021：日志只允许管理员查询，买家与卖家都没有入口
        bool QueryLogs(const std::string& AdminID,
                       const LogQuery& Cond,
                       std::vector<OperationLog>* Result,
                       std::string* Error = nullptr) const; 
        // ==================== 汇总统计 ====================

        // 发布量查商品表；预约量查「发起预约」日志（预约记录会被删除，不能以预约表为准）；
        // 完成交易量按交易记录「已完成」状态统计（等同于商品状态为交易完成）
        PlatformStats GetPlatformStats(long long FromMs, long long ToMs) const; 

        // ----- 以下函数暂未实现 ----------------------------------------
        bool BanUser(const std::string& AdminID,
                     const std::string& TargetUserID,
                     const std::string& Reason,
                     std::string* Error = nullptr);

        bool UnbanUser(const std::string& AdminID,
                       const std::string& TargetUserID,
                       std::string* Error = nullptr);

        bool ViolationOffShelfItem(const std::string& AdminID,
                                   const std::string& ItemID,
                                   const std::string& Reason,
                                   std::string* Error = nullptr);

    private:
        UserStore& Users;
        SellerApplicationStore& Applications;
        ItemStore& Items;
        AuditRecordStore& Audits;
        ReservationStore& Reservations;
        TradeRecordStore& Trades;
        LogService& Logs;
        ServiceClock Clock;
};
