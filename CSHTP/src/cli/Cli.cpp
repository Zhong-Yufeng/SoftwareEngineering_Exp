#include"Cli.h"

#include<cmath>
#include<cstdlib>
#include<ctime>
#include<filesystem>
#include<iomanip>
#include<iostream>
#include<sstream>

#ifdef _WIN32
#include<windows.h>
#endif

namespace cli
{
namespace
{
    std::string Trim(std::string Text) // 函数功能: 把字符串首尾的空白字符去掉
    {
        std::size_t Begin = 0;
        while (Begin < Text.size() && (Text[Begin] == ' ' || Text[Begin] == '\t'))
        {
            ++Begin;
        }
        Text = Text.substr(Begin);

        while (!Text.empty())
        {
            const char Last = Text.back();
            if (Last == '\r' || Last == '\n' || Last == ' ' || Last == '\t')
            {
                Text.pop_back();
                continue;
            }
            break;
        }
        return Text;
    }
}

Cli::Cli()
    : Users(Db),
      Applications(Db),
      Items(Db),
      Audits(Db),
      Reservations(Db),
      Trades(Db),
      Logs(Db),
      Log(Logs),
      Login(Users),
      Buyer(Users, Applications, Items, Reservations, Trades, Log, Db),
      Seller(Users, Items, Reservations, Trades, Log, Db),
      Admin(Users, Applications, Items, Audits, Reservations, Trades, Log, Db)
{
}

// ===================== 主流程 =====================

int Cli::run()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 数据库文件放在程序工作目录下的 data 目录里，首次运行时自动创建
    std::error_code Ignored;
    std::filesystem::create_directories("data", Ignored);
    if (!Db.Open("data/cshtp.db"))
    {
        std::cout << "\n[错误] 无法打开数据库 data/cshtp.db：" << Db.LastError() << "\n";
        return 1;
    }

    SeedDemoData();
    while (true)
    {
        if (!LoginScreen())
        {
            break;
        }
        if (!CurrentUser.has_value())
        {
            continue;   // 注册成功只回到登录页
        }

        if (CurrentUser->UserRole == Role::Admin)
        {
            AdminMenu();
        }
        else
        {
            TraderMenu();
        }

        CurrentUser.reset();   // 退出登录
    }

    std::cout << "\n已退出 CSHTP 校园二手交易平台。\n";
    return 0;
}

void Cli::SeedDemoData()
{
    // 数据库里已经有账号就不再重复写入，避免每次启动都把演示数据插一遍
    if (Users.Size() > 0)
    {
        return;
    }

    std::string Error;
    const auto Note = [](const std::string& Text)
    {
        std::cout << "[种子数据] " << Text << "\n";
    };

    const auto AdminUser = Admin.IssueAdminAccount("admin", "admin123", "平台管理员", &Error);
    if (!AdminUser.has_value())
    {
        Note("管理员账号创建失败：" + Error);
        return;
    }
    const std::string AdminID = AdminUser->UserID;

    const auto Wang = Buyer.Register("13800000001", "241220001", "123456", "王同学", "100001", &Error);
    const auto Li = Buyer.Register("13800000002", "241220002", "123456", "李同学", "100002", &Error);
    const auto Chen = Buyer.Register("13800000003", "241220003", "123456", "陈同学", "100003", &Error);
    if (!Wang.has_value() || !Li.has_value() || !Chen.has_value())
    {
        Note("演示用户创建失败：" + Error);
        return;
    }

    // 王、李注册后申请卖家认证，管理员通过；演示账号的初始角色由此得到
    for (const auto& Pair : {std::make_pair(Wang->UserID, std::string("王小明")),
                             std::make_pair(Li->UserID, std::string("李小红"))})
    {
        const auto Application = Buyer.ApplyForSeller(Pair.first, Pair.second, &Error);
        if (!Application.has_value())
        {
            Note("卖家认证申请失败：" + Error);
            continue;
        }
        if (!Admin.ApproveApplication(AdminID, Application->ApplicationID, &Error))
        {
            Note("卖家认证审核失败：" + Error);
        }
    }

    const auto MakeItem = [this, &AdminID, &Error](const std::string& SellerID,
                                                   const std::string& Title,
                                                   const std::string& Description,
                                                   Category ItemCategory,
                                                   long long PriceCents,
                                                   Condition ItemCondition,
                                                   const std::string& TradeLocation,
                                                   const std::string& Contact,
                                                   bool Publish)
    {
        ItemInfo Info;
        Info.Title = Title;
        Info.Description = Description;
        Info.ItemCategory = ItemCategory;
        Info.PriceCents = PriceCents;
        Info.ItemCondition = ItemCondition;
        Info.TradeLocation = TradeLocation;
        Info.Contact = Contact;

        const auto Draft = Seller.CreateDraft(SellerID, Info, &Error);
        if (!Draft.has_value())
        {
            return false;
        }
        if (!Seller.SubmitForReview(SellerID, Draft->ItemID, &Error))
        {
            return false;
        }
        if (Publish && !Admin.ApproveItem(AdminID, Draft->ItemID, &Error))
        {
            return false;
        }
        return true;
    };

    const std::string WangID = Wang->UserID;
    const std::string LiID = Li->UserID;

    if (!MakeItem(WangID, "高等数学（第七版）教材", "大一高数教材，笔记很少，无破损",
                  Category::Textbook, 1200, Condition::LikeNew, "东区食堂门口", "QQ 100001", true) ||
        !MakeItem(WangID, "罗技 M185 无线鼠标", "自用一年，功能正常，送电池",
                  Category::Digital, 3500, Condition::LikeNew, "图书馆一楼大厅", "QQ 100001", true) ||
        !MakeItem(LiID, "LED 护眼台灯", "三档亮度，灯罩有一道划痕，不影响使用",
                  Category::DailyGoods, 2000, Condition::Flawed, "西区宿舍 3 号楼", "QQ 100002", true) ||
        !MakeItem(LiID, "优衣库连帽卫衣 L 码", "全新未拆吊牌，买大了一号",
                  Category::Clothing, 4500, Condition::BrandNew, "南门快递点", "QQ 100002", true))
    {
        Note("演示商品创建失败：" + Error);
        return;
    }

    // 留一件待审核商品给管理员界面演示
    if (!MakeItem(WangID, "87 键机械键盘", "青轴，带原装包装",
                  Category::Digital, 12000, Condition::LikeNew, "东区食堂门口", "QQ 100001", false))
    {
        Note("待审核商品创建失败：" + Error);
        return;
    }
}

