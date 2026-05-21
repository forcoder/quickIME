#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace qi::ai {

// AI建议类型
enum class AISuggestionType : uint8_t {
    ShortReply = 0,     // 短句回复（2-15字）
    WorkPhrase,         // 工作话术
    DailyExpression,    // 日常用语
    ProfessionalText,   // 专业文案
    CategoryCount
};

// AI建议条目
struct AISuggestion {
    std::wstring text;              // 建议文本
    AISuggestionType type;
    float confidence;               // 置信度 (0.0 - 1.0)
    int64_t modelId;                // 模型ID（区分不同模型版本）
    std::chrono::steady_clock::time_point timestamp;

    AISuggestion() : type(AISuggestionType::ShortReply), confidence(0.0f), modelId(0) {}
};

// 提示词模板
struct PromptTemplate {
    std::wstring systemPrompt;      // 系统角色说明
    std::wstring inputPrefix;       // 输入前缀
    std::wstring outputSuffix;      // 输出后缀
    int maxTokens;                  // 最大token数
    float temperature;              // 温度参数
};

class InferenceEngine {
public:
    InferenceEngine();
    ~InferenceEngine();

    // ── 初始化 ──
    bool Initialize(const std::wstring& modelPath);
    void Shutdown();

    // ── 模型管理 ──
    bool LoadModel(const std::wstring& modelPath);
    void UnloadModel();
    bool IsModelLoaded() const;

    // ── 推理 ──
    // 生成AI建议（最多3条，简洁干练）
    std::vector<AISuggestion> GenerateSuggestions(
        const std::wstring& context,
        const std::wstring& userInput,
        int maxSuggestions = 3);

    // 生成短句回复
    std::vector<AISuggestion> GenerateShortReplies(
        const std::wstring& conversation,
        int count = 3);

    // 生成工作话术
    std::vector<AISuggestion> GenerateWorkPhrases(
        const std::wstring& situation,
        int count = 3);

    // 生成日常用语
    std::vector<AISuggestion> GenerateDailyExpressions(
        const std::wstring& scenario,
        int count = 3);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // 提示词模板（固定格式，禁止自由发散）
    PromptTemplate GetTemplate(AISuggestionType type) const;

    // 构建完整提示
    std::wstring BuildPrompt(
        const std::wstring& context,
        const std::wstring& userInput,
        AISuggestionType type) const;

    // 调用底层模型推理
    std::string Inference(const std::wstring& prompt, int maxTokens);

    // 后处理（清理、过滤、截断）
    std::wstring PostprocessOutput(const std::wstring& rawOutput, AISuggestionType type) const;

    // 置信度评估
    float EvaluateConfidence(const std::wstring& output) const;
};
} // namespace qi::ai