# AI模块构建指南

## 快速开始

### 1. 编译AI模块

```bash
cd D:/workspace/cpp/quickInput
mkdir build && cd build

# 配置项目（启用AI功能）
cmake .. -DQI_ENABLE_AI=ON -DQI_BUILD_TESTS=ON

# 编译项目
cmake --build .
```

### 2. 运行测试

```bash
# 运行AI模块测试
./test_ai.exe

# 或者从源码目录运行
../src/ai/test_ai.exe
```

### 3. 使用AI功能

在主项目中包含头文件并使用API：

```cpp
#include "quickinput/ai/inference_engine.h"

// 创建引擎实例
qi::ai::InferenceEngine engine;

// 初始化（需要模型文件）
if (engine.Initialize(L"path/to/model.gguf")) {
    // 生成建议
    auto suggestions = engine.GenerateSuggestions(
        L"上下文", L"用户输入", 3);

    // 处理结果...
}

// 清理资源
engine.Shutdown();
```

## 配置选项

### CMake配置参数

- `QI_ENABLE_AI=ON/OFF` - 启用/禁用AI模块（默认: ON）
- `QI_BUILD_TESTS=ON/OFF` - 构建测试程序（默认: ON）
- `QI_BUILD_INSTALLER=ON/OFF` - 构建安装程序（默认: ON）
- `QI_ENABLE_CLOUD=ON/OFF` - 启用云端LLM API（默认: OFF）

### 示例配置

```bash
# 仅编译主IME模块，不包含AI功能
cmake .. -DQI_ENABLE_AI=OFF

# 完整编译（包含AI模块和所有测试）
cmake .. -DQI_ENABLE_AI=ON -DQI_BUILD_TESTS=ON -DQI_BUILD_INSTALLER=ON

# 开发调试配置
cmake .. -DQI_ENABLE_AI=ON -DQI_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
```

## 依赖项

### 必需依赖
- C++20编译器
- Windows SDK (Windows平台)
- CMake 3.16+

### 可选依赖
- llama-cpp库 - 用于实际模型推理（自动检测）
- ONNX Runtime - 支持ONNX格式模型

### 第三方库配置

如果需要使用实际的llama.cpp模型推理，请确保：
1. 安装llama-cpp库
2. 设置PKG_CONFIG_PATH环境变量
3. 或手动指定库路径

## 测试说明

### test_ai 可执行程序

包含以下测试用例：

1. **模型加载器测试**
   - 检测支持的模型格式
   - 验证模型文件
   - 获取模型信息

2. **提示词模板测试**
   - 四种AI类型模板验证
   - 模板格式检查
   - 类型描述获取

3. **推理引擎测试**
   - 初始化流程
   - AI建议生成
   - 不同类型生成测试
   - 置信度评估

4. **性能测试**
   - 推理延迟测量
   - 并发性能测试
   - 内存使用情况

### 预期输出示例

```
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

## 故障排除

### 常见问题

1. **CMake找不到AI模块**
   ```bash
   # 确保源文件存在
   ls src/ai/*.cpp
   ls include/quickinput/ai/*.h
   ```

2. **链接错误**
   ```bash
   # 检查CMake配置
   cmake .. -DQI_ENABLE_AI=ON
   ```

3. **测试程序运行失败**
   ```bash
   # 重新编译并查看详细日志
   cmake --build . --verbose
   ```

### 调试技巧

- 使用`-DCMAKE_BUILD_TYPE=Debug`进行调试编译
- 添加`--verbose`参数查看详细编译过程
- 检查CMake缓存：`rm CMakeCache.txt && rm -rf CMakeFiles`

## 性能优化

### 编译优化

```bash
# 并行编译（提高速度）
cmake --build . --parallel 8

# 增量编译
cmake --build . --target quickinput_ai
```

### 运行时优化

- 预加载模型到内存
- 使用多线程处理并发请求
- 实现模型缓存机制

## 扩展开发

### 添加新的AI类型

1. 在`AISuggestionType`枚举中添加新类型
2. 在`PromptTemplateManager::s_templates`中添加对应模板
3. 实现相应的生成方法

### 集成实际模型

1. 替换`InferenceEngine::Inference()`中的模拟逻辑
2. 添加对llama.cpp或ONNX Runtime的实际调用
3. 实现异步推理接口

## 参考链接

- [QuickInput项目文档](../README.md)
- [模块5详细设计](../docs/Module5_AI_Inference.md)
- [更新日志](../docs/CHANGELOG.md)