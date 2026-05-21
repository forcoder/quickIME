# QuickInput IME - 环境配置指南

## 系统要求

### 操作系统
- Windows 10 (版本 1809+) 或 Windows 11
- 64位系统推荐

### 开发工具
- Visual Studio 2022 (MSVC v143) 或更高版本
- CMake 3.20 或更高版本
- Windows SDK 10.0.22621 或更高版本

## 安装步骤

### 1. Visual Studio 2022 安装

#### 推荐组件
在Visual Studio Installer中确保安装了以下工作负载：
- **使用C++的桌面开发**
  - MSVC v143 - VS 2022 C++ x64/x86 生成工具 (v14.3x)
  - Windows 10/11 SDK (10.0.22621)
  - C++ CMake 工具 for Windows
  - C++ ATL for latest v143 build tools
  - C++ MFC for latest v143 build tools

#### 可选但推荐的组件
- Git for Windows
- Python 3.x (用于某些脚本)

### 2. CMake 安装

#### 方法一：从官网下载安装
1. 访问 https://cmake.org/download/
2. 下载最新版本的CMake for Windows
3. 运行安装程序
4. 将CMake添加到系统PATH：
   ```powershell
   # 通常安装路径为：
   C:\Program Files\CMake\bin
   ```

#### 验证安装
```cmd
cmake --version
# 预期输出: cmake version 3.20+
```

### 3. 环境变量配置

#### 添加必要的环境变量
```cmd
# 添加到系统环境变量
setx PATH "%PATH%;C:\Program Files\CMake\bin"
setx INCLUDE "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared"
setx LIB "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"
```

#### 重新打开命令提示符验证
```cmd
where cl.exe
where cmake
```

## 项目特定配置

### 1. Visual Studio 命令行环境

#### 使用VS开发人员命令提示符
```cmd
# 从开始菜单运行："x64 Native Tools Command Prompt for VS 2022"
# 或者手动激活
"C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

# 验证编译器
cl --version
```

### 2. CMake配置参数

#### 推荐的CMake配置
```bash
mkdir build
cd build

# 基本配置（启用所有功能）
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DQI_ENABLE_AI=ON \
         -DQI_BUILD_TESTS=ON \
         -DQI_BUILD_INSTALLER=ON

# 调试配置
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DCMAKE_BUILD_TYPE=Debug

# 最小化配置（仅IME核心）
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DQI_ENABLE_AI=OFF \
         -DQI_BUILD_TESTS=OFF
```

### 3. 第三方依赖

#### SQLite3 安装
```cmd
# 使用vcpkg安装SQLite3
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install sqlite3:x64-windows

# 添加到环境变量
setx VCPKG_ROOT "C:\path\to\vcpkg"
```

#### llama.cpp (可选，用于AI推理)
```bash
# 克隆llama.cpp仓库
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
make LLAMA_CUBLAS=0  # CPU版本
```

## 验证环境配置

### 1. 基础验证脚本
```batch
@echo off
echo === QuickInput IME 环境验证 ===

echo [1] 检查编译器...
where cl.exe >nul 2>&1 && (
    echo ✅ cl.exe 找到
) || (
    echo ❌ cl.exe 未找到
)

echo [2] 检查CMake...
where cmake >nul 2>&1 && (
    cmake --version | findstr /i "cmake" && echo ✅ CMake 找到
) || (
    echo ❌ CMake 未找到
)

echo [3] 检查Windows SDK...
if exist "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0" (
    echo ✅ Windows SDK 10.0.22621.0 找到
) else (
    echo ❌ Windows SDK 未找到
)

echo [4] 检查项目文件...
if exist "include\quickinput\ime\text_service.h" (
    echo ✅ 项目头文件存在
) else (
    echo ❌ 项目文件缺失
)

echo.
echo === 环境验证完成 ===
pause
```

### 2. 手动验证步骤

#### 步骤1：编译测试
```cmd
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 如果成功，应该看到：
# -- The C compiler identification is MSVC ...
# -- The CXX compiler identification is MSVC ...
# -- Check for working C compiler: C:/.../cl.exe
# -- Check for working CXX compiler: C:/.../cl.exe
```

#### 步骤2：编译项目
```cmd
# 生成解决方案文件
cmake --build . --config Release

# 预期结果：
# quickinput_ime.dll - 主IME模块
# quickinput_config.exe - 配置面板
# test_ai.exe - AI模块测试
# test_kb.exe - 知识库测试
```

#### 步骤3：安装输入法
```cmd
# 以管理员身份运行
cmake --install . --config Release

# 验证安装
reg query "HKLM\SOFTWARE\Microsoft\CTF\SystemShared"
```

## 故障排除

### 常见问题和解决方案

#### 问题1：CMake找不到编译器
**症状**: `Could not find a C++ compiler`
**解决方案**:
```cmd
# 使用Visual Studio开发者命令提示符
"C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
# 然后重新运行CMake
```

#### 问题2：Windows SDK版本不匹配
**症状**: `Windows SDK version '10.0.19041.0' is too old`
**解决方案**:
```cmd
# 安装更新的SDK或使用现有版本
dir "C:\Program Files (x86)\Windows Kits\10\Include\*.*" /O:D
# 选择最新的版本号更新环境变量
```

#### 问题3：权限不足
**症状**: `Access denied` when installing
**解决方案**:
- 以管理员身份运行命令提示符
- 或者临时关闭防病毒软件

#### 问题4：链接错误
**症状**: `LNK1104 cannot open file 'xxx.lib'`
**解决方案**:
```cmd
# 确保Windows Kit库路径正确
setx LIB "C:\Program Files (x86)\Windows Kits\10\Lib\*;%LIB%"
```

### 日志和诊断

#### CMake日志位置
```
build/CMakeCache.txt
build/CMakeFiles/CMakeOutput.log
build/CMakeFiles/CMakeError.log
```

#### Visual Studio构建日志
```cmd
# 详细构建日志
cmake --build . --config Release --verbose
```

## 性能优化建议

### 1. 并行编译
```cmd
# 使用多核编译加速
cmake --build . --config Release --parallel 8
```

### 2. 增量编译
```cmd
# 只编译修改的模块
cmake --build . --target quickinput_ai
```

### 3. 缓存配置
```bash
# 第一次配置后缓存配置，后续更快
cmake .. -G "Visual Studio 17 2022" -A x64
```

## 参考资源

- [Visual Studio 2022 下载](https://visualstudio.microsoft.com/downloads/)
- [CMake 下载](https://cmake.org/download/)
- [Windows SDK 文档](https://learn.microsoft.com/en-us/windows/win32/sdk/)
- [TSF 开发指南](https://learn.microsoft.com/en-us/windows/win32/tsf/)

---

**最后更新**: 2026年5月21日
**文档版本**: 1.0.0