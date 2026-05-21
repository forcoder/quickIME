# QuickInput IME - 构建和测试指南

## 快速开始

### 完整构建流程
```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 配置项目（启用所有功能）
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DQI_ENABLE_AI=ON \
         -DQI_BUILD_TESTS=ON \
         -DQI_BUILD_INSTALLER=ON

# 3. 编译项目
cmake --build . --config Release

# 4. 安装输入法（需要管理员权限）
cmake --install . --config Release
```

## 模块编译选项

### CMake配置参数详解

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `QI_ENABLE_AI` | ON | 启用AI推理模块 |
| `QI_BUILD_TESTS` | ON | 构建测试程序 |
| `QI_BUILD_INSTALLER` | ON | 构建安装程序 |
| `QI_ENABLE_CLOUD` | OFF | 启用云端LLM API |
| `CMAKE_BUILD_TYPE` | Release | Debug/Release模式 |

### 推荐配置组合

#### 开发调试模式
```bash
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DCMAKE_BUILD_TYPE=Debug \
         -DQI_ENABLE_AI=ON \
         -DQI_BUILD_TESTS=ON
```

#### 生产发布模式
```bash
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DCMAKE_BUILD_TYPE=Release \
         -DQI_ENABLE_AI=ON \
         -DQI_BUILD_INSTALLER=ON
```

#### 最小化构建（仅核心IME）
```bash
cmake .. -G "Visual Studio 17 2022" -A x64 \
         -DQI_ENABLE_AI=OFF \
         -DQI_BUILD_TESTS=OFF
```

## 单元测试执行

### AI模块测试
```bash
# 运行AI推理接口测试
.\test_ai.exe

# 预期输出示例：
=== QuickInput AI 推理接口测试 ===

1. 测试模型加载器...
支持的模型格式: gguf, bin, onnx
模型类型检测: C:\models\llama.gguf -> Llama.cpp
模型大小: 1.23 GB

2. 测试提示词模板管理器...
类型: 短句回复 - 短句回复 - 2-15字的简短实用回复
系统提示: 你是一个专业的五笔输入法智能辅助助手...

3. 测试推理引擎...
初始化结果: 成功
模型加载状态: 已加载

生成的AI建议:
建议 1: [短句回复] 文本: 好的，明白 | 置信度: 85%
建议 2: [工作话术] 文本: 请协助处理相关事务 | 置信度: 78%

4. 性能测试...
生成10次建议耗时: 125 ms
平均每次: 12 ms

测试完成
```

### 知识库模块测试
```bash
# 运行知识库管理测试
.\test_kb.exe

# 预期输出示例：
=== QuickInput 知识库测试 ===

1. 数据库初始化...
数据库创建成功

2. 导入测试文档...
导入文件: test.txt
条目数量: 156

3. 搜索测试...
查询: "如何设置"
相似度搜索结果: 8 条
关键词搜索结果: 12 条

4. 性能测试...
搜索100次平均耗时: 23 ms
内存使用: 45 MB

测试完成
```

### 上下文管理器测试
```bash
# 运行上下文管理器测试
.\test_context.exe

# 预期输出示例：
=== QuickInput 上下文管理器测试 ===

1. 初始化测试...
ContextManager 初始化成功

2. 热键管理测试...
热键注册: Ctrl+Shift+A - 开关AI联想
热键注册: Ctrl+Shift+K - 开关知识库联想

3. 输入监控测试...
输入事件捕获: 正常
候选词管理: 正常

4. 性能基准...
处理1000个输入事件: 156 ms
平均处理时间: 0.156 ms/event

测试完成
```

## 集成测试流程

### 步骤1：环境验证
```bash
# 运行环境检查脚本
verify_env.bat

# 确认所有必需组件都已安装
```

### 步骤2：基础编译
```bash
# 创建并进入构建目录
mkdir build && cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译主IME模块
cmake --build . --target quickinput_ime
```

### 步骤3：单元测试
```bash
# 运行所有测试程序
.\test_ai.exe
.\test_kb.exe
.\test_context.exe

# 检查测试结果是否全部通过
```

### 步骤4：安装测试
```powershell
# 以管理员身份运行命令提示符
# 安装项目
cmake --install . --config Release

# 验证注册表项
reg query "HKLM\SOFTWARE\Microsoft\CTF\SystemShared"

# 预期应该看到QuickInput相关的注册表项
```

