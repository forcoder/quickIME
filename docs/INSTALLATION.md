# QuickInput 智能五笔输入法 - 安装指南

## 目录

- [系统要求](#系统要求)
- [安装步骤](#安装步骤)
- [卸载说明](#卸载说明)
- [常见问题](#常见问题)
- [故障排除](#故障排除)
- [技术支持](#技术支持)

---

## 系统要求

### 最低配置要求
- **操作系统**: Windows 7 SP1 / Windows 8.1 / Windows 10 / Windows 11
- **处理器**: Intel Core i3 或同等性能处理器
- **内存**: 4 GB RAM
- **硬盘空间**: 500 MB 可用空间
- **显示器**: 1024x768 分辨率或更高

### 推荐配置要求
- **操作系统**: Windows 10 版本 20H2 或更高版本
- **处理器**: Intel Core i5 或 AMD Ryzen 5 或更高
- **内存**: 8 GB RAM 或更高
- **硬盘空间**: 1 GB SSD 可用空间
- **显示器**: 1920x1080 分辨率或更高

### 软件依赖
- Microsoft Visual C++ Redistributable (2015-2022)
- .NET Framework 4.8 或更高版本
- DirectX 11 兼容显卡驱动

---

## 安装步骤

### 方法一：使用安装程序（推荐）

1. **下载安装包**
   - 从官方网站下载 `QuickInput_Setup.exe`
   - 或者从构建目录获取最新安装包

2. **运行安装程序**
   ```cmd
   QuickInput_Setup.exe
   ```

3. **按照安装向导操作**
   - 阅读许可协议并接受条款
   - 选择安装位置（默认：`C:\Program Files\QuickInput`）
   - 选择开始菜单文件夹
   - 勾选创建桌面图标选项

4. **等待安装完成**
   - 安装程序会自动注册输入法组件
   - 显示进度条和状态信息

5. **完成安装**
   - 点击"完成"按钮
   - 可以选择立即启动配置面板

### 方法二：便携版安装

1. **下载便携版**
   - 获取 `QuickInput_Portable_v1.0.0.zip`

2. **解压文件**
   ```cmd
   unzip QuickInput_Portable_v1.0.0.zip -d "D:\QuickInput_Portable"
   ```

3. **运行程序**
   - 双击 `bin\QuickInput.exe` 启动程序
   - 可选运行 `register_portable.bat` 进行注册表注册

### 方法三：开发版安装

1. **克隆代码库**
   ```bash
   git clone https://github.com/yourorg/quickinput.git
   cd quickinput
   ```

2. **构建项目**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **复制到安装目录**
   - 将 `bin\` 下的文件复制到目标目录
   - 复制 `models\` 和 `resources\` 目录

---

## 输入法设置

### 启用 QuickInput 输入法

1. **打开控制面板**
   - 按 `Win + R`，输入 `control` 回车

2. **进入语言设置**
   - 点击"时钟、语言和区域"
   - 点击"语言" → "首选语言"

3. **添加输入法**
   - 点击"选项"按钮
   - 在"键盘"部分点击"添加键盘"
   - 选择"QuickInput 智能五笔"

4. **切换输入法**
   - 按 `Win + Space` 切换输入法
   - 或使用 `Ctrl + Shift` 循环切换

### 配置文件位置

- **用户配置**: `%APPDATA%\QuickInput\config\`
- **全局配置**: `%PROGRAMDATA%\QuickInput\`
- **日志文件**: `%APPDATA%\QuickInput\logs\`

---

## 卸载说明

### 通过控制面板卸载

1. **打开控制面板**
   ```cmd
   control appwiz.cpl
   ```

2. **找到 QuickInput**
   - 在程序列表中找到"QuickInput 智能五笔输入法"

3. **执行卸载**
   - 点击"卸载"按钮
   - 确认卸载操作

4. **重启计算机**（建议）

### 手动卸载步骤

1. **删除安装目录**
   ```cmd
   rmdir /s /q "C:\Program Files\QuickInput"
   ```

2. **清理注册表**
   ```cmd
   reg delete "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f
   reg delete "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f
   reg delete "HKCU\SOFTWARE\QuickInput" /f
   ```

3. **清理用户数据**
   ```cmd
   rmdir /s /q "%APPDATA%\QuickInput"
   ```

---

## 常见问题

### Q1: 安装后无法启动 QuickInput
**解决方案**:
1. 检查是否安装了 Microsoft Visual C++ Redistributable
2. 以管理员身份运行安装程序
3. 检查防病毒软件是否阻止了程序运行
4. 查看日志文件获取详细错误信息

### Q2: 输入法无法切换
**解决方案**:
1. 确保已正确注册输入法组件
2. 检查系统语言设置
3. 尝试重启输入法服务
   ```cmd
   taskkill /f /im "quickinput_ime.dll"
   regsvr32 "C:\Program Files\QuickInput\quickinput_ime.dll"
   ```

### Q3: 编码输入无响应
**解决方案**:
1. 检查模型文件是否完整复制
2. 验证配置文件是否正确
3. 重新注册 COM 组件
   ```cmd
   regsvr32 "C:\Program Files\QuickInput\quickinput_ime.dll"
   ```

### Q4: 候选词显示异常
**解决方案**:
1. 检查显卡驱动是否需要更新
2. 调整显示器 DPI 设置
3. 修改配置文件中的字体设置

### Q5: 安装程序提示缺少依赖
**解决方案**:
1. 安装所有必要的运行时组件
2. 下载并安装 .NET Framework 4.8
3. 安装 Visual C++ Redistributable 包

---

## 故障排除

### 1. 检查系统兼容性

```cmd
systeminfo | findstr /C:"OS" /C:"System Type"
```

### 2. 验证文件完整性

检查以下关键文件是否存在：
- `bin\QuickInput.exe`
- `bin\config_panel.exe`
- `models\*.model`
- `resources\*.*`

### 3. 查看日志文件

日志文件位置：`%APPDATA%\QuickInput\logs\`

### 4. 重新注册组件

```cmd
cd "C:\Program Files\QuickInput"
register_ime.bat
regsvr32 quickinput_ime.dll
```

### 5. 清理临时文件

```cmd
del /q %TEMP%\quickinput_*
rmdir /s /q %TEMP%\quickinput_*
```

### 6. 重置配置

删除配置文件后重新启动程序：
```cmd
rmdir /s /q "%APPDATA%\QuickInput\config"
QuickInput.exe
```

---

## 高级配置

### 自定义安装路径

在安装向导中，点击"浏览"按钮选择自定义安装位置。

### 静默安装

使用命令行参数进行无人值守安装：
```cmd
QuickInput_Setup.exe /S /D=C:\Custom\Path
```

### 企业部署

对于企业环境，可以使用组策略或脚本进行批量部署。

---

## 技术支持

如果以上解决方案都无法解决问题，请联系技术支持团队：

- **邮箱**: support@quickinput.cn
- **QQ群**: 123456789
- **论坛**: https://forum.quickinput.cn
- **GitHub Issues**: https://github.com/yourorg/quickinput/issues

提供以下信息有助于更快解决问题：
1. 操作系统版本
2. QuickInput 版本号
3. 错误日志文件
4. 问题复现步骤

---

## 版本历史

### v1.0.0 (当前版本)
- 初始发布版本
- 支持 Windows 7 及以上系统
- 完整的五笔编码功能
- 智能预测引擎
- 可视化配置面板

### 更新日志
- 优化了内存占用
- 修复了编码延迟问题
- 增加了新的皮肤主题
- 提升了候选词准确率