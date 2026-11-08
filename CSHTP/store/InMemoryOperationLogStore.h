#pragma once
#include<unordered_map>
#include"OperationLogStore.h"

// ============================================================
// InMemoryOperationLogStore —— 用哈希表实现的操作日志存储
//
// 和 InMemoryAuditRecordStore 的差别只有一处：Save 不接受编号非空的日志，
// 因为日志生成后就不许改。
//
// 几个不变的约束：
//   1. 编号由这里分配（log-1、log-2……），分配后就要保证不重复。
//   2. 存的是拷贝，取出来的也是拷贝，库里的记录不会被外面改坏。
//   3. 查询是线性遍历：日志量在学生规模下最多几百条，
//      换索引只是为了快一点，却要多维护一份同步逻辑，不值得。
// ============================================================
class InMemoryOperationLogStore : public OperationLogStore
{
    public:
        bool Save(OperationLog& Log) override;
        std::optional<OperationLog> FindByID(const std::string& LogID) const override;
        std::vector<OperationLog> ListByType(LogType Type) const override;
        std::vector<OperationLog> ListByTimeRange(long long FromMs, long long ToMs) const override;
        std::vector<OperationLog> SearchByKeyword(const std::string& Keyword) const override;
        std::vector<OperationLog> All() const override;
        std::size_t Size() const override;

    private:
        // 下一个可用的编号。一旦被占用就继续往后找，避免重号
        std::string MakeNextID();

        // 键是 LogID。哈希表的遍历顺序不稳定，带列表的查询都要先排序
        std::unordered_map<std::string, OperationLog> Logs;
        std::size_t LastAssignedNumber = 0;
};
