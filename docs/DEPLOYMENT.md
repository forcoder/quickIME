# QuickInput 部署指南

## 目录

- [部署概述](#部署概述)
- [CI/CD 配置](#cicd-配置)
- [自动化构建](#自动化构建)
- [发布流程](#发布流程)
- [版本管理](#版本管理)
- [回滚策略](#回滚策略)

---

## 部署概述

### 支持的部署目标
1. **Windows Installer**: Inno Setup 安装程序
2. **Portable Version**: 绿色便携版
3. **Development Build**: 开发环境构建
4. **Enterprise Deployment**: 企业批量部署

### 部署架构
```
QuickInput Project
├── installer/
│   ├── setup.iss (Inno Setup脚本)
│   ├── CMakeLists.txt (CMake集成)
│   └── build_installer.cmake (独立构建脚本)
├── portable/
│   └── portable_version.bat (便携版制作)
├── docs/
│   ├── INSTALLATION.md (安装指南)
│   └── DEPLOYMENT.md (本文件)
└── resources/
    ├── models/ (模型文件)
    └── config/ (配置文件)
```

---

## CI/CD 配置

### GitHub Actions 工作流

创建 `.github/workflows/deploy.yml`:

```yaml
name: QuickInput Deploy

on:
  push:
    tags:
      - 'v*'
  workflow_dispatch:

jobs:
  build-installer:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup Visual Studio
      uses: ilammy/msvc-dev-cmd@v1
      
    - name: Setup Inno Setup
      run: choco install innosetup --version=6.2.2
      
    - name: Configure CMake
      run: cmake -B ${{github.workspace}}/build -DCMAKE_BUILD_TYPE=Release
      
    - name: Build Project
      run: cmake --build ${{github.workspace}}/build --config Release
      
    - name: Build Installer
      working-directory: ${{github.workspace}}/installer
      run: cmake -P build_installer.cmake
      
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: quickinput-installer
        path: |
          build/release/QuickInput_Setup.exe
          build/release/QuickInput_Portable_*.zip

  release:
    needs: build-installer
    runs-on: ubuntu-latest
    
    steps:
    - name: Download artifacts
      uses: actions/download-artifact@v3
      with:
        name: quickinput-installer
        
    - name: Create Release
      uses: softprops/action-gh-release@v1
      if: startsWith(github.ref, 'refs/tags/')
      with:
        files: |
          QuickInput_Setup.exe
          QuickInput_Portable_*.zip
        draft: false
        prerelease: false
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    
    environment {
        PROJECT_NAME = 'quickinput'
        VERSION = "${env.BUILD_NUMBER}"
    }
    
    stages {
        stage('Build') {
            steps {
                bat '''
                    mkdir build
                    cd build
                    cmake ..
                    cmake --build . --config Release
                '''
            }
        }
        
        stage('Package') {
            steps {
                bat '''
                    cd installer
                    cmake -P build_installer.cmake
                '''
            }
        }
        
        stage('Test') {
            steps {
                bat 'ctest --output-on-failure'
            }
        }
        
        stage('Deploy') {
            when { branch 'main' }
            steps {
                archiveArtifacts artifacts: 'build/release/*.exe,build/release/*.zip'
                // Add deployment to server
                bat 'xcopy /E /I build\\release \\\\\\server\\deploy\\quickinput\\'
            }
        }
    }
}
```

---

## 自动化构建

### 本地构建脚本

创建 `build_local.bat`:

```bat
@echo off
:: QuickInput Local Build Script

setlocal enabledelayedexpansion

echo [INFO] Starting QuickInput local build...
echo.

:: Check for required tools
where cmake >nul 2>&1 || (
    echo [ERROR] CMake not found. Please install CMake >= 3.20
    pause
    exit /b 1
)

where ISCC >nul 2>&1 || (
    echo [WARN] Inno Setup Compiler not found. Install from https://jrsoftware.org/isinfo.php
)

:: Create build directory
if not exist "build" mkdir "build"
cd build

:: Configure and build
echo [INFO] Configuring project...
cmake .. -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo [ERROR] Configuration failed
    pause
    exit /b 1
)

echo [INFO] Building project...
cmake --build . --config Release --target ALL_BUILD

if errorlevel 1 (
    echo [ERROR] Build failed
    pause
    exit /b 1
)

:: Build installer if possible
if exist "..\installer\ISCC.exe" (
    echo [INFO] Building installer...
    cmake -P ..\installer\build_installer.cmake
)

echo [SUCCESS] Build completed successfully!
echo Output location: %CD%\bin\Release\
pause
```

### 跨平台构建支持

对于 Linux/macOS 系统，创建 `build_unix.sh`:

```bash
#!/bin/bash
# QuickInput Unix Build Script

set -e

echo "[INFO] Starting QuickInput Unix build..."

# Install dependencies
if command -v apt-get &> /dev/null; then
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build mingw-w64
elif command -v brew &> /dev/null; then
    brew install cmake ninja mingw-w64
fi

# Cross-compile for Windows
mkdir -p build/windows
cd build/windows
cmake ../../.. \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-windows.cmake \
    -DCMAKE_BUILD_TYPE=Release

ninja

# Create installer package
cd ../..
./installer/build_installer.cmake

echo "[SUCCESS] Cross-platform build completed!"
```

---

## 发布流程

### 版本发布检查清单

1. **代码准备**
   - [ ] 所有测试通过
   - [ ] 代码审查完成
   - [ ] 文档更新
   - [ ] 版本号递增

2. **构建验证**
   - [ ] 本地构建成功
   - [ ] 安装程序生成
   - [ ] 便携版创建
   - [ ] 文件完整性检查

3. **测试验证**
   - [ ] 功能测试通过
   - [ ] 兼容性测试
   - [ ] 性能测试
   - [ ] 安全扫描

4. **发布准备**
   - [ ] 更新日志编写
   - [ ] 签名证书验证
   - [ ] 分发包准备
   - [ ] 回滚计划制定

### 手动发布步骤

1. **创建发布分支**
   ```bash
   git checkout -b release/v1.0.0
   ```

2. **更新版本信息**
   ```bash
   # 更新 CMakeLists.txt 中的版本号
   # 更新 installer/version_info.txt.in
   # 更新 docs/INSTALLATION.md
   ```

3. **构建发布版本**
   ```bash
   ./installer/build_installer.cmake
   ```

4. **验证构建产物**
   ```bash
   # 检查文件大小和完整性
   ls -lh build/release/
   ```

5. **创建 Git 标签**
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

6. **上传发布包**
   ```bash
   # 上传到 GitHub Releases
   # 上传到公司内部服务器
   ```

---

## 版本管理

### 语义化版本控制

遵循 [Semantic Versioning 2.0.0](https://semver.org/)

格式：`MAJOR.MINOR.PATCH`

- **MAJOR**: 不兼容的 API 更改
- **MINOR**: 向后兼容的功能性新增
- **PATCH**: 向后兼容的问题修正

### 版本命名规范

- 主版本: `v1.0.0`, `v2.0.0`
- 预览版本: `v1.0.0-beta1`, `v1.0.0-rc2`
- 开发版本: `v1.0.0-dev.1`

### 版本文件结构

```
project-root/
├── CMakeLists.txt (version: 1.0.0)
├── installer/
│   ├── setup.iss (AppVersion=1.0.0)
│   └── version_info.txt.in (ProjectVersion=1.0.0)
├── src/
│   └── main.cpp (VERSION_STRING "1.0.0")
└── docs/
    └── CHANGELOG.md
```

---

## 回滚策略

### 快速回滚方案

1. **安装程序回滚**
   ```cmd
   # 卸载当前版本
   QuickInput_Setup.exe /uninstall
   
   # 重新安装旧版本
   QuickInput_Setup_v0.9.8.exe
   ```

2. **配置文件备份**
   ```cmd
   # 备份用户配置
   xcopy "%APPDATA%\QuickInput" "backup\quickinput-config\" /E /I
   ```

3. **注册表恢复**
   ```cmd
   # 从备份恢复注册表
   reg import quickinput-backup.reg
   ```

### 自动化回滚脚本

创建 `rollback.bat`:

```bat
@echo off
:: QuickInput Rollback Script

setlocal enabledelayedexpansion

set VERSION=%1
if "%VERSION%"=="" set VERSION=v0.9.8

echo [INFO] Rolling back to version %VERSION%...

:: Stop running processes
taskkill /f /im "QuickInput.exe" /t >nul 2>&1
taskkill /f /im "config_panel.exe" /t >nul 2>&1

:: Backup current installation
set BACKUP_DIR="%TEMP%\quickinput-backup-%DATE:-=_%-%TIME::=-%"
mkdir "%BACKUP_DIR%"

xcopy "%PROGRAMFILES%\QuickInput" "%BACKUP_DIR%\current\" /E /I /Q
xcopy "%APPDATA%\QuickInput" "%BACKUP_DIR%\config\" /E /I /Q

:: Uninstall current version
start /wait "" "%PROGRAMFILES%\QuickInput\uninstall.exe" /silent

:: Install previous version
if exist "QuickInput_Setup_%VERSION%.exe" (
    start /wait "" "QuickInput_Setup_%VERSION%.exe" /silent
) else (
    echo [WARN] Previous version installer not found
)

:: Restore user configuration
if exist "%BACKUP_DIR%\config" (
    xcopy "%BACKUP_DIR%\config" "%APPDATA%\QuickInput\" /E /I /Q /Y
)

echo [SUCCESS] Rollback to version %VERSION% completed
echo Backup saved to: %BACKUP_DIR%
pause
```

### 监控和告警

设置监控系统以检测部署后的异常：

1. **错误日志监控**
   - 实时监控应用程序日志
   - 设置异常阈值告警

2. **性能指标监控**
   - CPU 和内存使用监控
   - 响应时间监控

3. **用户反馈收集**
   - 自动收集崩溃报告
   - 用户满意度调查

---

## 最佳实践

### 安全部署建议

1. **代码签名**
   - 对所有可执行文件进行数字签名
   - 使用有效的证书颁发机构

2. **防病毒白名单**
   - 将安装目录添加到防病毒软件白名单
   - 避免误报为恶意软件

3. **权限最小化**
   - 使用最低必要权限运行
   - 避免管理员权限需求

### 性能优化

1. **增量更新**
   - 实现差异更新机制
   - 减少下载大小

2. **缓存优化**
   - 智能缓存模型文件
   - 预加载常用资源

3. **启动优化**
   - 延迟加载非关键组件
   - 并行初始化

### 用户体验改进

1. **静默安装选项**
   ```cmd
   QuickInput_Setup.exe /S /V"/qn"
   ```

2. **自动更新检查**
   - 后台检查新版本
   - 提示用户更新

3. **故障自愈**
   - 自动修复常见问题
   - 提供自助解决方案