#include"SqliteDatabase.h"
#include"SqliteStatement.h"
#include<string>

namespace
{
    // 建表语句。枚举一律存 INTEGER（顺序即 enums.h 的声明顺序），
    // 空串与 NULL 的区别保持与内存版一致：可选字段用 NULL，必填字符串用空串。
    const char* const SchemaSql = R"SQL(
CREATE TABLE IF NOT EXISTS id_sequence(
    SequenceName TEXT PRIMARY KEY,
    NextValue INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS users(
    UserID TEXT PRIMARY KEY,
    UserPhone TEXT NOT NULL DEFAULT '',
    UserStdNo TEXT NOT NULL DEFAULT '',
    Password TEXT NOT NULL,
    Nickname TEXT NOT NULL,
    QQNo TEXT NOT NULL DEFAULT '',
    Status INTEGER NOT NULL,
    Reason TEXT NULL,
    UserRole INTEGER NOT NULL
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_users_phone ON users(UserPhone) WHERE UserPhone <> '';
CREATE UNIQUE INDEX IF NOT EXISTS ux_users_stdno ON users(UserStdNo) WHERE UserStdNo <> '';

CREATE TABLE IF NOT EXISTS items(
    ItemID TEXT PRIMARY KEY,
    Title TEXT NOT NULL DEFAULT '',
    Description TEXT NOT NULL DEFAULT '',
    Category INTEGER NOT NULL,
    PriceCents INTEGER NOT NULL DEFAULT 0,
    Condition INTEGER NOT NULL,
    TradeLocation TEXT NOT NULL DEFAULT '',
    Contact TEXT NOT NULL DEFAULT '',
    Status INTEGER NOT NULL,
    PublishedAtMs INTEGER NOT NULL DEFAULT 0,
    RejectReason TEXT NULL,
    SellerID TEXT NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS ix_items_seller ON items(SellerID);
CREATE INDEX IF NOT EXISTS ix_items_status ON items(Status);

CREATE TABLE IF NOT EXISTS reservations(
    ReservationID TEXT PRIMARY KEY,
    BuyerID TEXT NOT NULL,
    ItemID TEXT NOT NULL,
    CreatedAtMs INTEGER NOT NULL,
    Status INTEGER NOT NULL,
    ConfirmedAtMs INTEGER NULL
);
CREATE INDEX IF NOT EXISTS ix_reservations_buyer ON reservations(BuyerID);
CREATE INDEX IF NOT EXISTS ix_reservations_item ON reservations(ItemID);
CREATE UNIQUE INDEX IF NOT EXISTS ux_reservations_success
    ON reservations(ItemID) WHERE Status = 1;

CREATE TABLE IF NOT EXISTS trade_records(
    TradeID TEXT PRIMARY KEY,
    ReservationID TEXT NOT NULL,
    ItemID TEXT NOT NULL,
    BuyerID TEXT NOT NULL,
    SellerID TEXT NOT NULL,
    Status INTEGER NOT NULL,
    CreatedAtMs INTEGER NOT NULL,
    CompletedAtMs INTEGER NOT NULL DEFAULT 0,
    CancelledAtMs INTEGER NOT NULL DEFAULT 0,
    CancelReason TEXT NOT NULL DEFAULT '',
    ItemTitle TEXT NOT NULL DEFAULT '',
    PriceCents INTEGER NOT NULL DEFAULT 0,
    TradeLocation TEXT NOT NULL DEFAULT '',
    SellerContact TEXT NOT NULL DEFAULT '',
    BuyerContact TEXT NOT NULL DEFAULT ''
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_trade_reservation ON trade_records(ReservationID);
CREATE UNIQUE INDEX IF NOT EXISTS ux_trade_active
    ON trade_records(ItemID) WHERE Status = 0;
CREATE INDEX IF NOT EXISTS ix_trade_buyer ON trade_records(BuyerID);
CREATE INDEX IF NOT EXISTS ix_trade_seller ON trade_records(SellerID);
CREATE INDEX IF NOT EXISTS ix_trade_item ON trade_records(ItemID);

CREATE TABLE IF NOT EXISTS seller_applications(
    ApplicationID TEXT PRIMARY KEY,
    ApplicantID TEXT NOT NULL,
    RealName TEXT NOT NULL DEFAULT '',
    AuditStatus INTEGER NOT NULL,
    SubmittedAtMs INTEGER NOT NULL,
    AuditedAtMs INTEGER NULL,
    RejectReason TEXT NULL,
    ReviewerID TEXT NULL
);
CREATE INDEX IF NOT EXISTS ix_app_applicant ON seller_applications(ApplicantID);
CREATE UNIQUE INDEX IF NOT EXISTS ux_app_pending
    ON seller_applications(ApplicantID) WHERE AuditStatus = 0;

CREATE TABLE IF NOT EXISTS audit_records(
    RecordID TEXT PRIMARY KEY,
    ItemID TEXT NOT NULL,
    ReviewerID TEXT NOT NULL,
    Result INTEGER NOT NULL,
    AuditedAtMs INTEGER NOT NULL,
    Reason TEXT NULL
);
CREATE INDEX IF NOT EXISTS ix_audit_item ON audit_records(ItemID);
CREATE INDEX IF NOT EXISTS ix_audit_reviewer ON audit_records(ReviewerID);

CREATE TABLE IF NOT EXISTS operation_logs(
    LogID TEXT PRIMARY KEY,
    Type INTEGER NOT NULL,
    OperatedAtMs INTEGER NOT NULL,
    OperatorID TEXT NOT NULL DEFAULT '',
    ObjectType TEXT NOT NULL DEFAULT '',
    ObjectID TEXT NOT NULL DEFAULT '',
    Description TEXT NOT NULL DEFAULT '',
    Deleted TEXT NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS ix_logs_type ON operation_logs(Type);
CREATE INDEX IF NOT EXISTS ix_logs_time ON operation_logs(OperatedAtMs);
CREATE INDEX IF NOT EXISTS ix_logs_operator ON operation_logs(OperatorID);

INSERT OR IGNORE INTO id_sequence(SequenceName, NextValue) VALUES
    ('user', 1), ('item', 1), ('res', 1), ('trade', 1),
    ('app', 1), ('aud', 1), ('log', 1);
)SQL";
}

SqliteDatabase::~SqliteDatabase()
{
    Close();
}

bool SqliteDatabase::Open(const std::string& FilePath)
{
    if (HandleValue != nullptr)
    {
        return true;
    }

    sqlite3* Opened = nullptr;
    const int Result = sqlite3_open_v2(FilePath.c_str(), &Opened,
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (Result != SQLITE_OK)
    {
        ErrorMessage = Opened != nullptr ? sqlite3_errmsg(Opened) : "无法打开数据库文件";
        if (Opened != nullptr)
        {
            sqlite3_close(Opened);
        }
        return false;
    }

    HandleValue = Opened;

    // WAL：读不被写阻塞；busy_timeout：写锁被占时等 5 秒再报错
    if (!Exec("PRAGMA journal_mode=WAL;") || !Exec("PRAGMA busy_timeout=5000;")
        || !Exec("PRAGMA foreign_keys=OFF;"))
    {
        Close();
        return false;
    }

    if (!CreateSchema())
    {
        Close();
        return false;
    }

    ErrorMessage.clear();
    return true;
}

void SqliteDatabase::Close()
{
    if (HandleValue == nullptr)
    {
        return;
    }

    sqlite3_close(HandleValue);
    HandleValue = nullptr;
    TransactionDepth = 0;
}

bool SqliteDatabase::IsOpen() const
{
    return HandleValue != nullptr;
}

sqlite3* SqliteDatabase::Handle() const
{
    return HandleValue;
}

bool SqliteDatabase::Exec(const std::string& Sql)
{
    if (HandleValue == nullptr)
    {
        SetError("数据库尚未打开");
        return false;
    }

    char* Message = nullptr;
    const int Result = sqlite3_exec(HandleValue, Sql.c_str(), nullptr, nullptr, &Message);
    if (Result != SQLITE_OK)
    {
        ErrorMessage = Message != nullptr ? Message : sqlite3_errmsg(HandleValue);
        sqlite3_free(Message);
        return false;
    }

    sqlite3_free(Message);
    return true;
}

bool SqliteDatabase::Begin()
{
    if (!Exec("BEGIN IMMEDIATE;"))
    {
        return false;
    }
    ++TransactionDepth;
    return true;
}

bool SqliteDatabase::Commit()
{
    if (!Exec("COMMIT;"))
    {
        return false;
    }
    if (TransactionDepth > 0)
    {
        --TransactionDepth;
    }
    return true;
}

bool SqliteDatabase::Rollback()
{
    if (HandleValue == nullptr)
    {
        return false;
    }

    // 事务外回滚会报 "no transaction is active"，这里只清理状态
    if (TransactionDepth == 0)
    {
        return true;
    }

    const bool Succeeded = Exec("ROLLBACK;");
    TransactionDepth = 0;
    return Succeeded;
}

bool SqliteDatabase::InTransaction() const
{
    return TransactionDepth > 0;
}

bool SqliteDatabase::NextID(const std::string& SequenceName, const std::string& Prefix, std::string& OutID)
{
    // 一条 UPDATE ... RETURNING 完成「读并自增」，不需要先 SELECT 再 UPDATE
    SqliteStatement Take(*this,
                         "UPDATE id_sequence SET NextValue = NextValue + 1 "
                         "WHERE SequenceName = ? RETURNING NextValue - 1;");
    if (!Take.IsReady())
    {
        return false;
    }

    Take.BindText(1, SequenceName);
    if (!Take.Step())
    {
        SetError("编号序列不存在：" + SequenceName);
        return false;
    }

    OutID = Prefix + std::to_string(Take.Int64(0));
    return true;
}

const std::string& SqliteDatabase::LastError() const
{
    return ErrorMessage;
}

void SqliteDatabase::SetError(const std::string& Message)
{
    ErrorMessage = Message;
}

bool SqliteDatabase::CreateSchema()
{
    return Exec(SchemaSql);
}
