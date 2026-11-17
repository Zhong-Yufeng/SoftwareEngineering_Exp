# CSHTP 构建脚本（CMake）
#
# 用法（在 CSHTP 目录下）：
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#   cmake --build build --config Release
#   ctest --test-dir build -C Release --output-on-failure
#
# 产物：build/bin/cshtp.exe 与 build/bin/cshtp_tests.exe

cmake_minimum_required(VERSION 3.20)

project(CSHTP LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# 语言标准与编译选项（与 run.ps1 里 cl 的参数保持一致）
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(MSVC)
    add_compile_options(/utf-8 /W3)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# 静态库：core + store（内存版与 SQLite 版）+ service，外加 sqlite3 amalgamation
# ---------------------------------------------------------------------------
file(GLOB_RECURSE CSHTP_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cpp)

list(APPEND CSHTP_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store/DB/sqlite3.c)

add_library(cshtp_core STATIC ${CSHTP_CORE_SOURCES})

target_include_directories(cshtp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store/InMemory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store/DB
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service)

# ---------------------------------------------------------------------------
# 可执行程序：CLI
# ---------------------------------------------------------------------------
add_executable(cshtp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli/Cli.cpp)

target_include_directories(cshtp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli)

target_link_libraries(cshtp PRIVATE cshtp_core)

# ---------------------------------------------------------------------------
# 冒烟测试
# ---------------------------------------------------------------------------
enable_testing()

file(GLOB CSHTP_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp)

add_executable(cshtp_tests ${CSHTP_TEST_SOURCES})

target_include_directories(cshtp_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test)

target_link_libraries(cshtp_tests PRIVATE cshtp_core)

add_test(NAME smoke COMMAND cshtp_tests)
