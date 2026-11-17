#pragma once
#include<string>
#include"sqlite3.h"

class SqliteDatabase;

// 预编译语句的 RAII 包装：负责绑参、取列，出错时把消息写回 SqliteDatabase。
class SqliteStatement
{
    public:
        SqliteStatement(SqliteDatabase& Db, const std::string& Sql);
        ~SqliteStatement();

        SqliteStatement(const SqliteStatement&) = delete;
        SqliteStatement& operator=(const SqliteStatement&) = delete;

        bool IsReady() const;
        bool HasError() const;

        void BindText(int Index, const std::string& Value);
        void BindInt64(int Index, long long Value);
        void BindNull(int Index);

        // 返回 true 表示当前有一行可读；false 表示结束或出错（用 HasError 区分）
        bool Step();

        long long Int64(int Column) const;
        std::string Text(int Column) const;
        bool IsNull(int Column) const;

    private:
        void Fail(const std::string& Message);

        SqliteDatabase* Database = nullptr;
        sqlite3_stmt* Statement = nullptr;
        bool Error = false;
};
