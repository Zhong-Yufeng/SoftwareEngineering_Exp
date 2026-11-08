#pragma once
#include<optional>
#include<string>
#include"enums.h"

// ============================================================
// SellerApplication —— 买家申请成为卖家的认证单
//
// 字段全部公开：它是一条「记录」，不是要维护不变式的领域对象，再包一层
// 访问函数只会让存储层和服务层多绕一圈，不带来任何好处。
//
// 类里只留三样东西：
//   Create    校验申请人、姓名，造一张待审核的单，编号留空给存储层
//   Approve   审核员改状态：通过
//   Reject    审核员改状态：驳回
//
// 【没有草稿、也没有提交】申请单创建出来就是待审核：认证审核只有「交上来
// 等审」这一种形态，不存在填一半先存着的中间态。
//
// 【一人一条】每个用户只能有一条申请记录，不论结论是待审、通过还是驳回。
// 这条约束要看别的记录才能判断，所以由存储层保证（Save 会拒绝第二条），
// 本类只负责「这一条本身合法」。
//
// 【状态模型】3 个状态、2 条边，见 enums.h 的 ApplicationStatus 注释：
//   PendingReview ──通过──► Approved
//                 ──驳回──► Rejected
// 通过/驳回都是终态，不能再改判，也没有「重新提交」。
// ============================================================
class SellerApplication
{
    public:
        // 卖家认证申请类的变量
        std::string ApplicationID;                  // 由存储层分配
        std::string ApplicantID;                    // 申请人（当前是买家）
        std::string RealName;                       // 申请时填的真实姓名
        ApplicationStatus AuditStatus = ApplicationStatus::PendingReview;
        long long SubmittedAtMs = 0;                // 提交时间，创建时写入
        std::optional<long long> AuditedAtMs;       // 审核时间，审核前为空
        std::optional<std::string> RejectReason;    // 驳回理由，仅 Rejected 时有值
        std::optional<std::string> ReviewerID;      // 审核人，审核前为空
        // 卖家认证申请类的变量

        // 卖家认证申请类的构造函数
        // 申请人与真实姓名是审核员判断的全部依据，任一为空返回 std::nullopt；
        // 提交时间必须为正，否则返回 std::nullopt。
        // 以下字段在函数内固定：ApplicationID 留空（存储层分配）、
        // AuditStatus=PendingReview、三个审核字段为空。
        // 时间由外部传入而不是内部读时钟，是为了让测试能断言具体数值。
        static std::optional<SellerApplication> Create(const std::string& ApplicantID,
                                                       const std::string& RealName,
                                                       long long NowMs);
        // 卖家认证申请类的构造函数

        // 卖家认证申请类的方法

        // 审核通过：PendingReview → Approved
        // 前置：当前是待审核、审核人非空、时间为正；否则返回 false 且对象不动。
        // 副作用：写审核时间、审核人，清空驳回理由（维持「Rejected ⇔ 有理由」）。
        bool Approve(const std::string& Reviewer, long long NowMs);

        // 审核驳回：PendingReview → Rejected
        // 前置：当前是待审核、理由非空、审核人非空、时间为正。
        // 副作用：写审核时间、审核人、驳回理由。
        // 【为什么理由必须非空】需求规定驳回必须填理由；空串和「没填」在
        //   业务上没有区别，所以在这里挡掉，不留给界面去校验。
        bool Reject(const std::string& Reason, const std::string& Reviewer, long long NowMs);

        // 卖家认证申请类的方法

    private:
        // 默认构造函数私有：申请单只能由 Create 造出来。
        // 拷贝构造仍由编译器隐式生成且公开，这是有意保留的：
        // 存储层要把申请拷进容器，查询也要返回拷贝。
        SellerApplication() = default;
};