### 步骤5：功能测试
```text
# 手动测试步骤：
1. 打开Windows设置 -> 时间和语言 -> 语言
2. 添加中文(简体, 中国) -> 选项
3. 添加键盘 -> QuickInput
4. 切换到QuickInput输入法
5. 在任意文本编辑器中测试：
   - 输入五笔编码，如"wubi"看候选词
   - 按空格选择候选词
   - 测试简码、词组输入
```

## 性能基准测试

### AI推理延迟测量
```bash
# 修改test_ai.cpp添加性能测量
./test_ai.exe --benchmark

# 目标性能指标：
# - 单次推理延迟: < 100ms
# - 并发处理能力: > 10请求/秒
# - 内存占用: < 500MB (典型负载)
```

### 知识库搜索性能
```bash
# 准备测试数据集
echo "测试数据..." > test_large.txt

# 导入到知识库
./test_kb.exe --import test_large.txt

# 性能测试
./test_kb.exe --performance

# 期望结果：
# - 索引构建时间: < 5秒 (1000条目)
# - 搜索响应时间: < 50ms
# - 内存增长: < 100MB
```

## 故障排除

### 常见构建问题

#### 问题1：CMake找不到编译器
**症状**: `Could not find a C++ compiler`
**解决方案**:
```cmd
# 使用Visual Studio开发者命令提示符
"C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
cd D:\workspace\cpp\quickInput
mkdir build
cd build
cmake ..
```

#### 问题2：链接错误
**症状**: `LNK1104 cannot open file 'xxx.lib'`
**解决方案**:
```cmd
# 检查Windows Kit库路径
dir "C:\Program Files (x86)\Windows Kits\10\Lib\*.*" /O:D

# 确保环境变量正确设置
setx LIB "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64;%LIB%"
```

#### 问题3：AI模块加载失败
**症状**: `Failed to load model`
**解决方案**:
```cmd
# 检查模型文件是否存在
dir "*.gguf" 2>nul || echo "模型文件缺失"

# 提供示例模型或mock实现
echo "使用模拟AI响应进行测试"
```

### 测试失败分析

#### AI测试失败
**症状**: `Test failed: Model loader initialization failed`
**可能原因**:
1. llama.cpp库未正确链接
2. 模型文件格式不支持
3. 缺少必要的依赖

**诊断步骤**:
```cmd
# 查看详细构建日志
cmake --build . --verbose

# 检查链接的库
dumpbin /DEPENDENTS test_ai.exe
```

#### 知识库测试失败
**症状**: `Database initialization failed`
**可能原因**:
1. SQLite3库未找到
2. 数据库权限问题
3. 磁盘空间不足

**诊断步骤**:
```cmd
# 检查SQLite3库
where sqlite3.dll

# 检查磁盘空间
fsutil volume diskfree C:
```

## 高级构建选项

### 交叉编译
```bash
# 为不同架构编译
cmake .. -G "Visual Studio 17 2022" -A Win32    # x86
cmake .. -G "Visual Studio 17 2022" -A ARM64    # ARM64
```

### 增量编译
```bash
# 只重新编译修改的模块
cmake --build . --target quickinput_ai --config Release

# 清理特定目标
cmake --build . --clean-first
```

### 自定义安装路径
```bash
# 指定安装目录
cmake --install . --prefix "C:\QuickInput"
```

## 持续集成配置

### GitHub Actions 示例
```yaml
name: Build and Test
on: [push, pull_request]
jobs:
  build:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v2
    - name: Configure CMake
      run: cmake -B ${{github.workspace}}/build -DCMAKE_BUILD_TYPE=Release
    - name: Build
      run: cmake --build ${{github.workspace}}/build
    - name: Test
      run: ${{github.workspace}}/build/test_ai.exe
```

## 参考资源

- [CMake官方文档](https://cmake.org/documentation/)
- [Visual Studio构建工具](https://learn.microsoft.com/en-us/cpp/build/reference/compiling-and-building)
- [TSF开发指南](https://learn.microsoft.com/en-us/windows/win32/tsf/)

---

**最后更新**: 2026年5月21日
**文档版本**: 1.0.0