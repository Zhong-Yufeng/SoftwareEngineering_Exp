#pragma once
#include<optional>
#include<string>
#include<vector>
#include"OperationLog.h"

// ============================================================
// OperationLogStore —— 操作日志的存储与查询
//
// 和 AuditRecordStore 一样，这里只做「存、取」，不解释业务：
//   1. 只增、不改、不删。日志是留痕，能删就等于没留；同一条日志存第二次
//      会被拒绝。
//   2. 编号由这里分配（log-1、log-2……），调用方不许自己拼。
//   3. 列表按操作时间升序返回。
//
// 查询只保留三种能落地的：按类型（某类动作有几条）、按时间范围、按关键词
// 匹配内容。日志内容是一整句话，所以「按操作人查」「按商品查」都归到关键词
// 里（把 ID 当关键字传进来）；没有结构化字段可查，这是压缩成一句话的代价，
// 学生规模的数据量下可以接受。
// ============================================================
class OperationLogStore
{
    public:
        virtual ~OperationLogStore() = default;

        // 存一条新日志。编号为空则分配；编号非空一律拒绝（日志不可改写）。
        virtual bool Save(OperationLog& Log) = 0;

        virtual std::optional<OperationLog> FindByID(const std::string& LogID) const = 0;

        // 某个类型的全部日志，按时间升序
        virtual std::vector<OperationLog> ListByType(LogType Type) const = 0;

        // [FromMs, ToMs] 内的全部日志，按时间升序，两端都包含
        virtual std::vector<OperationLog> ListByTimeRange(long long FromMs, long long ToMs) const = 0;

        // 内容包含 Keyword 的日志，按时间升序。
        // 传入用户 ID、商品 ID、管理员 ID 即可实现「按操作人/对象查」。
        virtual std::vector<OperationLog> SearchByKeyword(const std::string& Keyword) const = 0;

        virtual std::vector<OperationLog> All() const = 0;

        virtual std::size_t Size() const = 0;
};
