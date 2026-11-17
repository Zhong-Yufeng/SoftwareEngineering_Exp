#pragma once

// 事务边界抽象：服务层只面向这个接口，不依赖具体的存储实现。
// 内存实现用 NullTransaction（全部空操作），数据库实现用 SqliteDatabase（映射到 BEGIN/COMMIT/ROLLBACK）。
class Transaction
{
    public:
        virtual ~Transaction() = default;

        // 开启事务；已在事务中时返回 false（SQLite 不支持嵌套）
        virtual bool Begin() = 0;

        // 提交当前事务
        virtual bool Commit() = 0;

        // 回滚当前事务；不在事务中时视为成功
        virtual bool Rollback() = 0;
};

// 内存存储用的事务实现：没有真正的持久化，开启与提交都不需要做任何事
class NullTransaction : public Transaction
{
    public:
        bool Begin() override
        {
            return true;
        }

        bool Commit() override
        {
            return true;
        }

        bool Rollback() override
        {
            return true;
        }
};
