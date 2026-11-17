#pragma once
#include<string>
#include"Transaction.h"
#include"sqlite3.h"

// SQLite 连接、事务与编号分配的唯一入口。
// 七个 Sqlite*Store 共用同一个连接；事务边界由服务层调用 Begin/Commit/Rollback 划定。
class SqliteDatabase : public Transaction
{
    public:
        SqliteDatabase() = default;
        ~SqliteDatabase();

        SqliteDatabase(const SqliteDatabase&) = delete;
        SqliteDatabase& operator=(const SqliteDatabase&) = delete;

        // 打开（文件不存在则创建）并建表；已打开时直接返回 true
        bool Open(const std::string& FilePath);
        void Close();
        bool IsOpen() const;

        sqlite3* Handle() const;

        bool Exec(const std::string& Sql);

        bool Begin() override;
        bool Commit() override;
        bool Rollback() override;
        bool InTransaction() const;

        // 取下一条编号：id_sequence 表内自增，再拼上前缀，例如 user-7
        bool NextID(const std::string& SequenceName, const std::string& Prefix, std::string& OutID);

        const std::string& LastError() const;
        void SetError(const std::string& Message);

    private:
        bool CreateSchema();

        sqlite3* HandleValue = nullptr;
        int TransactionDepth = 0;
        std::string ErrorMessage;
};
