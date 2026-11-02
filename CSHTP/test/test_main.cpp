#include<iostream>
#include"test.h"




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
    RunTests();
    return Test::Summary();
}