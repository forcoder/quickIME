# QuickInput 部署需求文档

## 目录

- [系统架构](#系统架构)
- [构建依赖](#构建依赖)
- [部署环境要求](#部署环境要求)
- [CI/CD 工具链](#cicd-工具链)
- [自动化脚本](#自动化脚本)
- [监控与运维](#监控与运维)

---

## 系统架构

### 组件分解

```
QuickInput 部署架构
├── 核心组件 (bin/)
│   ├── QuickInput.exe (主程序)
│   ├── config_panel.exe (配置面板)
│   └── quickinput_ime.dll (IME服务)
├── 数据组件 (models/)
│   ├── encoding_model.bin (编码模型)
│   ├── prediction_model.bin (预测模型)
│   └── language_model.bin (语言模型)
├── 资源组件 (resources/)
│   ├── fonts/ (字体文件)
│   ├── themes/ (主题文件)
│   └── localization/ (本地化文件)
└── 配置组件 (config/)
    ├── default.json (默认配置)
    ├── hotkeys.json (快捷键配置)
    └── user_preferences.json (用户偏好)
```

### 通信协议

- **进程间通信**: Windows COM + RPC
- **配置文件格式**: JSON
- **日志格式**: 结构化文本日志
- **网络协议**: HTTP/HTTPS (可选云同步)

---

## 构建依赖

### 必需软件包

| 组件 | 版本要求 | 说明 |
|------|----------|------|
| CMake | >= 3.20 | 跨平台构建系统 |
| Inno Setup | v6.x | Windows安装程序制作 |
| Visual Studio | 2019/2022 | C++编译器 |
| Git | latest | 版本控制 |

### 运行时依赖

| 依赖项 | 版本要求 | 获取方式 |
|--------|----------|----------|
| Microsoft Visual C++ Redistributable | 2015-2022 | 自动安装 |
| .NET Framework | 4.8+ | Windows自带 |
| DirectX Runtime | 11 | Windows自带 |

### 第三方库

```cmake
# CMakeLists.txt 中的依赖
find_package(Boost REQUIRED COMPONENTS filesystem system)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Protobuf REQUIRED)
```

---

## 部署环境要求

### 开发环境

#### Windows 开发机
- **操作系统**: Windows 10/11 (64位)
- **内存**: 16GB RAM
- **存储**: 50GB SSD
- **工具**: VS2019/VS2022, CMake, Git

#### Linux 交叉编译环境
- **主机**: Ubuntu 20.04 LTS
- **目标**: Windows x64
- **工具链**: mingw-w64, wine

### 生产环境

#### 服务器要求
- **CPU**: 2核以上
- **内存**: 4GB RAM
- **存储**: 10GB可用空间
- **网络**: 互联网连接

#### 客户端要求
- **Windows 7 SP1** 或更高版本
- **DirectX 11** 兼容显卡
- **显示器**: 1024x768 分辨率

---

## CI/CD 工具链

### GitHub Actions

```yaml
# .github/workflows/deploy.yml
name: Deploy QuickInput

on:
  push:
    branches: [ main ]
    tags: [ 'v*' ]

jobs:
  build-and-deploy:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup MSVC
      uses: ilammy/msvc-dev-cmd@v1
      
    - name: Install Inno Setup
      run: choco install innosetup --version=6.2.2
      
    - name: Configure and Build
      run: |
        mkdir build
        cd build
        cmake .. -G "Visual Studio 17 2022" -A x64
        cmake --build . --config Release
        
    - name: Package Installer
      working-directory: ${{github.workspace}}/installer
      run: cmake -P build_installer.cmake
      
    - name: Upload to Releases
      if: startsWith(github.ref, 'refs/tags/')
      uses: softprops/action-gh-release@v1
      with:
        files: build/release/*
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    tools {
        cmake 'CMake 3.20'
        visual_studio 'Visual Studio 2022'
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        
        stage('Build') {
            steps {
                bat '''
                    cmake -B build -DCMAKE_BUILD_TYPE=Release
                    cmake --build build --config Release
                '''
            }
        }
        
        stage('Test') {
            steps {
                bat 'ctest --output-on-failure'
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
        
        stage('Deploy') {
            when { branch 'main' }
            steps {
                archiveArtifacts artifacts: 'build/release/**/*'
                // Add deployment steps
            }
        }
    }
}
```

### GitLab CI

```yaml
stages:
  - build
  - test
  - package
  - deploy

variables:
  BUILD_DIR: "build"
  INSTALLER_DIR: "installer"

build:
  stage: build
  script:
    - mkdir -p $BUILD_DIR
    - cd $BUILD_DIR && cmake .. -DCMAKE_BUILD_TYPE=Release
    - cmake --build . --config Release

test:
  stage: test
  script:
    - cd $BUILD_DIR && ctest --output-on-failure

package:
  stage: package
  script:
    - cd $INSTALLER_DIR && cmake -P build_installer.cmake
  artifacts:
    paths:
      - build/release/

deploy_production:
  stage: deploy
  script:
    - echo "Deploying to production..."
    # Add deployment commands
  only:
    - main
```

---

## 自动化脚本

### 构建脚本

`build.bat` - Windows 本地构建

```bat
@echo off
:: QuickInput Build Script v1.0
:: Automated build process for development and deployment

setlocal enabledelayedexpansion

:: Configuration
set PROJECT_NAME=QuickInput
set VERSION=1.0.0
set BUILD_DIR=build
set OUTPUT_DIR=dist

:: Colors
set "INFO=[INFO]"
set "WARN=[WARN]"
set "ERROR=[ERROR]"

echo %INFO% Starting %PROJECT_NAME% %VERSION% build...

:: Check prerequisites
call :check_prerequisites || exit /b 1

:: Clean previous builds
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"

:: Create directories
mkdir "%BUILD_DIR%"
mkdir "%OUTPUT_DIR%"

:: Configure and build
echo %INFO% Configuring project...
cd "%BUILD_DIR%"
cmake .. -DCMAKE_BUILD_TYPE=Release -DPROJECT_VERSION=%VERSION%
if errorlevel 1 (
    echo %ERROR% CMake configuration failed
    exit /b 1
)

echo %INFO% Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo %ERROR% Build failed
    exit /b 1
)

:: Package installer
echo %INFO% Packaging installer...
cd ..
call :package_installer
if errorlevel 1 (
    echo %ERROR% Failed to package installer
    exit /b 1
)

echo %INFO% Build completed successfully!
echo Output location: %OUTPUT_DIR%
goto :eof

:: Function to check prerequisites
:check_prerequisites
where cmake >nul 2>&1
if errorlevel 1 (
    echo %ERROR% CMake not found. Please install CMake >= 3.20
    exit /b 1
)

where git >nul 2>&1
if errorlevel 1 (
    echo %ERROR% Git not found. Please install Git.
    exit /b 1
)

exit /b 0

:: Function to package installer
:package_installer
if exist "installer\ISCC.exe" (
    installer\ISCC.exe setup.iss
    if exist "QuickInput_Setup.exe" (
        copy "QuickInput_Setup.exe" "%OUTPUT_DIR%\"
        echo %INFO% Installer packaged successfully
        exit /b 0
    )
)
echo %WARN% Inno Setup not available, skipping installer packaging
exit /b 0
```

### 部署脚本

`deploy.bat` - 自动化部署

```bat
@echo off
:: QuickInput Deployment Script
:: Handles automated deployment to various environments

setlocal enabledelayedexpansion

:: Deployment configuration
set DEPLOY_ENV=%1
if "%DEPLOY_ENV%"=="" set DEPLOY_ENV=production

set SOURCE_DIR=dist
set TARGET_DIR=C:\Program Files\QuickInput

echo [INFO] Deploying QuickInput to %DEPLOY_ENV% environment...

:: Validate source
if not exist "%SOURCE_DIR%\QuickInput_Setup.exe" (
    echo [ERROR] Source installer not found
    exit /b 1
)

:: Environment-specific configurations
if "%DEPLOY_ENV%"=="development" (
    set TARGET_DIR=D:\Dev\QuickInput
    set SILENT=false
) else if "%DEPLOY_ENV%"=="staging" (
    set TARGET_DIR=E:\Staging\QuickInput
    set SILENT=false
) else (
    set SILENT=true
)

:: Stop running processes
echo [INFO] Stopping QuickInput processes...
taskkill /f /im "QuickInput.exe" /t >nul 2>&1
taskkill /f /im "config_panel.exe" /t >nul 2>&1

:: Backup existing installation
if exist "%TARGET_DIR%" (
    set BACKUP_DIR="%TEMP%\quickinput-backup-%DATE:-=_%-%TIME::=-%"
    echo [INFO] Backing up current installation...
    xcopy "%TARGET_DIR%" "%BACKUP_DIR%\" /E /I /Q /Y
    echo [INFO] Backup saved to: %BACKUP_DIR%
)

:: Install new version
echo [INFO] Installing new version...
start /wait "" "%SOURCE_DIR%\QuickInput_Setup.exe" /S /D=%TARGET_DIR%
if errorlevel 1 (
    echo [ERROR] Installation failed
    exit /b 1
)

:: Verify installation
if not exist "%TARGET_DIR%\QuickInput.exe" (
    echo [ERROR] Installation verification failed
    exit /b 1
)

:: Restart services
echo [INFO] Restarting QuickInput services...
net start "QuickInput Service" >nul 2>&1

echo [SUCCESS] Deployment completed successfully!
echo Target directory: %TARGET_DIR%
pause
```

---

## 监控与运维

### 健康检查脚本

`health_check.bat`:

```bat
@echo off
:: QuickInput Health Check Script

set HEALTH_SCORE=0
set MAX_SCORE=100

echo QuickInput Health Check Report
echo ================================

:: Check 1: Process status (10 points)
tasklist /fi "imagename eq QuickInput.exe" 2>nul | find "QuickInput.exe" >nul
if %errorlevel% equ 0 (
    echo [OK] Main process running
    set /a HEALTH_SCORE+=10
) else (
    echo [FAIL] Main process not running
)

:: Check 2: Service status (10 points)
sc query "QuickInput Service" | find "RUNNING" >nul
if %errorlevel% equ 0 (
    echo [OK] Service running
    set /a HEALTH_SCORE+=10
) else (
    echo [WARN] Service not running
)

:: Check 3: File integrity (20 points)
if exist "%PROGRAMFILES%\QuickInput\QuickInput.exe" (
    echo [OK] Core executable present
    set /a HEALTH_SCORE+=20
) else (
    echo [FAIL] Core executable missing
)

:: Check 4: Registry entries (15 points)
reg query "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" >nul
if %errorlevel% equ 0 (
    echo [OK] Registry entries present
    set /a HEALTH_SCORE+=15
) else (
    echo [WARN] Registry entries missing
)

:: Check 5: Model files (25 points)
if exist "%PROGRAMFILES%\QuickInput\models\*.model" (
    echo [OK] Model files present
    set /a HEALTH_SCORE+=25
) else (
    echo [FAIL] Model files missing
)

:: Check 6: User data (20 points)
if exist "%APPDATA%\QuickInput\logs\*.log" (
    echo [OK] User data accessible
    set /a HEALTH_SCORE+=20
) else (
    echo [WARN] User data not accessible
)

:: Display results
echo.
echo Health Score: %HEALTH_SCORE%/%MAX_SCORE%
echo Status: %HEALTH_SCORE%%% Complete

if %HEALTH_SCORE% geq 90 (
    echo Overall Status: EXCELLENT
) else if %HEALTH_SCORE% geq 70 (
    echo Overall Status: GOOD
) else if %HEALTH_SCORE% geq 50 (
    echo Overall Status: FAIR
) else (
    echo Overall Status: POOR
)

echo.
pause
```

### 日志轮转配置

创建 `logrotate.conf`:

```
# QuickInput Log Rotation Configuration

/var/log/quickinput/*.log {
    daily
    missingok
    rotate 30
    compress
    delaycompress
    notifempty
    create 644 quickinput quickinput
    postrotate
        /usr/bin/killall -HUP quickinput
    endscript
}

# Windows equivalent using PowerShell
$LogPath = "$env:APPDATA\QuickInput\logs"
Get-ChildItem $LogPath\* -Recurse | Where-Object {$_.LastWriteTime -lt (Get-Date).AddDays(-30)} | Remove-Item -Force
```

### 性能监控

创建 `performance_monitor.ps1`:

```powershell
# QuickInput Performance Monitor
param(
    [int]$Duration = 300,
    [string]$OutputFile = "performance.log"
)

$StartTime = Get-Date
$EndTime = $StartTime.AddSeconds($Duration)

"QuickInput Performance Monitoring - $(Get-Date)" | Out-File $OutputFile -Append

while ((Get-Date) -lt $EndTime) {
    $CpuUsage = (Get-Counter '\Process\QuickInput*\% Processor Time').CounterSamples.CookedValue | Measure-Object -Average
    $MemoryUsage = (Get-Counter '\Process\QuickInput*\Working Set').CounterSamples.CookedValue | Measure-Object -Average
    
    "$((Get-Date).ToString()) - CPU: $([math]::Round($CpuUsage.Average,2))%, Memory: $([math]::Round($MemoryUsage.Average/1MB,2)) MB" | 
        Out-File $OutputFile -Append
    
    Start-Sleep -Seconds 10
}
```

---

## 故障恢复

### 灾难恢复计划

1. **数据备份策略**
   - 每日增量备份
   - 每周完整备份
   - 异地备份存储

2. **快速恢复步骤**
   ```bat
   :: Recovery.bat
   @echo off
   echo Starting QuickInput recovery...

   :: Restore from backup
   robocopy \\backup-server\quickinput\%DATE% "%PROGRAMFILES%\QuickInput\" /MIR

   :: Re-register components
   regsvr32 "%PROGRAMFILES%\QuickInput\quickinput_ime.dll"

   :: Restart service
   net start "QuickInput Service"

   echo Recovery completed successfully!
   pause
   ```

3. **监控告警配置**
   - CPU 使用率 > 90% 持续 5 分钟
   - 内存使用率 > 85% 持续 10 分钟
   - 磁盘空间 < 10% 可用
   - 错误日志出现特定错误代码

### 安全考虑

1. **部署安全**
   - 所有传输使用 TLS 加密
   - 数字签名验证
   - 防病毒软件白名单

2. **权限管理**
   - 最小权限原则
   - 定期权限审查
   - 审计日志记录

3. **漏洞管理**
   - 定期安全扫描
   - 及时补丁更新
   - 渗透测试