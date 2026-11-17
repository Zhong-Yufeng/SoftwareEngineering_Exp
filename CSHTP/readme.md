## CSHTP(Campus Second-Hand Trade Platform, 校园二手交易平台) 为软件工程课程项目, 采用 CLI(Command Line Interface, 命令行界面)实现, 以课程要求为实现标准.

## 开发环境

- 操作系统: Windows 10/11 x64
- 编译器: MSVC(Visual Studio 2022, "使用 C++ 的桌面开发" 工作负载), 语言标准 C++17
- 构建脚本: PowerShell 5.1+ 与 `run.ps1`
- 可选构建工具: CMake(Cross platform Make, 跨平台构建工具) 3.20 及以上
- 数据库: SQLite 3(随项目自带 `src/store/DB/sqlite3.c` 单文件 amalgamation, 无需另装)

## 运行环境要求

- 已安装 Visual Studio 2022, 且 `vcvars64.bat` 可用; `run.ps1` 会通过 `vswhere.exe` 自动定位, 失败时回退到 `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat`
- 首次运行会在当前目录创建 `data/cshtp.db` 并写入演示账号, 不需要手工建库
- 运行结束后数据保存在 `data/` 下, 重复启动不会重复播种
- 使用 CMake 时需在 "Developer PowerShell for VS 2022" 中执行, 或使用 VS 自带 cmake 的完整路径

## 一键运行测试指令

```powershell
cd CSHTP
.\run.ps1 test
```

该命令编译 `src/` 与 `test/` 下的全部源码, 运行冒烟测试, 退出码 0 表示全部通过(当前 378/378).

使用 CMake 时:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## 一键运行程序指令

```powershell
cd CSHTP
.\run.ps1 run
```

该命令编译并立即启动 CLI 程序, 首次启动自动建库并写入演示数据; 演示账号显示在登录界面上.
