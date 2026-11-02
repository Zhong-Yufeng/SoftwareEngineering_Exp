#include<iostream>
#include<string>
#ifdef _WIN32
#include<windows.h>
#endif
#include"User.h"
#include"InMemoryUserStore.h"

// ============================================================
// 冒烟测试
//
// 断言自己判断对错，失败才输出，全部通过只输出汇总一行。
// 颜色用 ANSI 转义序列：\033[31m 红、\033[32m 绿、\033[0m 复位。
// Windows 控制台默认不解析这些序列，所以 main 开头要打开 VT 处理。
//
// 覆盖范围：User::create 的校验与装配 + InMemoryUserStore 的
//           Save / FindByID / FindByPhone / FindByStdNo + 多态。
// 其余 5 个实体类尚未实现，暂无测试。
// ============================================================

namespace
{
    int gTotal = 0;      // 断言总数
    int gPass = 0;       // 通过的断言数

    const char* const RED = "\033[31m";
    const char* const GREEN = "\033[32m";
    const char* const RESET = "\033[0m";

    // 只有失败才打印，通过的断言保持沉默
    void Check(bool Cond, const std::string& What)
    {
        ++gTotal;
        if (Cond)
        {
            ++gPass;
            return;
        }
        std::cout << RED << "[FAIL] " << RESET << What << std::endl;
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE Out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD Mode = 0;
    if (GetConsoleMode(Out, &Mode))
    {
        SetConsoleMode(Out, Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

    // ================= 1. create 的校验规则 =================
    {
        auto A = User::create("t001", "15659883561", "241220170", "ws123456", "mambo", "1063617981");
        Check(A.has_value(), "create: 手机号和学号都有，应成功");
    }
    {
        auto A = User::create("t002", "13900000001", "", "12345", "小明", "10001");
        Check(!A.has_value(), "create: 密码 5 位，应被拒绝");
    }
    {
        auto A = User::create("t003", "13900000002", "", "123456", "小明", "10002");
        Check(A.has_value(), "create: 密码 6 位（边界值），应成功");
    }
    {
        auto A = User::create("t004", "", "", "ws123456", "小明", "10004");
        Check(!A.has_value(), "create: 手机号和学号都为空，应被拒绝");
    }
    {
        auto A = User::create("t005", "13900000005", "", "ws123456", "小明", "10005");
        Check(A.has_value(), "create: 只给手机号，应成功");
    }
    {
        auto A = User::create("t006", "", "241220176", "ws123456", "小明", "10006");
        Check(A.has_value(), "create: 只给学号，应成功");
    }
    {
        auto A = User::create("t007", "13900000007", "", "ws123456", "", "10007");
        Check(!A.has_value(), "create: 昵称为空，应被拒绝");
    }
    {
        auto A = User::create("t008", "13900000008", "", "ws123456", "小明", "");
        Check(!A.has_value(), "create: 联系方式为空，应被拒绝");
    }

    // ================= 2. create 的字段装配 =================
    {
        auto A = User::create("t100", "15659880000", "241220100", "ws123456", "mambo", "1063617981");
        Check(A.has_value(), "create: 装配组前置，合法输入应成功");
        if (A)
        {
            Check(A->UserID   == "t100",                "create: UserID 应等于入参");
            Check(A->Password == "ws123456",            "create: Password 应等于入参");
            Check(A->Nickname == "mambo",               "create: Nickname 应等于入参");
            Check(A->QQNo     == "1063617981",          "create: QQNo 应等于入参");
            Check(A->Status   == AccountStatus::Normal, "create: 新用户状态应为 Normal");
        }
    }
    {
        auto A = User::create("t101", "", "241220101", "ws123456", "小红", "10010");
        Check(A.has_value() && !A->UserPhone.has_value(),
              "create: 只给学号时 UserPhone 应为空，而不是空串");
    }
    {
        auto A = User::create("t102", "13900000012", "", "ws123456", "小刚", "10011");
        Check(A.has_value() && !A->UserStdNo.has_value(),
              "create: 只给手机号时 UserStdNo 应为空，而不是空串");
    }

    // ================= 3. Save 与 FindByID =================
    {
        InMemoryUserStore Store;
        auto A = User::create("t200", "13900000020", "", "ws123456", "阿全", "10020");
        Check(A.has_value(), "Save: 前置，create 应成功");
        if (A)
        {
            Store.Save(*A);

            auto Got = Store.FindByID("t200");
            Check(Got.has_value(),                       "FindByID: 已保存的 ID 应能查到");
            Check(Got && Got->Nickname == "阿全",         "FindByID: 查到的昵称应与存入的一致");
            Check(!Store.FindByID("nope").has_value(),   "FindByID: 不存在的 ID 应返回空");
        }
    }
    {
        InMemoryUserStore Store;
        auto A = User::create("t210", "13900000021", "", "ws123456", "旧昵称", "10021");
        Check(A.has_value(), "Save: 覆盖组前置，create 应成功");
        if (A)
        {
            Store.Save(*A);
            A->Nickname = "新昵称";
            Store.Save(*A);

            auto Got = Store.FindByID("t210");
            Check(Got && Got->Nickname == "新昵称", "Save: 同一 ID 再次保存应覆盖为新值");
        }
    }
    {
        InMemoryUserStore Store;
        auto A = User::create("t220", "13900000022", "", "ws123456", "甲", "10022");
        auto B = User::create("t221", "13900000023", "", "ws123456", "乙", "10023");
        Check(A.has_value() && B.has_value(), "Save: 多用户组前置，create 应成功");
        if (A && B)
        {
            Store.Save(*A);
            Store.Save(*B);

            auto GotA = Store.FindByID("t220");
            auto GotB = Store.FindByID("t221");
            Check(GotA && GotA->Nickname == "甲", "Save: 两个用户互不覆盖（甲）");
            Check(GotB && GotB->Nickname == "乙", "Save: 两个用户互不覆盖（乙）");
        }
    }

    // ================= 4. FindByPhone =================
    {
        InMemoryUserStore Store;
        auto A = User::create("t300", "15659880001", "", "ws123456", "手机用户", "10030");
        auto B = User::create("t301", "", "241220301", "ws123456", "学号用户", "10031");
        Check(A.has_value() && B.has_value(), "FindByPhone: 前置，两个用户应创建成功");
        if (A && B)
        {
            Store.Save(*A);
            Store.Save(*B);

            auto ByPhone = Store.FindByPhone("15659880001");
            Check(ByPhone.has_value() && ByPhone->UserID == "t300",
                  "FindByPhone: 应命中正确用户");
            Check(!Store.FindByPhone("13999999999").has_value(),
                  "FindByPhone: 不存在的手机号应返回空");
            Check(!Store.FindByPhone("").has_value(),
                  "FindByPhone: 空字符串不应命中只用学号注册的用户（空值 vs 空串）");
        }
    }

    // ================= 5. FindByStdNo =================
    {
        InMemoryUserStore Store;
        auto A = User::create("t400", "", "241220400", "ws123456", "学号用户", "10040");
        Check(A.has_value(), "FindByStdNo: 前置，create 应成功");
        if (A)
        {
            Store.Save(*A);

            auto ByStdNo = Store.FindByStdNo("241220400");
            Check(ByStdNo.has_value() && ByStdNo->UserID == "t400",
                  "FindByStdNo: 应命中正确用户");
            Check(!Store.FindByStdNo("241220999").has_value(),
                  "FindByStdNo: 不存在的学号应返回空");
            Check(!Store.FindByStdNo("").has_value(),
                  "FindByStdNo: 空字符串不应命中只用手机号注册的用户（空值 vs 空串）");
        }
    }

    // ================= 6. 基类指针多态 =================
    {
        InMemoryUserStore Store;
        auto A = User::create("t500", "13900000050", "", "ws123456", "多态", "10050");
        Check(A.has_value(), "多态: 前置，create 应成功");
        if (A)
        {
            Store.Save(*A);

            UserStore* Base = &Store;
            auto Got = Base->FindByID("t500");
            Check(Got.has_value() && Got->Nickname == "多态",
                  "多态: 基类指针应调到派生类实现");
            Check(!Base->FindByID("t999").has_value(),
                  "多态: 基类指针查不到时应返回空");
        }
    }

    // ================= 汇总 =================
    std::cout << (gPass == gTotal ? GREEN : RED)
              << gPass << "/" << gTotal
              << (gPass == gTotal ? ", successful!" : ", failed!")
              << RESET << std::endl;

    return gPass == gTotal ? 0 : 1;
}
