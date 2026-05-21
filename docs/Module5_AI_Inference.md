# 模块5: 离线AI推理接口

## 概述

模块5实现了QuickInput五笔输入法的离线AI推理接口，为用户提供智能文本生成功能。该模块支持多种类型的AI建议生成，包括短句回复、工作话术、日常用语和专业文案。

## 主要特性

- **多模型支持**: 支持llama.cpp (.gguf) 和 ONNX Runtime (.onnx) 格式
- **多种AI类型**: 短句回复、工作话术、日常用语、专业文案
- **固定提示词模板**: 确保输出符合要求（2-15字，简洁干练）
- **置信度评估**: 为每个AI建议提供置信度评分
- **线程安全**: 所有公共接口都是线程安全的
- **错误处理**: 完善的错误处理和日志记录

## 文件结构

```
src/ai/
├── inference_engine.h      # AI推理引擎核心类定义
├── inference_engine.cpp    # AI推理引擎实现
├── prompt_templates.h      # 提示词模板管理器定义
├── prompt_templates.cpp    # 提示词模板管理器实现
├── model_loader.h          # 模型加载器定义
├── model_loader.cpp        # 模型加载器实现
└── test_ai.cpp            # 测试程序

include/quickinput/ai/
├── inference_engine.h      # AI推理引擎头文件
├── prompt_templates.h      # 提示词模板管理器头文件
└── model_loader.h          # 模型加载器头文件
```

## API参考

### InferenceEngine 类

#### 构造函数/析构函数
```cpp
InferenceEngine();
~InferenceEngine();
```

#### 初始化方法
```cpp
bool Initialize(const std::wstring& modelPath);
void Shutdown();
```

#### 模型管理方法
```cpp
bool LoadModel(const std::wstring& modelPath);
void UnloadModel();
bool IsModelLoaded() const;
```

#### 推理方法
```cpp
std::vector<AISuggestion> GenerateSuggestions(
    const std::wstring& context,
    const std::wstring& userInput,
    int maxSuggestions = 3);

std::vector<AISuggestion> GenerateShortReplies(
    const std::wstring& conversation,
    int count = 3);

std::vector<AISuggestion> GenerateWorkPhrases(
    const std::wstring& situation,
    int count = 3);

std::vector<AISuggestion> GenerateDailyExpressions(
    const std::wstring& scenario,
    int count = 3);
```

### AISuggestion 结构体
```cpp
struct AISuggestion {
    std::wstring text;              // 建议文本（2-15字）
    AISuggestionType type;          // 建议类型
    float confidence;               // 置信度 (0.0 - 1.0)
    int64_t modelId;                // 模型ID
    std::chrono::steady_clock::time_point timestamp;
};
```

### AISuggestionType 枚举
```cpp
enum class AISuggestionType : uint8_t {
    ShortReply = 0,     // 短句回复
    WorkPhrase,         // 工作话术
    DailyExpression,    // 日常用语
    ProfessionalText,   // 专业文案
    CategoryCount       // 类别数量
};
```

## 使用示例

```cpp
#include "quickinput/ai/inference_engine.h"

// 创建推理引擎实例
qi::ai::InferenceEngine engine;

// 初始化（需要指定模型路径）
if (!engine.Initialize(L"C:\\models\\my_model.gguf")) {
    // 处理错误
}

// 生成AI建议
std::wstring context = L"用户正在与客户沟通";
std::wstring userInput = L"请帮我生成合适的回复";

auto suggestions = engine.GenerateSuggestions(context, userInput, 3);

// 处理结果
for (const auto& suggestion : suggestions) {
    std::wcout << L"建议: " << suggestion.text
              << L" (置信度: " << suggestion.confidence << L")"
              << std::endl;
}

// 清理资源
engine.Shutdown();
```

## 配置要求

### CMake配置
需要在CMakeLists.txt中启用AI功能：
```cmake
option(QI_ENABLE_AI "Enable AI inference" ON)
```

### 依赖库
- C++20标准
- Windows SDK（Windows平台）
- （可选）llama-cpp 库用于实际模型推理

## 性能考虑

- **模型加载**: 首次加载较慢，建议在应用启动时完成
- **推理延迟**: 模拟实现约100ms，实际实现取决于模型大小和硬件
- **内存使用**: 根据模型大小动态分配，建议监控内存使用

## 错误处理

所有API都包含输入验证和错误检查：
- 空输入检测
- 模型状态检查
- 文件格式验证
- 内存访问保护

## 测试

提供了完整的测试程序 `test_ai.cpp`，包含：
- 模型加载器测试
- 提示词模板测试
- 推理引擎功能测试
- 性能测试

编译并运行测试：
```bash
mkdir build && cd build
cmake .. -DQI_ENABLE_AI=ON -DQI_BUILD_TESTS=ON
cmake --build .
./test_ai
```

## 扩展性

该模块设计具有良好的扩展性：
- **新模型类型**: 通过继承ModelLoader基类添加
- **新AI类型**: 在AISuggestionType枚举中添加，并在PromptTemplateManager中配置模板
- **自定义提示词**: 通过PromptTemplateManager配置
- **异步推理**: 可在Impl结构体中添加异步任务队列

## 安全性

- 所有字符串操作都有边界检查
- 模型路径验证防止目录遍历攻击
- 输入长度限制防止缓冲区溢出
- 敏感操作记录日志