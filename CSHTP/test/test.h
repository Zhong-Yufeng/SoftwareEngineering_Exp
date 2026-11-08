#pragma once
#include<iostream>
#include<string>
#ifdef _WIN32
#include<windows.h>
#endif

namespace Test
{
    inline int gTotal = 0;
    inline int gPass = 0;

    inline const char* const RESET = "\033[0m";
    inline const char* const RED   = "\033[31m";
    inline const char* const GREEN = "\033[32m";

    inline void Check(bool Cond, const std::string& What)
    {
        gTotal++;
        if (Cond)
        {
            gPass++;
            return;
        }
        std::cout << RED << "[FAIL]" << RESET << What << std::endl;
    }

    inline int Summary()
    {
        std::cout << (gPass == gTotal ? GREEN : RED)
            << gPass << "/" << gTotal
            << (gPass == gTotal ? "successful!" : "failed!")
            << RESET << std::endl;
            return gTotal == gPass ? 0 : 1; 
    }
}

void RunTests();

// 各业务的测试入口，由 RunTests 统一调用
void RunItemCreateTests();
void RunItemStateTests();
void RunItemStoreTests();
void RunReservationStoreTests();
void RunAuditRecordTests();
void RunTradeRecordTests();
void RunSellerApplicationTests();
void RunOperationLogTests();