// ===================== 登录与注册 =====================

bool Cli::LoginScreen()
{
    ShowBanner();
    ShowAccounts();
    std::cout << " 1. 登录\n"
              << " 2. 注册\n"
              << " 0. 退出\n";

    const int Choice = AskChoice("请选择");
    if (Choice == 0)
    {
        return false;
    }
    if (Choice == 2)
    {
        RegisterFlow();
        return true;
    }
    if (Choice != 1)
    {
        ShowNotice("没有这个选项");
        return true;
    }

    Section("登录");
    std::cout << "买家可用手机号或学号登录；管理员用管理员账号登录。\n";
    const std::string Account = AskRequired("账号");
    const std::string Password = AskRequired("密码");

    std::string Error;
    const auto Logged = Login.Login(Account, Password, &Error);
    if (!Logged.has_value())
    {
        ShowFailure(Error.empty() ? "登录失败" : Error);
        Pause();
        return true;
    }

    CurrentUser = Logged;
    ShowNotice("登录成功，欢迎 " + Logged->Nickname + "（" + RoleText(Logged->UserRole) + "）");
    Pause();
    return true;
}

void Cli::RegisterFlow()
{
    Section("注册");
    std::cout << "昵称、QQ、密码为必填；手机号与学号至少填写一项。\n";

    const std::string Nickname = AskRequired("昵称");
    const std::string QQNo = AskRequired("QQ 号");
    const std::string Password = AskRequired("密码（至少 6 位）");
    const std::string Phone = Ask("手机号（可留空）");
    const std::string StdNo = Ask("学号（可留空）");

    std::string Error;
    const auto Created = Buyer.Register(Phone, StdNo, Password, Nickname, QQNo, &Error);
    if (!Created.has_value())
    {
        ShowFailure(Error.empty() ? "注册失败" : Error);
    }
    else
    {
        ShowNotice("注册成功，请返回登录页用手机号或学号登录。");
    }
    Pause();
}

// ===================== 买卖界面 =====================

void Cli::TraderMenu()
{
    while (true)
    {
        ShowBanner();
        std::cout << "当前用户：" << CurrentUser->Nickname
                  << "（" << RoleText(CurrentUser->UserRole) << "）"
                  << "   编号 " << CurrentUser->UserID << "\n";

        Divider();
        Section("买家功能");
        std::cout << " 1. 浏览与搜索商品\n"
                  << " 2. 我的预约（可取消）\n"
                  << " 3. 我的交易记录\n"
                  << " 4. 申请成为卖家 / 查看认证状态\n";

        Divider();
        Section("卖家功能");
        const bool IsSeller = CurrentUser->UserRole == Role::Seller;
        if (IsSeller)
        {
            std::cout << " 5. 发布商品\n"
                      << " 6. 我的商品（提交审核）\n"
                      << " 7. 查看预约并确认\n"
                      << " 8. 完成交易\n"
                      << " 9. 下架商品\n";
        }
        else
        {
            ShowSellerStatus();
        }

        Divider();
        std::cout << " 0. 退出登录\n";

        const int Choice = AskChoice("请选择");
        switch (Choice)
        {
        case 0:
            return;
        case 1:
            BrowseItems();
            break;
        case 2:
            MyReservations();
            break;
        case 3:
            MyTrades();
            break;
        case 4:
            SellerApplyFlow();
            break;
        case 5:
            if (IsSeller) { PublishItem(); } else { ShowNotice("该功能仅卖家可用"); Pause(); }
            break;
        case 6:
            if (IsSeller) { MyItems(); } else { ShowNotice("该功能仅卖家可用"); Pause(); }
            break;
        case 7:
            if (IsSeller) { SellerReservations(); } else { ShowNotice("该功能仅卖家可用"); Pause(); }
            break;
        case 8:
            if (IsSeller) { CompleteTradeFlow(); } else { ShowNotice("该功能仅卖家可用"); Pause(); }
            break;
        case 9:
            if (IsSeller) { OffShelfFlow(); } else { ShowNotice("该功能仅卖家可用"); Pause(); }
            break;
        default:
            ShowNotice("没有这个选项");
            break;
        }
    }
}

