#include"OperationLog.h"
#include"InMemoryOperationLogStore.h"
#include"test.h"

// ================= OperationLog 的测试 =================
//
// 分三块：
//   1. 四个 Create 的入参校验与句式
//   2. 存储层的编号、拒改与三个查询维度
//   3. 存取之间互不影响
void RunOperationLogTests()
{
    const long long T0 = 1700000000000LL;   // 固定时间，避免用系统时钟导致用例不稳定
    const long long T1 = T0 + 60 * 1000;    // 一分钟后
    const long long T2 = T0 + 120 * 1000;   // 两分钟后

    // ================= 1. Create 的校验与句式 =================
    {
        Test::Check(!OperationLog::CreateRegister("", T0).has_value(),
                    "注册日志：用户 ID 为空时返回空");

        Test::Check(!OperationLog::CreateRegister("u1", 0).has_value(),
                    "注册日志：时间为 0 时返回空");

        auto Register = OperationLog::CreateRegister("u1", T0);
        Test::Check(Register.has_value(), "注册日志：正常入参可以创建");
        Test::Check(Register->Type == LogType::Register, "注册日志：类型是 Register");
        Test::Check(Register->OperatedAtMs == T0, "注册日志：记录了操作时间");
        Test::Check(Register->LogID.empty(), "注册日志：编号留给存储层");
        Test::Check(Register->Content == "用户u1在" + OperationLog::FormatTime(T0) + "注册了账号",
                    "注册日志：句式与约定一致");

        Test::Check(!OperationLog::CreateItemSold("b1", "s1", "i1", "", T0).has_value(),
                    "成交日志：商品名称为空时返回空");

        auto Sold = OperationLog::CreateItemSold("b1", "s1", "i1", "旧手机", T0);
        Test::Check(Sold.has_value(), "成交日志：正常入参可以创建");
        Test::Check(Sold->Type == LogType::ItemSold, "成交日志：类型是 ItemSold");
        Test::Check(Sold->Content
                        == "买家b1和卖家s1在" + OperationLog::FormatTime(T0) + "成功交易i1+旧手机",
                    "成交日志：句式与约定一致");

        Test::Check(!OperationLog::CreateItemAudit("a1", "s1", "i1", "旧手机", true, 0).has_value(),
                    "商品审核日志：时间为 0 时返回空");

        auto AuditPassed = OperationLog::CreateItemAudit("a1", "s1", "i1", "旧手机", true, T0);
        Test::Check(AuditPassed.has_value(), "商品审核日志：通过时可以创建");
        Test::Check(AuditPassed->Type == LogType::ItemAudited, "商品审核日志：类型是 ItemAudited");
        Test::Check(AuditPassed->Content
                        == "管理员a1在" + OperationLog::FormatTime(T0)
                               + "审核通过了卖家s1的商品i1+旧手机",
                    "商品审核日志：通过的句式与约定一致");

        auto AuditRejected = OperationLog::CreateItemAudit("a1", "s1", "i1", "旧手机", false, T0);
        Test::Check(AuditRejected.has_value(), "商品审核日志：驳回时可以创建");
        Test::Check(AuditRejected->Content
                        == "管理员a1在" + OperationLog::FormatTime(T0)
                               + "审核驳回了卖家s1的商品i1+旧手机",
                    "商品审核日志：驳回只改结果词，句式不变");

        Test::Check(!OperationLog::CreateSellerAudit("", "u1", true, T0).has_value(),
                    "卖家审核日志：管理员 ID 为空时返回空");

        auto SellerPassed = OperationLog::CreateSellerAudit("a1", "u1", true, T0);
        Test::Check(SellerPassed.has_value(), "卖家审核日志：通过时可以创建");
        Test::Check(SellerPassed->Content
                        == "管理员a1在" + OperationLog::FormatTime(T0) + "审核通过了用户u1的卖家申请",
                    "卖家审核日志：通过的句式与约定一致");

        auto SellerRejected = OperationLog::CreateSellerAudit("a1", "u1", false, T0);
        Test::Check(SellerRejected->Content
                        == "管理员a1在" + OperationLog::FormatTime(T0) + "审核驳回了用户u1的卖家申请",
                    "卖家审核日志：驳回的句式与约定一致");
    }

    // ================= 2. 存储层：编号与拒改 =================
    {
        InMemoryOperationLogStore Store;

        auto Log = OperationLog::CreateRegister("u1", T0);
        Test::Check(Store.Save(*Log), "存储：新日志可以存入");
        Test::Check(Log->LogID == "log-1", "存储：第一条日志编号是 log-1");
        Test::Check(Store.Size() == 1, "存储：存入后数量是 1");

        // 已经带编号的日志再存一次，必须被拒绝
        Test::Check(!Store.Save(*Log), "存储：带编号的日志不能再存");
        Test::Check(Store.Size() == 1, "存储：被拒绝后数量不变");

        // 同一句内容换一个新对象，属于另一条日志，编号接着往下发
        auto Another = OperationLog::CreateRegister("u1", T1);
        Test::Check(Store.Save(*Another), "存储：同内容的第二条日志可以存入");
        Test::Check(Another->LogID == "log-2", "存储：编号连续分配");

        auto Found = Store.FindByID("log-2");
        Test::Check(Found.has_value(), "存储：按编号可以查到");
        Test::Check(Found->Content == Another->Content, "存储：查到的是同一条内容");
        Test::Check(!Store.FindByID("log-99").has_value(), "存储：查没有的编号返回空");
        Test::Check(!Store.FindByID("").has_value(), "存储：查空编号返回空");
    }

    // ================= 3. 查询：按类型 / 时间 / 关键词 =================
    {
        InMemoryOperationLogStore Store;

        auto Register = OperationLog::CreateRegister("u1", T0);
        auto Sold = OperationLog::CreateItemSold("b1", "s1", "i1", "旧手机", T1);
        auto Audit = OperationLog::CreateItemAudit("a1", "s1", "i1", "旧手机", true, T2);
        Test::Check(Store.Save(*Register), "查询准备：注册日志存入");
        Test::Check(Store.Save(*Sold), "查询准备：成交日志存入");
        Test::Check(Store.Save(*Audit), "查询准备：审核日志存入");

        // 按类型
        auto Registers = Store.ListByType(LogType::Register);
        Test::Check(Registers.size() == 1, "按类型：注册日志只有一条");
        Test::Check(Registers[0].LogID == "log-1", "按类型：内容对得上");

        Test::Check(Store.ListByType(LogType::SellerAudited).empty(),
                    "按类型：没有的类别返回空");

        // 按时间范围，两端都包含
        const auto AtT0 = Store.ListByTimeRange(T0, T0);
        Test::Check(AtT0.size() == 1, "按时间：只传一个时刻也能查到当刻的日志");
        Test::Check(AtT0[0].OperatedAtMs == T0, "按时间：查到的是那一刻的日志");

        const auto Middle = Store.ListByTimeRange(T0 + 1, T2 - 1);
        Test::Check(Middle.size() == 1 && Middle[0].LogID == "log-2",
                    "按时间：区间内只有中间的成交日志");

        const auto AllInRange = Store.ListByTimeRange(T0, T2);
        Test::Check(AllInRange.size() == 3, "按时间：全区间包含三条");
        Test::Check(AllInRange[0].OperatedAtMs == T0 && AllInRange[2].OperatedAtMs == T2,
                    "按时间：结果按时间升序");

        Test::Check(Store.ListByTimeRange(T2, T0).empty(), "按时间：起点晚于终点返回空");

        // 按关键词
        const auto ByUser = Store.SearchByKeyword("u1");
        Test::Check(ByUser.size() == 1, "按关键词：u1 只出现在注册日志里");
        Test::Check(ByUser[0].Type == LogType::Register, "按关键词：命中的是注册日志");

        const auto ByItem = Store.SearchByKeyword("i1");
        Test::Check(ByItem.size() == 2, "按关键词：i1 出现在成交与审核两条里");
        Test::Check(ByItem[0].Type == LogType::ItemSold && ByItem[1].Type == LogType::ItemAudited,
                    "按关键词：结果按时间升序");

        Test::Check(Store.SearchByKeyword("不存在").empty(), "按关键词：无命中返回空");
        Test::Check(Store.SearchByKeyword("").empty(), "按关键词：空串返回空而不是全部");

        // 全部
        const auto Everything = Store.All();
        Test::Check(Everything.size() == 3, "全部：三条都在");
        Test::Check(Everything[0].OperatedAtMs <= Everything[1].OperatedAtMs
                        && Everything[1].OperatedAtMs <= Everything[2].OperatedAtMs,
                    "全部：按时间升序");
    }

    // ================= 4. 存取互不影响 =================
    {
        InMemoryOperationLogStore Store;

        auto Log = OperationLog::CreateRegister("u1", T0);
        Test::Check(Store.Save(*Log), "隔离：日志已存入");

        // 存进去之后再改调用方的对象，库里的记录不能被带走
        Log->Content = "被改过的内容";
        auto Found = Store.FindByID("log-1");
        Test::Check(Found.has_value() && Found->Content != "被改过的内容",
                    "隔离：存的是拷贝，外部改动不影响库");

        Found->Content = "又一次外部改动";
        auto Again = Store.FindByID("log-1");
        Test::Check(Again.has_value() && Again->Content != "又一次外部改动",
                    "隔离：查出来的也是拷贝，改动不会写回");
    }
}
