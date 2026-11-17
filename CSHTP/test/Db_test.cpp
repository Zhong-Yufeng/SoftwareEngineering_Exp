#include"test.h"
#include<filesystem>
#include"User.h"
#include"Item.h"
#include"ServiceCommon.h"
#include"LogService.h"
#include"BuyerService.h"
#include"SqliteDatabase.h"
#include"SqliteUserStore.h"
#include"SqliteItemStore.h"
#include"SqliteReservationStore.h"
#include"SqliteTradeRecordStore.h"
#include"SqliteSellerApplicationStore.h"
#include"SqliteAuditRecordStore.h"
#include"SqliteOperationLogStore.h"

using Test::Check;

// 数据库层回归测试：临时库文件，跑完即删，不碰程序的 data/cshtp.db
void RunDbTests()
{
    namespace fs = std::filesystem;
    std::error_code Ec;

    const fs::path DbPath = fs::temp_directory_path() / "cshtp_db_test.db";
    const auto RemoveDbFiles = [&DbPath, &Ec]()
    {
        fs::remove(DbPath, Ec);
        fs::remove(fs::path(DbPath.string() + "-wal"), Ec);
        fs::remove(fs::path(DbPath.string() + "-shm"), Ec);
    };
    RemoveDbFiles();

    {
        SqliteDatabase Db;
        Check(Db.Open(DbPath.string()), "数据库打开并建表");

        SqliteUserStore Users(Db);
        SqliteItemStore Items(Db);
        SqliteReservationStore Reservations(Db);
        SqliteTradeRecordStore Trades(Db);
        SqliteSellerApplicationStore Applications(Db);
        SqliteAuditRecordStore Audits(Db);
        SqliteOperationLogStore Logs(Db);

        // ---------- 基本读写：编号由存储层分配，字段完整还原 ----------
        const auto WangOr = User::create("13800000001", "241220001", "123456", "王同学", "100001");
        Check(WangOr.has_value(), "用户对象创建");
        User Wang = WangOr.value();
        Check(Users.Save(Wang), "用户写入数据库");
        Check(Wang.UserID == "user-1", "编号由数据库分配");

        const auto Reloaded = Users.FindByID(Wang.UserID);
        Check(Reloaded.has_value(), "按编号读回用户");
        Check(Reloaded->Nickname == "王同学" && Reloaded->UserPhone == "13800000001"
              && Reloaded->UserRole == Role::Buyer,
              "用户字段完整还原");

        // ---------- 商品查询：关键词区分大小写，与内存版一致 ----------
        ItemInfo Info;
        Info.Title = "ABC 高数教材";
        Info.Description = "笔记很少，无破损";
        Info.ItemCategory = Category::Textbook;
        Info.PriceCents = 1200;
        Info.ItemCondition = Condition::LikeNew;
        Info.TradeLocation = "东区食堂门口";
        Info.Contact = "QQ 100001";

        const auto Draft = Item::CreateDraft(Info, Wang.UserID);
        Check(Draft.has_value(), "商品对象创建");
        Item Goods = Draft.value();
        Check(Items.Save(Goods), "商品写入数据库");

        ItemQuery Cond;
        Cond.Keyword = "abc";
        Check(Items.Query(Cond).empty(), "关键词区分大小写");
        Cond.Keyword = "ABC";
        Check(Items.Query(Cond).size() == 1, "关键词命中标题");
        Cond.Keyword = std::nullopt;
        Cond.TradeLocation = "食堂";
        Check(Items.Query(Cond).size() == 1, "交易地点按子串匹配");

        // ---------- 事务：回滚后不留下记录 ----------
        const std::size_t BeforeRollback = Users.Size();
        Check(Db.Begin(), "事务开启");
        User Temp = User::create("13800000002", "241220002", "123456", "李同学", "100002").value();
        Check(Users.Save(Temp), "事务内写入");
        Check(Db.Rollback(), "事务回滚");
        Check(Users.Size() == BeforeRollback, "回滚后记录数不变");

        // ---------- 服务层直接跑在数据库上 ----------
        LogService Log(Logs, []() { return 5000LL; });
        BuyerService Buyer(Users, Applications, Items, Reservations, Trades, Log, Db,
                           []() { return 5000LL; });
        std::string Error;
        const auto Chen = Buyer.Register("13800000003", "241220003", "123456", "陈同学", "100003", &Error);
        Check(Chen.has_value(), "服务层注册成功");
        Check(Users.FindByPhone("13800000003").has_value(), "注册结果已落库");
        Check(Log.Query(LogQuery{}).size() == 1, "注册日志与用户在同一事务内提交");
    }

    RemoveDbFiles();
}