void Cli::ShowSellerStatus()
{
    const auto Application = Applications.FindByApplicant(CurrentUser->UserID);
    if (!Application.has_value())
    {
        std::cout << "状态：尚未申请卖家认证，可在买家功能 4 提交申请。\n";
        return;
    }

    switch (Application->AuditStatus)
    {
    case ApplicationStatus::PendingReview:
        std::cout << "状态：卖家认证审核中，通过后本栏功能自动开放。\n";
        break;
    case ApplicationStatus::Rejected:
        std::cout << "状态：卖家认证已驳回";
        if (Application->RejectReason.has_value())
        {
            std::cout << "（" << Application->RejectReason.value() << "）";
        }
        std::cout << "，本版本不支持重新申请。\n";
        break;
    case ApplicationStatus::Approved:
        std::cout << "状态：卖家认证已通过。\n";
        break;
    }
}

// ===================== 买家功能 =====================

void Cli::BrowseItems()
{
    Section("浏览与搜索商品");
    const std::string Keyword = Ask("关键字（直接回车表示全部）");

    ItemQuery Cond;
    Cond.Status = ItemStatus::Published;
    if (!Keyword.empty())
    {
        Cond.Keyword = Keyword;
    }

    const auto Goods = Buyer.QueryItems(Cond, CurrentUser->UserID);
    PrintItems(Goods);
    if (Goods.empty())
    {
        Pause();
        return;
    }

    const int Choice = AskChoice("输入编号查看详情并发起预约（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Goods.size())
    {
        return;
    }
    ShowItemDetail(Goods[static_cast<std::size_t>(Choice - 1)]);
}

void Cli::ShowItemDetail(const Item& Goods)
{
    Section("商品详情");
    std::cout << "标题：" << Goods.Title << "\n"
              << "价格：" << PriceText(Goods.PriceCents) << "\n"
              << "分类/成色：" << CategoryText(Goods.ItemCategory)
              << " / " << ConditionText(Goods.ItemCondition) << "\n"
              << "交易地点：" << Goods.TradeLocation << "\n"
              << "描述：" << Goods.Description << "\n"
              << "状态：" << ItemStatusText(Goods.Status) << "\n";
    const auto Owner = Users.FindByID(Goods.SellerID);
    std::cout << "卖家：" << (Owner.has_value() ? Owner->Nickname : Goods.SellerID) << "\n"
              << "联系方式：成交后由交易记录给出\n";

    if (Goods.Status != ItemStatus::Published)
    {
        ShowNotice("该商品当前不可预约。");
        Pause();
        return;
    }

    if (AskChoice("1. 发起预约   0. 返回") != 1)
    {
        return;
    }

    std::string Error;
    const auto Order = Buyer.Reserve(CurrentUser->UserID, Goods.ItemID, &Error);
    if (!Order.has_value())
    {
        ShowFailure(Error.empty() ? "预约失败" : Error);
    }
    else
    {
        ShowNotice("预约成功，预约编号 " + Order->ReservationID +
                   "，状态：" + ReservationStatusText(Order->Status));
    }
    Pause();
}

void Cli::MyReservations()
{
    Section("我的预约");
    const auto Orders = Buyer.ListMyReservations(CurrentUser->UserID);
    if (Orders.empty())
    {
        ShowNotice("还没有预约记录。");
        Pause();
        return;
    }

    for (std::size_t Index = 0; Index < Orders.size(); ++Index)
    {
        const auto Goods = Items.FindByID(Orders[Index].ItemID);
        std::cout << std::setw(3) << (Index + 1) << ". "
                  << (Goods.has_value() ? Goods->Title : Orders[Index].ItemID)
                  << "  [" << ReservationStatusText(Orders[Index].Status) << "]"
                  << "  创建于 " << TimeText(Orders[Index].CreatedAtMs) << "\n";
    }

    const int Choice = AskChoice("输入编号取消该预约（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Orders.size())
    {
        return;
    }

    const Reservation& Target = Orders[static_cast<std::size_t>(Choice - 1)];
    std::string Error;
    if (!Buyer.CancelReservation(CurrentUser->UserID, Target.ItemID, Target.ReservationID, &Error))
    {
        ShowFailure(Error.empty() ? "取消失败" : Error);
    }
    else
    {
        ShowNotice("已取消预约。");
    }
    Pause();
}

void Cli::MyTrades()
{
    Section("我的交易记录");
    const auto Records = Buyer.ListMyTrades(CurrentUser->UserID);
    if (Records.empty())
    {
        ShowNotice("还没有交易记录。");
        Pause();
        return;
    }

    for (const TradeRecord& Record : Records)
    {
        std::cout << "交易编号：" << Record.TradeID << "\n"
                  << "  商品：" << Record.ItemTitle
                  << "   价格：" << PriceText(Record.PriceCents) << "\n"
                  << "  状态：" << TradeStatusText(Record.Status)
                  << "   交易地点：" << Record.TradeLocation << "\n"
                  << "  卖家联系方式：" << Record.SellerContact << "\n"
                  << "  生成于 " << TimeText(Record.CreatedAtMs);
        if (Record.CompletedAtMs > 0)
        {
            std::cout << "   完成于 " << TimeText(Record.CompletedAtMs);
        }
        std::cout << "\n";
    }
    Pause();
}

void Cli::SellerApplyFlow()
{
    Section("卖家认证");
    const auto Existing = Applications.FindByApplicant(CurrentUser->UserID);
    if (Existing.has_value())
    {
        std::cout << "状态：" << ApplicationStatusText(Existing->AuditStatus) << "\n"
                  << "真实姓名：" << Existing->RealName << "\n"
                  << "提交时间：" << TimeText(Existing->SubmittedAtMs) << "\n";
        if (Existing->RejectReason.has_value())
        {
            std::cout << "驳回原因：" << Existing->RejectReason.value() << "\n";
        }
        ShowNotice("每位用户只能提交一次申请，结果以管理员审核为准。");
        Pause();
        return;
    }

    const std::string RealName = AskRequired("真实姓名");
    std::string Error;
    const auto Application = Buyer.ApplyForSeller(CurrentUser->UserID, RealName, &Error);
    if (!Application.has_value())
    {
        ShowFailure(Error.empty() ? "提交失败" : Error);
    }
    else
    {
        ShowNotice("已提交卖家认证申请，编号 " + Application->ApplicationID + "，等待管理员审核。");
    }
    Pause();
}

// ===================== 卖家功能 =====================

void Cli::PublishItem()
{
    Section("发布商品");
    ItemInfo Info;

    Info.Title = AskRequired("标题");
    Info.Description = AskRequired("描述");

    std::cout << "分类：1 教材   2 数码   3 服饰   4 生活用品\n";
    switch (AskChoice("分类"))
    {
    case 1: Info.ItemCategory = Category::Textbook; break;
    case 2: Info.ItemCategory = Category::Digital; break;
    case 3: Info.ItemCategory = Category::Clothing; break;
    case 4: Info.ItemCategory = Category::DailyGoods; break;
    default: ShowNotice("分类无效，已取消发布。"); Pause(); return;
    }

    std::cout << "成色：1 全新   2 九成新   3 有瑕疵\n";
    switch (AskChoice("成色"))
    {
    case 1: Info.ItemCondition = Condition::BrandNew; break;
    case 2: Info.ItemCondition = Condition::LikeNew; break;
    case 3: Info.ItemCondition = Condition::Flawed; break;
    default: ShowNotice("成色无效，已取消发布。"); Pause(); return;
    }

    long long PriceCents = 0;
    while (PriceCents <= 0)
    {
        const std::string Text = AskRequired("价格（元）");
        try
        {
            const double Yuan = std::stod(Text);
            if (Yuan > 0 && Yuan < 1000000)
            {
                PriceCents = static_cast<long long>(std::llround(Yuan * 100.0));
            }
        }
        catch (...)
        {
            PriceCents = 0;
        }
        if (PriceCents <= 0)
        {
            ShowFailure("价格必须是正的数字");
        }
    }
    Info.PriceCents = PriceCents;

    Info.TradeLocation = AskRequired("交易地点");
    Info.Contact = AskRequired("联系方式（QQ 或手机号）");

    std::string Error;
    const auto Draft = Seller.CreateDraft(CurrentUser->UserID, Info, &Error);
    if (!Draft.has_value())
    {
        ShowFailure(Error.empty() ? "发布失败" : Error);
        Pause();
        return;
    }

    ShowNotice("已保存为草稿，商品编号 " + Draft->ItemID + "。");
    if (AskChoice("1. 立即提交审核   0. 先不提交") != 1)
    {
        Pause();
        return;
    }

    if (!Seller.SubmitForReview(CurrentUser->UserID, Draft->ItemID, &Error))
    {
        ShowFailure(Error.empty() ? "提交审核失败" : Error);
    }
    else
    {
        ShowNotice("已提交审核，等待管理员处理。");
    }
    Pause();
}

void Cli::MyItems()
{
    Section("我的商品");
    const auto Goods = Seller.ListMyItems(CurrentUser->UserID);
    if (Goods.empty())
    {
        ShowNotice("还没有发布过商品。");
        Pause();
        return;
    }

    PrintItems(Goods);
    for (std::size_t Index = 0; Index < Goods.size(); ++Index)
    {
        if (Goods[Index].RejectReason.has_value())
        {
            std::cout << "    驳回原因：" << Goods[Index].RejectReason.value() << "\n";
        }
    }

    const int Choice = AskChoice("输入编号提交审核（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Goods.size())
    {
        return;
    }

    std::string Error;
    const Item& Target = Goods[static_cast<std::size_t>(Choice - 1)];
    if (!Seller.SubmitForReview(CurrentUser->UserID, Target.ItemID, &Error))
    {
        ShowFailure(Error.empty() ? "提交审核失败" : Error);
    }
    else
    {
        ShowNotice("已提交审核。");
    }
    Pause();
}

void Cli::SellerReservations()
{
    Section("查看预约并确认");
    const auto Goods = Seller.ListMyItems(CurrentUser->UserID);
    if (Goods.empty())
    {
        ShowNotice("还没有发布过商品。");
        Pause();
        return;
    }

    PrintItems(Goods);
    const int ItemChoice = AskChoice("选择商品（0 返回）");
    if (ItemChoice <= 0 || static_cast<std::size_t>(ItemChoice) > Goods.size())
    {
        return;
    }

    const Item& Target = Goods[static_cast<std::size_t>(ItemChoice - 1)];
    std::string Error;
    const auto Orders = Seller.ListItemReservations(CurrentUser->UserID, Target.ItemID, &Error);
    if (Orders.empty())
    {
        ShowNotice(Error.empty() ? "该商品暂无预约。" : Error);
        Pause();
        return;
    }

    std::cout << "商品：" << Target.Title << "（" << ItemStatusText(Target.Status) << "）\n";
    for (std::size_t Index = 0; Index < Orders.size(); ++Index)
    {
        const auto BuyerUser = Users.FindByID(Orders[Index].BuyerID);
        std::cout << std::setw(3) << (Index + 1) << ". "
                  << (BuyerUser.has_value() ? BuyerUser->Nickname : Orders[Index].BuyerID)
                  << "  [" << ReservationStatusText(Orders[Index].Status) << "]"
                  << "  创建于 " << TimeText(Orders[Index].CreatedAtMs) << "\n";
    }

    const int OrderChoice = AskChoice("输入编号确认该预约（0 返回）");
    if (OrderChoice <= 0 || static_cast<std::size_t>(OrderChoice) > Orders.size())
    {
        return;
    }

    const Reservation& Order = Orders[static_cast<std::size_t>(OrderChoice - 1)];
    if (!Seller.ConfirmReservation(CurrentUser->UserID, Target.ItemID, Order.ReservationID, &Error))
    {
        ShowFailure(Error.empty() ? "确认失败" : Error);
    }
    else
    {
        ShowNotice("已确认预约，商品转入「已确认」，其余待确认预约已删除，交易记录已生成。");
    }
    Pause();
}

void Cli::CompleteTradeFlow()
{
    Section("完成交易");
    const auto Mine = Seller.ListMyItems(CurrentUser->UserID);

    std::vector<Item> Confirmed;
    for (const Item& Goods : Mine)
    {
        if (Goods.Status == ItemStatus::Confirmed)
        {
            Confirmed.push_back(Goods);
        }
    }
    if (Confirmed.empty())
    {
        ShowNotice("没有处于「已确认」状态的商品。");
        Pause();
        return;
    }

    PrintItems(Confirmed);
    const int Choice = AskChoice("输入编号完成交易（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Confirmed.size())
    {
        return;
    }

    std::string Error;
    const Item& Target = Confirmed[static_cast<std::size_t>(Choice - 1)];
    if (!Seller.CompleteTrade(CurrentUser->UserID, Target.ItemID, &Error))
    {
        ShowFailure(Error.empty() ? "完成交易失败" : Error);
    }
    else
    {
        ShowNotice("交易已完成，商品状态转为「交易完成」。");
    }
    Pause();
}

void Cli::OffShelfFlow()
{
    Section("下架商品");
    std::vector<Item> Goods;
    for (const Item& Mine : Seller.ListMyItems(CurrentUser->UserID))
    {
        if (Mine.Status == ItemStatus::Published || Mine.Status == ItemStatus::Confirmed)
        {
            Goods.push_back(Mine);
        }
    }
    if (Goods.empty())
    {
        ShowNotice("没有可下架的已发布/已确认商品。");
        Pause();
        return;
    }

    PrintItems(Goods);
    const int Choice = AskChoice("输入编号下架（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Goods.size())
    {
        return;
    }

    std::string Error;
    const Item& Target = Goods[static_cast<std::size_t>(Choice - 1)];
    if (!Seller.OffShelfItem(CurrentUser->UserID, Target.ItemID, &Error))
    {
        ShowFailure(Error.empty() ? "下架功能尚未实现" : Error);
    }
    else
    {
        ShowNotice("商品已下架。");
    }
    Pause();
}

// ===================== 管理员功能 =====================

void Cli::AdminMenu()
{
    while (true)
    {
        ShowBanner();
        std::cout << "当前管理员：" << CurrentUser->Nickname
                  << "   编号 " << CurrentUser->UserID << "\n";
        Divider();
        Section("管理员功能");
        std::cout << " 1. 卖家认证审核\n"
                  << " 2. 商品审核（通过 / 驳回）\n"
                  << " 3. 用户列表\n"
                  << " 4. 操作日志\n"
                  << " 5. 平台统计\n"
                  << " 6. 商品违规下架（尚未实现）\n"
                  << " 7. 封禁 / 解封（尚未实现）\n"
                  << " 0. 退出登录\n";

        const int Choice = AskChoice("请选择");
        switch (Choice)
        {
        case 0:
            return;
        case 1:
            AuditApplications();
            break;
        case 2:
            AuditItems();
            break;
        case 3:
            UserList();
            break;
        case 4:
            LogQueryScreen();
            break;
        case 5:
            PlatformStatsScreen();
            break;
        case 6:
            ShowNotice("违规下架功能尚未实现。");
            Pause();
            break;
        case 7:
            ShowNotice("封禁 / 解封功能尚未实现。");
            Pause();
            break;
        default:
            ShowNotice("没有这个选项");
            break;
        }
    }
}

void Cli::AuditApplications()
{
    Section("卖家认证审核");
    const auto Pending = Admin.ListApplications(ApplicationStatus::PendingReview);
    if (Pending.empty())
    {
        ShowNotice("没有待审核的卖家认证申请。");
        Pause();
        return;
    }

    for (std::size_t Index = 0; Index < Pending.size(); ++Index)
    {
        const auto Applicant = Users.FindByID(Pending[Index].ApplicantID);
        std::cout << std::setw(3) << (Index + 1) << ". "
                  << (Applicant.has_value() ? Applicant->Nickname : Pending[Index].ApplicantID)
                  << "   真实姓名：" << Pending[Index].RealName
                  << "   编号 " << Pending[Index].ApplicationID
                  << "   提交于 " << TimeText(Pending[Index].SubmittedAtMs) << "\n";
    }

    const int Choice = AskChoice("输入编号处理（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Pending.size())
    {
        return;
    }

    std::string Error;
    const auto Application = Admin.FindApplication(CurrentUser->UserID,
                                                   Pending[static_cast<std::size_t>(Choice - 1)].ApplicationID,
                                                   &Error);
    if (!Application.has_value())
    {
        ShowFailure(Error.empty() ? "无法读取申请" : Error);
        Pause();
        return;
    }

    std::cout << "\n申请人：" << Application->ApplicantID
              << "   真实姓名：" << Application->RealName << "\n";

    const int Decision = AskChoice("1. 通过   2. 驳回   0. 返回");
    if (Decision == 1)
    {
        if (Admin.ApproveApplication(CurrentUser->UserID, Application->ApplicationID, &Error))
        {
            ShowNotice("已通过，申请人角色转为卖家。");
        }
        else
        {
            ShowFailure(Error.empty() ? "审核失败" : Error);
        }
    }
    else if (Decision == 2)
    {
        const std::string Reason = AskRequired("驳回原因");
        if (Admin.RejectApplication(CurrentUser->UserID, Application->ApplicationID, Reason, &Error))
        {
            ShowNotice("已驳回。");
        }
        else
        {
            ShowFailure(Error.empty() ? "审核失败" : Error);
        }
    }
    Pause();
}

void Cli::AuditItems()
{
    Section("商品审核");
    ItemQuery Cond;
    Cond.Status = ItemStatus::PendingReview;
    const auto Pending = Admin.QueryItems(Cond, CurrentUser->UserID);
    if (Pending.empty())
    {
        ShowNotice("没有待审核的商品。");
        Pause();
        return;
    }

    PrintItems(Pending);
    const int Choice = AskChoice("输入编号处理（0 返回）");
    if (Choice <= 0 || static_cast<std::size_t>(Choice) > Pending.size())
    {
        return;
    }

    const Item& Goods = Pending[static_cast<std::size_t>(Choice - 1)];
    const auto Owner = Users.FindByID(Goods.SellerID);
    std::cout << "\n标题：" << Goods.Title << "\n"
              << "卖家：" << (Owner.has_value() ? Owner->Nickname : Goods.SellerID) << "\n"
              << "价格：" << PriceText(Goods.PriceCents) << "\n"
              << "描述：" << Goods.Description << "\n";

    std::string Error;
    const int Decision = AskChoice("1. 通过   2. 驳回   0. 返回");
    if (Decision == 1)
    {
        if (Admin.ApproveItem(CurrentUser->UserID, Goods.ItemID, &Error))
        {
            ShowNotice("已通过，商品状态转为「已发布」。");
        }
        else
        {
            ShowFailure(Error.empty() ? "审核失败" : Error);
        }
    }
    else if (Decision == 2)
    {
        const std::string Reason = AskRequired("驳回原因");
        if (Admin.RejectItem(CurrentUser->UserID, Goods.ItemID, Reason, &Error))
        {
            ShowNotice("已驳回，卖家可在「我的商品」看到原因。");
        }
        else
        {
            ShowFailure(Error.empty() ? "审核失败" : Error);
        }
    }
    Pause();
}

void Cli::UserList()
{
    Section("用户列表");
    std::string Error;
    const auto UsersList = Admin.ListUsers(CurrentUser->UserID, &Error);
    if (UsersList.empty())
    {
        ShowFailure(Error.empty() ? "没有用户" : Error);
        Pause();
        return;
    }

    for (const User& Account : UsersList)
    {
        std::cout << Account.UserID
                  << "  " << Account.Nickname
                  << "  账号 " << (Account.UserStdNo.empty() ? Account.UserPhone : Account.UserStdNo)
                  << "  角色 " << RoleText(Account.UserRole)
                  << "  状态 " << (Account.Status == AccountStatus::Normal ? "正常" : "已封禁");
        if (Account.Reason.has_value())
        {
            std::cout << "（" << Account.Reason.value() << "）";
        }
        std::cout << "\n";
    }
    Pause();
}

void Cli::LogQueryScreen()
{
    Section("操作日志");
    LogQuery Cond;
    const std::string Keyword = Ask("关键字（可留空，匹配操作人、对象编号与描述）");
    if (!Keyword.empty())
    {
        Cond.Keyword = Keyword;
    }

    std::string Error;
    std::vector<OperationLog> Result;
    if (!Admin.QueryLogs(CurrentUser->UserID, Cond, &Result, &Error))
    {
        ShowFailure(Error.empty() ? "查询失败" : Error);
        Pause();
        return;
    }
    if (Result.empty())
    {
        ShowNotice("没有匹配的日志。");
        Pause();
        return;
    }

    std::size_t Shown = 0;
    for (const OperationLog& Record : Result)
    {
        if (Shown >= 30)
        {
            std::cout << "... 共 " << Result.size() << " 条，仅显示最近 30 条\n";
            break;
        }
        const auto Operator = Users.FindByID(Record.OperatorID);
        std::cout << TimeText(Record.OperatedAtMs)
                  << "  " << LogTypeText(Record.Type)
                  << "  " << (Operator.has_value() ? Operator->Nickname : Record.OperatorID)
                  << "  " << Record.ObjectType << ":" << Record.ObjectID
                  << "  " << Record.Description << "\n";
        ++Shown;
    }
    Pause();
}

void Cli::PlatformStatsScreen()
{
    Section("平台统计");
    const PlatformStats Stats = Admin.GetPlatformStats(0, SystemTimeMs());
    std::cout << "统计区间：全部\n"
              << "发布商品数：" << Stats.PublishedCount << "\n"
              << "预约发起次数：" << Stats.ReservationCount << "（来自「发起预约」日志）\n"
              << "完成交易数：" << Stats.CompletedTradeCount << "\n";
    Pause();
}

// ===================== 显示与输入 =====================

void Cli::ShowBanner()
{
    std::cout << "\n============================================================\n"
              << "              CSHTP 校园二手交易平台（CLI）\n"
              << "============================================================\n";
}

// 每次回到登录页都列一遍种子账号，调试时不必翻记忆。
void Cli::ShowAccounts()
{
    std::cout << " 演示账号：\n"
              << "   管理员  admin / admin123\n"
              << "   卖家    13800000001（王同学）、13800000002（李同学） / 123456\n"
              << "   买家    13800000003（陈同学） / 123456\n";
}

void Cli::Section(const std::string& Text)
{
    std::cout << "[" << Text << "]\n";
}

void Cli::Divider()
{
    std::cout << "------------------------------------------------------------\n";
}

void Cli::Pause()
{
    std::cout << "\n按回车继续...";
    std::string Ignored;
    std::getline(std::cin, Ignored);

    // 清屏，下一屏从头开始绘制
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H" << std::flush;
#endif
}

std::string Cli::Ask(const std::string& Prompt)
{
    std::cout << Prompt << "：";
    std::string Line;
    std::getline(std::cin, Line);
    return Trim(Line);
}

std::string Cli::AskRequired(const std::string& Prompt)
{
    while (true)
    {
        const std::string Value = Ask(Prompt);
        if (!Value.empty())
        {
            return Value;
        }
        ShowFailure("该项为必填");
    }
}

int Cli::AskChoice(const std::string& Prompt)
{
    while (true)
    {
        const std::string Line = Ask(Prompt);
        try
        {
            std::size_t Used = 0;
            const int Value = std::stoi(Line, &Used);
            if (Used == Line.size())
            {
                return Value;
            }
        }
        catch (...)
        {
            // 落到下面的统一提示
        }
        ShowFailure("请输入数字");
    }
}

void Cli::ShowNotice(const std::string& Message)
{
    std::cout << "[提示] " << Message << "\n";
}

void Cli::ShowFailure(const std::string& Message)
{
    std::cout << "[失败] " << Message << "\n";
}

void Cli::PrintItems(const std::vector<Item>& Goods)
{
    if (Goods.empty())
    {
        std::cout << "（没有符合条件的商品）\n";
        return;
    }

    for (std::size_t Index = 0; Index < Goods.size(); ++Index)
    {
        const Item& Target = Goods[Index];
        std::cout << std::setw(3) << (Index + 1) << ". "
                  << Target.Title
                  << "  " << PriceText(Target.PriceCents)
                  << "  " << CategoryText(Target.ItemCategory)
                  << "/" << ConditionText(Target.ItemCondition)
                  << "  [" << ItemStatusText(Target.Status) << "]"
                  << "  " << Target.TradeLocation << "\n";
    }
}

std::string Cli::PriceText(long long PriceCents)
{
    std::ostringstream Stream;
    Stream << std::fixed << std::setprecision(2)
           << (static_cast<double>(PriceCents) / 100.0) << " 元";
    return Stream.str();
}

std::string Cli::TimeText(long long Milliseconds)
{
    if (Milliseconds <= 0)
    {
        return "-";
    }

    const std::time_t Seconds = static_cast<std::time_t>(Milliseconds / 1000);
    std::tm Local{};
#ifdef _WIN32
    localtime_s(&Local, &Seconds);
#else
    localtime_r(&Seconds, &Local);
#endif

    char Buffer[32] = {};
    std::strftime(Buffer, sizeof(Buffer), "%Y-%m-%d %H:%M", &Local);
    return Buffer;
}

std::string Cli::RoleText(Role Value)
{
    switch (Value)
    {
    case Role::Buyer:  return "买家";
    case Role::Seller: return "卖家";
    case Role::Admin:  return "管理员";
    }
    return "未知";
}

std::string Cli::ItemStatusText(ItemStatus Value)
{
    switch (Value)
    {
    case ItemStatus::Draft:             return "草稿";
    case ItemStatus::PendingReview:     return "待审核";
    case ItemStatus::Rejected:          return "驳回";
    case ItemStatus::Published:         return "已发布";
    case ItemStatus::Confirmed:         return "已确认";
    case ItemStatus::OffShelf:          return "已下架";
    case ItemStatus::ViolationOffShelf: return "违规下架";
    case ItemStatus::Traded:            return "交易完成";
    }
    return "未知";
}

std::string Cli::CategoryText(Category Value)
{
    switch (Value)
    {
    case Category::Textbook:   return "教材";
    case Category::Digital:    return "数码";
    case Category::Clothing:   return "服饰";
    case Category::DailyGoods: return "生活用品";
    }
    return "未知";
}

std::string Cli::ConditionText(Condition Value)
{
    switch (Value)
    {
    case Condition::BrandNew: return "全新";
    case Condition::LikeNew:  return "九成新";
    case Condition::Flawed:   return "有瑕疵";
    }
    return "未知";
}

std::string Cli::ReservationStatusText(ReservationStatus Value)
{
    switch (Value)
    {
    case ReservationStatus::PendingConfirm: return "待确认";
    case ReservationStatus::Succeeded:      return "已成功";
    }
    return "未知";
}

std::string Cli::TradeStatusText(TradeStatus Value)
{
    switch (Value)
    {
    case TradeStatus::InProgress: return "进行中";
    case TradeStatus::Completed:  return "已完成";
    case TradeStatus::Cancelled:  return "已作废";
    }
    return "未知";
}

std::string Cli::ApplicationStatusText(ApplicationStatus Value)
{
    switch (Value)
    {
    case ApplicationStatus::PendingReview: return "待审核";
    case ApplicationStatus::Approved:      return "已通过";
    case ApplicationStatus::Rejected:      return "已驳回";
    }
    return "未知";
}

std::string Cli::LogTypeText(LogType Value)
{
    switch (Value)
    {
    case LogType::Register:           return "注册";
    case LogType::SubmitReview:       return "提交审核";
    case LogType::Audit:              return "审核";
    case LogType::OffShelf:           return "下架";
    case LogType::ViolationHandling:  return "违规处置";
    case LogType::Ban:                return "封禁";
    case LogType::Unban:              return "解封";
    case LogType::CreateReservation:  return "发起预约";
    case LogType::CancelReservation:  return "取消预约";
    case LogType::ConfirmReservation: return "确认预约";
    case LogType::DeleteReservation:  return "预约记录删除";
    case LogType::CompleteTrade:      return "完成交易";
    }
    return "未知";
}
}
