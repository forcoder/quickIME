# QuickInput IME 开发指导

## 项目状态概览

### 已完成模块 (5/6)
- ✅ **模块1**: TSF输入法框架核心 (src/ime/, dllmain.cpp)
- ✅ **模块2**: 五笔编码引擎 (src/core/wubi_engine.*)
- ✅ **模块3**: 知识库管理 (src/knowledge/*) - 完成报告已生成
- ✅ **模块4**: AI推理接口 (src/ai/*) - 完成报告已生成
- ✅ **模块5**: 候选栏UI渲染 (src/ui/*) - 等待验证
- ❓ **模块6**: 配置面板应用 (config_panel/*) - 待集成测试

### 当前工作重点
**集成测试与验证阶段**
- 构建完整项目并测试各模块集成
- 验证输入法注册和基础输入功能
- 测试AI建议生成和知识库检索功能
- 确保配置面板正常工作

## 快速开始

### 编译要求
- Windows 10/11 (x64)
- Visual Studio 2022 (MSVC v143)
- CMake >= 3.20
- Windows SDK >= 10.0.22621

### 构建步骤
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64 -DQI_ENABLE_AI=ON -DQI_BUILD_TESTS=ON

# 编译
cmake --build . --config Release

# 安装（需要管理员权限）
cmake --install . --config Release
```

## 模块依赖关系

```
IME框架 (dllmain.cpp + src/ime/)
    ↓
五笔引擎 (src/core/wubi_engine.*)
    ↓
UI渲染 (src/ui/*)
    ↓
AI推理 (src/ai/*) ← 可选
知识库 (src/knowledge/*) ← 可选
    ↓
配置管理 (config_panel/*)
```

## 开发规范

### 代码风格
- **缩进**: 4空格（C++），2空格（YAML）
- **命名**: snake_case (变量), PascalCase (类), camelCase (函数参数)
- **注释**: 中文注释 + 英文算法说明
- **文件结尾**: 必须有空行

### 安全要求
- **输入验证**: 所有API必须包含输入验证
- **错误处理**: 使用异常处理和日志记录
- **资源管理**: RAII原则，避免内存泄漏

### 测试要求
- **覆盖率**: 关键模块需达到90%以上测试覆盖
- **边界条件**: 必须测试空值、超长输入等边界情况
- **性能测试**: 推理延迟应<100ms，并发支持10+请求

## 关键配置文件

### config.json 示例
```json
{
    "wubi_version": "86",
    "ai_enabled": true,
    "knowledge_enabled": true,
    "suggestion_delay_ms": 300,
    "candidate_page_size": 5,
    "knowledge_paths": ["C:/Users/you/Documents/notes"],
    "ai_model_path": "C:/QuickInput/models/llama-3-8b-q4.gguf"
}
```

### CMake选项
- `QI_ENABLE_AI=ON/OFF` - 启用AI模块
- `QI_BUILD_TESTS=ON/OFF` - 构建测试程序
- `QI_BUILD_INSTALLER=ON/OFF` - 构建安装包

## 快捷键定义

| 快捷键 | 功能 |
|--------|------|
| Ctrl+Shift+A | 开关AI联想 |
| Ctrl+Shift+K | 开关知识库联想 |
| Ctrl+Shift+S | 保存当前输入到知识库 |
| Ctrl+Shift+, | 打开配置面板 |

## 构建和测试流程

### 1. 单元测试
```bash
# 运行AI模块测试
./test_ai.exe

# 运行知识库测试
./test_kb.exe

# 运行上下文管理器测试
./test_context.exe
```

### 2. 集成测试
```bash
# 编译并安装完整项目
cmake --build . --target install

# 验证输入法注册
reg query "HKLM\SOFTWARE\Microsoft\CTF\SystemShared"

# 测试基本功能
# 1. 切换至QuickInput输入法
# 2. 输入五笔编码测试候选词显示
# 3. 测试AI建议功能
```

### 3. 性能基准
```bash
# 测量AI推理延迟
./test_ai.exe --benchmark

# 知识库搜索性能测试
./test_kb.exe --performance

# 内存使用情况监控
# 使用Windows任务管理器或第三方工具
```

## 故障排除

### 常见问题

#### 1. CMake配置失败
**症状**: `CMake Error at CMakeLists.txt:XX (find_package):`
**解决方案**:
```bash
# 检查Visual Studio环境
where cl

# 设置正确的SDK版本
set(CMAKE_SYSTEM_VERSION 10.0.22621.0)
```

#### 2. 输入法无法注册
**症状**: 输入法未出现在系统列表中
**解决方案**:
```powershell
# 以管理员身份运行
regsvr32 quickinput_ime.dll

# 检查注册表项
reg query "HKLM\SOFTWARE\Microsoft\CTF\SystemShared"
```

#### 3. AI模块加载失败
**症状**: `Failed to load model`错误
**解决方案**:
- 确认模型文件路径正确
- 检查文件格式支持 (.gguf, .onnx)
- 验证磁盘空间充足

## 文档更新

### 需要维护的文档
- [ ] `README.md` - 项目概述和构建指南
- [ ] `docs/CHANGELOG.md` - 版本更新日志
- [ ] `KNOWLEDGE_MODULE_COMPLETED.md` - 知识库模块完成报告
- [ ] `docs/AI_MODULE_COMPLETION.md` - AI模块完成报告

### 新文档创建
- [ ] `CLAUDE.md` - 本文件，开发者指导
- [ ] `BUILD_INSTRUCTIONS.md` - 详细构建说明
- [ ] `INTEGRATION_GUIDE.md` - 模块集成指南

## 下一步行动

### 短期目标 (本周)
1. ✅ 完成所有模块源代码编写
2. ✅ 实现完整的CMake构建系统
3. 🔄 **进行中**: 集成测试和环境验证
4. ⏳ **计划中**: 性能优化和功能完善

### 中期目标 (本月)
1. ⏳ **计划中**: 用户反馈收集和bug修复
2. ⏳ **计划中**: 多语言支持和本地化
3. ⏳ **计划中**: 云AI功能集成
4. ⏳ **计划中**: 安装包制作和分发

### 长期规划
1. ⏳ **规划中**: GPU加速推理支持
2. ⏳ **规划中**: 移动端适配
3. ⏳ **规划中**: 企业级功能扩展

## 联系方式

- **项目仓库**: https://github.com/your-repo/quickInput
- **问题反馈**: issues@quickinput.dev
- **技术讨论**: dev@quickinput.dev

---

**最后更新**: 2026年5月21日
**文档版本**: 1.0.0
**项目状态**: 模块开发完成，进入集成测试阶段