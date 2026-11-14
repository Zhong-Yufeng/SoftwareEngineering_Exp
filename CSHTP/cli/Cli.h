#pragma once
#include<string>
#include<vector>
#include<optional>

#include"User.h"
#include"Item.h"
#include"Reservation.h"
#include"SellerApplication.h"
#include"AuditRecord.h"
#include"TradeRecord.h"
#include"OperationLog.h"

#include"InMemoryUserStore.h"
#include"InMemoryItemStore.h"
#include"InMemoryReservationStore.h"
#include"InMemorySellerApplicationStore.h"
#include"InMemoryAuditRecordStore.h"
#include"InMemoryTradeRecordStore.h"
#include"InMemoryOperationLogStore.h"

#include"LogService.h"
#include"LoginService.h"
#include"BuyerService.h"
#include"SellerService.h"
#include"AdminService.h"

namespace cli
{
// 控制台界面：只做「显示 + 取输入 + 调 service」，
// 不碰 core 工厂、不碰仓储实现（只有启动时造种子数据例外）。
// 主流程在 run()：登录/注册 → 按角色进管理员界面或买卖界面 → 退出登录。
class Cli
{
    public:
        Cli();

        int run();

    private:
        // ==================== 会话 ====================
        std::optional<User> CurrentUser;   // 空表示未登录

        // ==================== 仓储（内存版） ====================
        InMemoryUserStore Users;
        InMemorySellerApplicationStore Applications;
        InMemoryItemStore Items;
        InMemoryAuditRecordStore Audits;
        InMemoryReservationStore Reservations;
        InMemoryTradeRecordStore Trades;
        InMemoryOperationLogStore Logs;

        // ==================== 服务 ====================
        LogService Log;
        LoginService Login;
        BuyerService Buyer;
        SellerService Seller;
        AdminService Admin;

        // ==================== 顶层流程 ====================
        void SeedDemoData();
        bool LoginScreen();
        void RegisterFlow();
        void AdminMenu();
        void TraderMenu();

        // ==================== 买家功能 ====================
        void BrowseItems();
        void ShowItemDetail(const Item& Goods);
        void MyReservations();
        void MyTrades();
        void SellerApplyFlow();

        // ==================== 卖家功能 ====================
        void PublishItem();
        void MyItems();
        void SellerReservations();
        void CompleteTradeFlow();
        void OffShelfFlow();

        // ==================== 管理员功能 ====================
        void AuditApplications();
        void AuditItems();
        void UserList();
        void LogQueryScreen();
        void PlatformStatsScreen();

        // ==================== 显示与输入 ====================
        void ShowBanner();
        void ShowAccounts();
        void Section(const std::string& Text);
        void Divider();
        void Pause();
        std::string Ask(const std::string& Prompt);
        std::string AskRequired(const std::string& Prompt);
        int AskChoice(const std::string& Prompt);
        void ShowNotice(const std::string& Message);
        void ShowFailure(const std::string& Message);
        void ShowSellerStatus();
        void PrintItems(const std::vector<Item>& Goods);

        static std::string PriceText(long long PriceCents);
        static std::string TimeText(long long Milliseconds);
        static std::string RoleText(Role Value);
        static std::string ItemStatusText(ItemStatus Value);
        static std::string CategoryText(Category Value);
        static std::string ConditionText(Condition Value);
        static std::string ReservationStatusText(ReservationStatus Value);
        static std::string TradeStatusText(TradeStatus Value);
        static std::string ApplicationStatusText(ApplicationStatus Value);
        static std::string LogTypeText(LogType Value);
};
}
