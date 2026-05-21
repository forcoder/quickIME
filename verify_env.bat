@echo off
echo ========================================
echo QuickInput IME 环境验证工具
echo ========================================
echo.

echo [1] 检查系统信息...
systeminfo | findstr /B /C:"OS Name" /C:"OS Version"
echo.

echo [2] 检查Visual Studio安装...
if exist "%VSINSTALLDIR%" (
    echo Visual Studio 安装路径: %VSINSTALLDIR%
) else (
    echo %VSINSTALLDIR%
)
echo.

echo [3] 检查编译器...
where cl.exe >nul 2>&1 && (
    echo ✅ cl.exe 找到
    cl --version 2>nul || echo cl --version 失败
) || (
    echo ❌ cl.exe 未找到
)
echo.

echo [4] 检查CMake...
where cmake >nul 2>&1 && (
    echo ✅ CMake 找到
    cmake --version | findstr /i "cmake" && echo ✅ CMake 版本正常
) || (
    echo ❌ CMake 未找到
)
echo.

echo [5] 检查Windows SDK...
if exist "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0" (
    echo ✅ Windows SDK 10.0.22621.0 找到
) else (
    echo ❌ Windows SDK 未找到
    dir "C:\Program Files (x86)\Windows Kits\10\Include\*.*" /O:D 2>nul | head -5
)
echo.

echo [6] 检查项目文件...
if exist "include\quickinput\ime\text_service.h" (
    echo ✅ 项目头文件存在
) else (
    echo ❌ 项目文件缺失
)
echo.

echo [7] 检查第三方依赖...
dir "build" 2>nul && (
    echo ✅ 构建目录存在
) || (
    echo ❓ 构建目录不存在（正常）
)
echo.

echo ========================================
echo 环境验证完成
echo ========================================
echo.
echo 如果所有检查都通过，可以尝试:
echo 1. mkdir build
echo 2. cd build
echo 3. cmake .. -G "Visual Studio 17 2022" -A x64
pause