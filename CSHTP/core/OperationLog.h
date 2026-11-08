#pragma once
#include<optional>
#include<string>
#include"enums.h"

// ============================================================
// OperationLog —— 关键操作的留痕记录
//
// 一条日志最终只留三样东西：
//   Content        给人看的一句话（类型不同，句式不同）
//   Type           机器用的分类，四种之一
//   OperatedAtMs   操作时间，用于按时间范围查、按时间排序
//
// 把「什么内容」压成一个字符串，是为了让日志只承担「事后翻看」这一件事：
// 界面直接打印 Content 就行，不用再拼接字段；代价是按操作人、按对象
// 查只能对 Content 做子串匹配，结构化查询换不来学生规模的收益。
//
// 【句式写在 Create 里】四种类型各一个 Create：
//   注册         用户[用户ID]在[时间]注册了账号
//   商品成交     买家[买家ID]和卖家[卖家ID]在[时间]成功交易[商品ID]+[商品名称]
//   商品审核     管理员[管理员ID]在[时间]审核[通过/驳回]了卖家[卖家ID]的商品[商品ID]+[商品名称]
//   卖家申请审核 管理员[管理员ID]在[时间]审核[通过/驳回]了用户[用户ID]的卖家申请
//
// 【只增不改】日志生成后不能被修改或删除（REQ-021）。本类没有修改方法，
// 存储层的 Save 也只接受新日志。
// ============================================================
class OperationLog
{
    public:
        // 操作日志类的变量
        std::string LogID;              // 由存储层分配（log-1、log-2……）
        LogType Type = LogType::Register;
        long long OperatedAtMs = 0;     // 操作时间
        std::string Content;            // 完整的一句话，供界面直接展示
        // 操作日志类的变量

        // 操作日志类的构造函数

        // 注册：用户[UserID]在[时间]注册了账号
        static std::optional<OperationLog> CreateRegister(const std::string& UserID,
                                                          long long NowMs);

        // 商品成交：买家[BuyerID]和卖家[SellerID]在[时间]成功交易[ItemID]+[ItemName]
        static std::optional<OperationLog> CreateItemSold(const std::string& BuyerID,
                                                          const std::string& SellerID,
                                                          const std::string& ItemID,
                                                          const std::string& ItemName,
                                                          long long NowMs);

        // 商品审核：管理员[AdminID]在[时间]审核[结果]了卖家[SellerID]的商品[ItemID]+[ItemName]
        static std::optional<OperationLog> CreateItemAudit(const std::string& AdminID,
                                                           const std::string& SellerID,
                                                           const std::string& ItemID,
                                                           const std::string& ItemName,
                                                           bool Passed,
                                                           long long NowMs);

        // 卖家申请审核：管理员[AdminID]在[时间]审核[结果]了用户[UserID]的卖家申请
        static std::optional<OperationLog> CreateSellerAudit(const std::string& AdminID,
                                                             const std::string& UserID,
                                                             bool Passed,
                                                             long long NowMs);
        // 操作日志类的构造函数

        // 把毫秒时间戳格式化成「年-月-日 时:分:秒」，供句式拼接和测试比对
        static std::string FormatTime(long long NowMs);

    private:
        // 默认构造函数私有：日志只能由上面四个 Create 造出来。
        // 拷贝构造仍隐式生成且公开，存储层要把日志拷进容器、查询返回拷贝。
        OperationLog() = default;
};
