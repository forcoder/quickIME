#include "quickinput/ai/inference_engine.h"
#include "quickinput/ai/prompt_templates.h"
#include "quickinput/ai/model_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <thread>

namespace qi::ai {

struct InferenceEngine::Impl {
    std::wstring modelPath;
    bool isInitialized = false;
    bool isModelLoaded = false;
    int64_t currentModelId = 0;
    std::chrono::steady_clock::time_point lastUsed;

    Impl() : lastUsed(std::chrono::steady_clock::now()) {}
};

InferenceEngine::InferenceEngine() : m_impl(std::make_unique<Impl>()) {}

InferenceEngine::~InferenceEngine() {
    Shutdown();
}

bool InferenceEngine::Initialize(const std::wstring& modelPath) {
    if (m_impl->isInitialized) {
        return true;
    }

    // 验证模型文件
    if (!ModelLoader::ValidateModelFile(modelPath)) {
        std::wcerr << L"模型文件不存在或不可读: " << modelPath << std::endl;
        return false;
    }

    // 加载模型
    if (!LoadModel(modelPath)) {
        return false;
    }

    m_impl->isInitialized = true;
    m_impl->modelPath = modelPath;
    return true;
}

void InferenceEngine::Shutdown() {
    UnloadModel();
    m_impl->isInitialized = false;
}

bool InferenceEngine::LoadModel(const std::wstring& modelPath) {
    if (m_impl->isModelLoaded) {
        UnloadModel();
    }

    // 检测模型类型
    ModelType modelType = ModelLoader::DetectModelType(modelPath);
    if (modelType == ModelType::Unknown) {
        std::wcerr << L"不支持的模型格式: " << modelPath << std::endl;
        return false;
    }

    // 获取模型信息
    LoadResult result = ModelLoader::GetModelInfo(modelPath);
    if (!result.success) {
        std::wcerr << L"加载模型失败: " << result.error << std::endl;
        return false;
    }

    // 模拟模型加载过程
    m_impl->currentModelId = std::hash<std::wstring>{}(modelPath);
    m_impl->isModelLoaded = true;
    m_impl->lastUsed = std::chrono::steady_clock::now();

    std::wcout << L"模型加载成功: " << modelPath << std::endl;
    std::wcout << L"类型: " <<
        (modelType == ModelType::LlamaCpp ? L"Llama.cpp (.gguf)" :
         modelType == ModelType::ONNXRuntime ? L"ONNX Runtime (.onnx)" : L"未知") << std::endl;
    std::wcout << L"大小: " << ModelLoader::FormatMemorySize(result.modelInfo.memorySize) << std::endl;

    return true;
}

void InferenceEngine::UnloadModel() {
    if (m_impl->isModelLoaded) {
        std::wcout << L"模型已卸载" << std::endl;
        m_impl->isModelLoaded = false;
        m_impl->currentModelId = 0;
    }
}

bool InferenceEngine::IsModelLoaded() const {
    return m_impl->isModelLoaded && m_impl->isInitialized;
}

std::vector<AISuggestion> InferenceEngine::GenerateSuggestions(
    const std::wstring& context,
    const std::wstring& userInput,
    int maxSuggestions) {

    if (!IsModelLoaded()) {
        std::wcerr << L"错误：未加载模型" << std::endl;
        return {};
    }

    if (userInput.empty()) {
        std::wcerr << L"错误：用户输入不能为空" << std::endl;
        return {};
    }

    if (maxSuggestions <= 0 || maxSuggestions > 3) {
        maxSuggestions = 3; // 默认值
    }

    std::vector<AISuggestion> suggestions;

    // 生成不同类型的建议
    for (int i = 0; i < static_cast<int>(AISuggestionType::CategoryCount); ++i) {
        AISuggestionType type = static_cast<AISuggestionType>(i);

        // 根据类型生成相应建议
        switch (type) {
            case AISuggestionType::ShortReply:
                suggestions.push_back(GenerateShortReply(context, userInput));
                break;
            case AISuggestionType::WorkPhrase:
                suggestions.push_back(GenerateWorkPhrase(context, userInput));
                break;
            case AISuggestionType::DailyExpression:
                suggestions.push_back(GenerateDailyExpression(context, userInput));
                break;
            case AISuggestionType::ProfessionalText:
                suggestions.push_back(GenerateProfessionalText(context, userInput));
                break;
            default:
                break;
        }

        // 限制数量
        if (suggestions.size() >= static_cast<size_t>(maxSuggestions)) {
            break;
        }
    }

    // 按置信度排序
    std::sort(suggestions.begin(), suggestions.end(),
        [](const AISuggestion& a, const AISuggestion& b) {
            return a.confidence > b.confidence;
        });

    // 截取前maxSuggestions条
    if (suggestions.size() > static_cast<size_t>(maxSuggestions)) {
        suggestions.resize(maxSuggestions);
    }

    return suggestions;
}

std::vector<AISuggestion> InferenceEngine::GenerateShortReplies(
    const std::wstring& conversation,
    int count) {

    if (count <= 0 || count > 3) {
        count = 3;
    }

    std::vector<AISuggestion> replies;

    // 模拟生成短句回复
    std::vector<std::wstring> mockReplies = {
        L"好的，明白", L"收到", L"没问题",
        L"谢谢", L"不客气", L"请说",
        L"稍等", L"马上到", L"知道了",
        L"可以", L"不行", L"行吧"
    };

    for (int i = 0; i < count && i < static_cast<int>(mockReplies.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = mockReplies[i];
        suggestion.type = AISuggestionType::ShortReply;
        suggestion.confidence = 0.8f + (i * 0.05f); // 模拟置信度
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();

        replies.push_back(suggestion);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateWorkPhrases(
    const std::wstring& situation,
    int count) {

    if (count <= 0 || count > 3) {
        count = 3;
    }

    std::vector<AISuggestion> phrases;

    // 模拟生成工作话术
    std::vector<std::wstring> mockPhrases = {
        L"请协助处理相关事务", L"需要您的专业意见", L"请尽快确认",
        L"此事项需优先处理", L"请提供详细报告", L"需要进一步讨论",
        L"请安排时间会议", L"需要上级审批", L"请跟进进度",
        L"此方案可行", L"需要修改完善", L"请评估风险"
    };

    for (int i = 0; i < count && i < static_cast<int>(mockPhrases.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = mockPhrases[i];
        suggestion.type = AISuggestionType::WorkPhrase;
        suggestion.confidence = 0.7f + (i * 0.08f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();

        phrases.push_back(suggestion);
    }

    return phrases;
}

std::vector<AISuggestion> InferenceEngine::GenerateDailyExpressions(
    const std::wstring& scenario,
    int count) {

    if (count <= 0 || count > 3) {
        count = 3;
    }

    std::vector<AISuggestion> expressions;

    // 模拟生成日常用语
    std::vector<std::wstring> mockExpressions = {
        L"今天天气不错", L"周末愉快", L"注意休息",
        L"路上小心", L"好好休息", L"保重身体",
        L"工作顺利", L"生活愉快", L"身体健康",
        L"天天开心", L"万事如意", L"心想事成"
    };

    for (int i = 0; i < count && i < static_cast<int>(mockExpressions.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = mockExpressions[i];
        suggestion.type = AISuggestionType::DailyExpression;
        suggestion.confidence = 0.6f + (i * 0.1f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();

        expressions.push_back(suggestion);
    }

    return expressions;
}

PromptTemplate InferenceEngine::GetTemplate(AISuggestionType type) const {
    return PromptTemplateManager::GetTemplate(type);
}

std::wstring InferenceEngine::BuildPrompt(
    const std::wstring& context,
    const std::wstring& userInput,
    AISuggestionType type) const {

    PromptTemplate template_ = GetTemplate(type);

    std::wostringstream prompt;
    prompt << template_.systemPrompt << L"\n\n";
    prompt << L"上下文: " << context << L"\n\n";
    prompt << L"用户输入: " << userInput << L"\n\n";
    prompt << template_.inputPrefix;

    return prompt.str();
}

std::string InferenceEngine::Inference(const std::wstring& prompt, int maxTokens) {
    // 模拟推理过程 - 在实际实现中这里会调用底层AI模型
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 模拟输出结果
    std::string mockOutput = "模拟AI输出结果";
    return mockOutput;
}

std::wstring InferenceEngine::PostprocessOutput(const std::wstring& rawOutput, AISuggestionType type) const {
    std::wstring result = rawOutput;

    // 清理输出：移除多余空格、换行符等
    result.erase(std::remove_if(result.begin(), result.end(),
        [](wchar_t c) { return std::isspace(c); }), result.end());

    // 根据类型截断文本长度（2-15字）
    size_t maxLength = 15;
    if (result.length() > maxLength) {
        result = result.substr(0, maxLength);
        // 确保在中文字符边界处截断
        while (result.length() > 0 && (result.back() & 0xFF80) == 0) {
            result.pop_back();
        }
    }

    // 如果太短则补充
    if (result.length() < 2) {
        result = L"好的";
    }

    return result;
}

float InferenceEngine::EvaluateConfidence(const std::wstring& output) const {
    // 简单的置信度评估算法
    float confidence = 0.5f; // 基础置信度

    // 根据输出长度调整置信度
    if (output.length() >= 2 && output.length() <= 15) {
        confidence += 0.3f; // 长度合适的加分
    }

    // 随机因子（实际实现中会根据模型质量评估）
    confidence += (rand() % 100) / 1000.0f;

    return std::min(confidence, 1.0f);
}

// 辅助函数实现
AISuggestion InferenceEngine::GenerateShortReply(const std::wstring& context, const std::wstring& userInput) {
    AISuggestion suggestion;
    suggestion.text = PostprocessOutput(L"好的", AISuggestionType::ShortReply);
    suggestion.type = AISuggestionType::ShortReply;
    suggestion.confidence = EvaluateConfidence(L"好的");
    suggestion.modelId = m_impl->currentModelId;
    suggestion.timestamp = std::chrono::steady_clock::now();
    return suggestion;
}

AISuggestion InferenceEngine::GenerateWorkPhrase(const std::wstring& context, const std::wstring& userInput) {
    AISuggestion suggestion;
    suggestion.text = PostprocessOutput(L"请协助处理", AISuggestionType::WorkPhrase);
    suggestion.type = AISuggestionType::WorkPhrase;
    suggestion.confidence = EvaluateConfidence(L"请协助处理");
    suggestion.modelId = m_impl->currentModelId;
    suggestion.timestamp = std::chrono::steady_clock::now();
    return suggestion;
}

AISuggestion InferenceEngine::GenerateDailyExpression(const std::wstring& context, const std::wstring& userInput) {
    AISuggestion suggestion;
    suggestion.text = PostprocessOutput(L"注意休息", AISuggestionType::DailyExpression);
    suggestion.type = AISuggestionType::DailyExpression;
    suggestion.confidence = EvaluateConfidence(L"注意休息");
    suggestion.modelId = m_impl->currentModelId;
    suggestion.timestamp = std::chrono::steady_clock::now();
    return suggestion;
}

AISuggestion InferenceEngine::GenerateProfessionalText(const std::wstring& context, const std::wstring& userInput) {
    AISuggestion suggestion;
    suggestion.text = PostprocessOutput(L"专业分析完成", AISuggestionType::ProfessionalText);
    suggestion.type = AISuggestionType::ProfessionalText;
    suggestion.confidence = EvaluateConfidence(L"专业分析完成");
    suggestion.modelId = m_impl->currentModelId;
    suggestion.timestamp = std::chrono::steady_clock::now();
    return suggestion;
}

} // namespace qi::ai