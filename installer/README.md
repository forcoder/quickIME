# QuickInput 安装程序模块

## 概述

QuickInput 安装程序模块负责处理项目的 Windows 安装、部署和分发。该模块包含完整的安装解决方案，支持标准安装程序、便携版制作和自动化构建。

## 目录结构

```
installer/
├── setup.iss                 # Inno Setup 主安装脚本
├── CMakeLists.txt           # CMake 集成配置
├── build_installer.cmake    # 独立构建脚本
├── register_ime.bat         # IME 注册脚本
├── unregister_ime.bat       # IME 注销脚本
├── validate_installer.cmake # 验证脚本
├── version_info.txt.in      # 版本信息模板
├── build_report_template.txt.in # 构建报告模板
└── README.md               # 本文件
```

## 快速开始

### 1. 安装依赖

```bash
# 安装 Inno Setup (Windows)
choco install innosetup --version=6.2.2

# 或从官网下载: https://jrsoftware.org/isinfo.php
```

### 2. 构建安装程序

```bash
cd installer
cmake -P build_installer.cmake
```

### 3. 手动构建

```bash
# 使用 Inno Setup 编译器
ISCC.exe setup.iss
```

## 配置文件说明

### setup.iss

主安装脚本，定义：

- **安装设置**: 应用程序名称、版本、安装路径
- **文件复制**: 所有组件文件的部署位置
- **注册表操作**: COM 组件注册和输入法配置
- **快捷方式**: 开始菜单和桌面图标
- **卸载支持**: 完整的卸载功能

### CMakeLists.txt

CMake 集成配置，提供：

- Inno Setup 编译器检测
- 自定义构建目标
- 资源文件管理
- 构建验证

## 高级用法

### 静默安装

```cmd
QuickInput_Setup.exe /S /V"/qn"
```

### 自定义安装路径

```cmd
QuickInput_Setup.exe /DIR="D:\Custom\Path"
```

### 开发环境构建

```bat
:: build_dev.bat
@echo off
set BUILD_TYPE=Debug
cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --config Debug
cd build/debug && ISCC.exe setup.iss
```

## 故障排除

### 常见问题

**Q: 安装程序找不到文件**
A: 确保所有源文件都在正确的相对路径下

**Q: 注册表权限错误**
A: 以管理员身份运行安装程序

**Q: Inno Setup 编译失败**
A: 检查 ISCC 路径是否正确添加到系统 PATH

### 日志文件

- 安装日志: `%TEMP%\is-uninst.log`
- 注册表操作日志: `%APPDATA%\QuickInput\logs\install.log`

## 贡献指南

1. 修改 `setup.iss` 时确保遵循现有格式
2. 更新相应的文档文件
3. 测试安装包在不同 Windows 版本上的兼容性
4. 提交前运行 `validate_installer.cmake`

## 许可证

本项目遵循 QuickInput 整体项目许可证。