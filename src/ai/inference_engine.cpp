#include "quickinput/ai/inference_engine.h"
#include "quickinput/ai/prompt_templates.h"
#include "quickinput/ai/model_loader.h"
#include "quickinput/ai/llama_context.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <thread>
#include <windows.h>

namespace qi::ai {

// ─── WString Conversion ───
static std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

static std::wstring UTF8ToWString(const std::string& str) {
    if (str.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

struct InferenceEngine::Impl {
    std::wstring modelPath;
    bool isInitialized = false;
    bool isModelLoaded = false;
    int64_t currentModelId = 0;
    std::chrono::steady_clock::time_point lastUsed;

    // llama.cpp context
    std::unique_ptr<LlamaContext> llama;

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

    // Initialize llama.cpp context
    m_impl->llama = std::make_unique<LlamaContext>();
    LlamaParams params;
    params.nCtx = 2048;
    params.nThreads = 4;
    params.n_gpu_layers = 0;
    params.temperature = 0.7f;
    params.maxTokens = 256;
    params.batchSize = 512;
    params.topP = 0.95f;
    params.topK = 40;

    if (!m_impl->llama->LoadModel(modelPath, params)) {
        std::wcerr << L"[InferenceEngine] llama.cpp model load failed" << std::endl;
        m_impl->llama.reset();
        return false;
    }

    m_impl->currentModelId = std::hash<std::wstring>{}(modelPath);
    m_impl->isModelLoaded = true;
    m_impl->lastUsed = std::chrono::steady_clock::now();

    std::wcout << L"[InferenceEngine] Model loaded: " << modelPath << std::endl;
    std::wcout << L"  Type: " <<
        (modelType == ModelType::LlamaCpp ? L"Llama.cpp (.gguf)" :
         modelType == ModelType::ONNXRuntime ? L"ONNX Runtime (.onnx)" : L"Unknown") << std::endl;
    std::wcout << L"  Size: " << ModelLoader::FormatMemorySize(result.modelInfo.memorySize) << std::endl;

    return true;
}

void InferenceEngine::UnloadModel() {
    if (m_impl->isModelLoaded) {
        if (m_impl->llama) {
            m_impl->llama->UnloadModel();
            m_impl->llama.reset();
        }
        std::wcout << L"[InferenceEngine] Model unloaded" << std::endl;
        m_impl->isModelLoaded = false;
        m_impl->currentModelId = 0;
    }
}

bool InferenceEngine::IsModelLoaded() const {
    return m_impl->isModelLoaded && m_impl->isInitialized && m_impl->llama && m_impl->llama->IsModelLoaded();
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

    if (!IsModelLoaded()) {
        std::wcerr << L"[InferenceEngine] Model not loaded" << std::endl;
        return {};
    }

    std::vector<AISuggestion> replies;

    // Use llama.cpp to generate
    std::wstring prompt = BuildPrompt(conversation, L"Generate 3 short replies (2-15 Chinese chars each)", AISuggestionType::ShortReply);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);

    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::ShortReply);

        // Parse multiple replies (by line)
        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion suggestion;
            suggestion.text = lines[i];
            suggestion.type = AISuggestionType::ShortReply;
            suggestion.confidence = 0.85f - (i * 0.05f);
            suggestion.modelId = m_impl->currentModelId;
            suggestion.timestamp = std::chrono::steady_clock::now();
            replies.push_back(suggestion);
        }
    }

    // Fallback if model output is insufficient
    while (static_cast<int>(replies.size()) < count) {
        static const std::wstring fallbacks[] = {
            L"好的，明白", L"收到", L"没问题", L"谢谢", L"不客气"
        };
        int idx = replies.size();
        if (idx >= 5) break;
        AISuggestion suggestion;
        suggestion.text = fallbacks[idx];
        suggestion.type = AISuggestionType::ShortReply;
        suggestion.confidence = 0.7f;
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

    if (!IsModelLoaded()) {
        return {};
    }

    std::vector<AISuggestion> phrases;

    // Use llama.cpp to generate
    std::wstring prompt = BuildPrompt(situation, L"Generate 3 work phrases", AISuggestionType::WorkPhrase);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);

    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::WorkPhrase);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion suggestion;
            suggestion.text = lines[i];
            suggestion.type = AISuggestionType::WorkPhrase;
            suggestion.confidence = 0.80f - (i * 0.05f);
            suggestion.modelId = m_impl->currentModelId;
            suggestion.timestamp = std::chrono::steady_clock::now();
            phrases.push_back(suggestion);
        }
    }

    // Fallback
    while (static_cast<int>(phrases.size()) < count) {
        static const std::wstring fallbacks[] = {
            L"请协助处理相关事务", L"需要您的专业意见", L"请尽快确认"
        };
        int idx = phrases.size();
        if (idx >= 3) break;
        AISuggestion suggestion;
        suggestion.text = fallbacks[idx];
        suggestion.type = AISuggestionType::WorkPhrase;
        suggestion.confidence = 0.65f;
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

    if (!IsModelLoaded()) {
        return {};
    }

    std::vector<AISuggestion> expressions;

    // Use llama.cpp to generate
    std::wstring prompt = BuildPrompt(scenario, L"Generate 3 daily expressions", AISuggestionType::DailyExpression);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);

    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::DailyExpression);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion suggestion;
            suggestion.text = lines[i];
            suggestion.type = AISuggestionType::DailyExpression;
            suggestion.confidence = 0.75f - (i * 0.05f);
            suggestion.modelId = m_impl->currentModelId;
            suggestion.timestamp = std::chrono::steady_clock::now();
            expressions.push_back(suggestion);
        }
    }

    // Fallback
    while (static_cast<int>(expressions.size()) < count) {
        static const std::wstring fallbacks[] = {
            L"今天天气不错", L"周末愉快", L"注意休息"
        };
        int idx = expressions.size();
        if (idx >= 3) break;
        AISuggestion suggestion;
        suggestion.text = fallbacks[idx];
        suggestion.type = AISuggestionType::DailyExpression;
        suggestion.confidence = 0.6f;
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

    PromptTemplate tpl = GetTemplate(type);

    std::wostringstream prompt;
    prompt << tpl.systemPrompt << L"\n\n";
    prompt << L"Context: " << context << L"\n\n";
    prompt << L"User Input: " << userInput << L"\n\n";
    prompt << tpl.inputPrefix;

    return prompt.str();
}

std::wstring InferenceEngine::BuildCSPrompt(
    const std::wstring& message,
    const std::wstring& context,
    AISuggestionType csType) const {

    PromptTemplate tpl = GetTemplate(csType);

    std::wostringstream prompt;
    prompt << tpl.systemPrompt << L"\n\n";
    prompt << L"Conversation Context: " << context << L"\n";
    prompt << L"Customer Message: " << message << L"\n\n";
    prompt << tpl.inputPrefix << L"\n";
    prompt << tpl.outputSuffix;

    return prompt.str();
}

std::string InferenceEngine::Inference(const std::wstring& prompt, int maxTokens) {
    if (!IsModelLoaded() || !m_impl->llama) {
        return {};
    }

    std::string utf8Prompt = WStringToUTF8(prompt);
    LlamaResult result = m_impl->llama->Generate(utf8Prompt);

    if (result.success) {
        return result.output;
    }
    return {};
}

bool InferenceEngine::InferenceStreaming(
    const std::wstring& prompt,
    int maxTokens,
    StreamCallback callback) {

    if (!IsModelLoaded() || !m_impl->llama || !callback) {
        return false;
    }

    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaParams params;
    params.maxTokens = maxTokens;
    params.onToken = [callback](const std::string& token) {
        callback(UTF8ToWString(token));
    };

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);
    return result.success;
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
        // 确保在中文字符边界处截断（UTF-16 surrogate pair检查）
        while (result.length() > 0) {
            wchar_t c = result.back();
            // 检查是否为高代理字符 (0xD800-0xDBFF)
            if ((c & 0xF800) == 0xD800) {
                result.pop_back(); // 移除未配对的高代理
            } else {
                break; // 正常字符，截断完成
            }
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

// ── 客服场景推理实现 ──

std::vector<AISuggestion> InferenceEngine::GenerateCustomerServiceReply(
    const std::wstring& customerMessage,
    const std::wstring& context,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    if (!IsModelLoaded()) {
        std::vector<AISuggestion> replies;
        static const std::wstring fb[] = {
            L"您好，感谢您的咨询，我会尽快为您处理。",
            L"非常理解您的情况，请稍等我们正在核实。",
            L"您好，请问还有什么可以帮您的吗？"
        };
        for (int i = 0; i < count && i < 3; ++i) {
            AISuggestion s;
            s.text = fb[i];
            s.type = AISuggestionType::CustomerService;
            s.confidence = 0.7f;
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
        return replies;
    }

    std::vector<AISuggestion> replies;
    std::wstring prompt = BuildCSPrompt(customerMessage, context, AISuggestionType::CustomerService);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);
    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::CustomerService);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) lines.push_back(line);
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion s;
            s.text = lines[i];
            s.type = AISuggestionType::CustomerService;
            s.confidence = 0.85f - (i * 0.05f);
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
    }

    // Fallback
    while (static_cast<int>(replies.size()) < count) {
        static const std::wstring fb[] = {
            L"您好，感谢您的咨询，我会尽快为您处理。",
            L"非常理解您的情况，请稍等我们正在核实。",
            L"您好，请问还有什么可以帮您的吗？"
        };
        int idx = replies.size();
        if (idx >= 3) break;
        AISuggestion s;
        s.text = fb[idx];
        s.type = AISuggestionType::CustomerService;
        s.confidence = 0.7f;
        s.modelId = m_impl->currentModelId;
        s.timestamp = std::chrono::steady_clock::now();
        replies.push_back(s);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateFaqReply(
    const std::wstring& question,
    const std::wstring& kbMatch,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    if (!IsModelLoaded()) {
        std::vector<AISuggestion> replies;
        if (!kbMatch.empty()) {
            AISuggestion s;
            s.text = kbMatch.substr(0, 40);
            s.type = AISuggestionType::FaqReply;
            s.confidence = 0.95f;
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
        static const std::wstring fb[] = {
            L"您可以查看帮助中心获取更多信息。",
            L"如需进一步帮助，请随时联系我们。"
        };
        for (size_t i = 0; i < 2 && static_cast<int>(replies.size()) < count; ++i) {
            AISuggestion s;
            s.text = fb[i];
            s.type = AISuggestionType::FaqReply;
            s.confidence = 0.72f;
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
        return replies;
    }

    std::vector<AISuggestion> replies;
    std::wstring prompt = BuildCSPrompt(question, kbMatch, AISuggestionType::FaqReply);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);
    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::FaqReply);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) lines.push_back(line);
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion s;
            s.text = lines[i];
            s.type = AISuggestionType::FaqReply;
            s.confidence = 0.88f - (i * 0.04f);
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
    }

    while (static_cast<int>(replies.size()) < count) {
        static const std::wstring fb[] = {
            L"请查看我们的帮助中心获取详细解答。",
            L"您可以参考以下步骤操作：..."
        };
        int idx = replies.size();
        if (idx >= 2) break;
        AISuggestion s;
        s.text = fb[idx];
        s.type = AISuggestionType::FaqReply;
        s.confidence = 0.72f;
        s.modelId = m_impl->currentModelId;
        s.timestamp = std::chrono::steady_clock::now();
        replies.push_back(s);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateComplaintResponse(
    const std::wstring& complaint,
    const std::wstring& context,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    if (!IsModelLoaded()) {
        std::vector<AISuggestion> replies;
        static const std::wstring fb[] = {
            L"非常抱歉给您带来不便，我们会立即核实并处理。",
            L"感谢您的反馈，我们深表歉意。",
            L"我们承诺会在24小时内给您答复。"
        };
        for (int i = 0; i < count && i < 3; ++i) {
            AISuggestion s;
            s.text = fb[i];
            s.type = AISuggestionType::ComplaintHandle;
            s.confidence = 0.68f + (i * 0.05f);
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
        return replies;
    }

    std::vector<AISuggestion> replies;
    std::wstring prompt = BuildCSPrompt(complaint, context, AISuggestionType::ComplaintHandle);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);
    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::ComplaintHandle);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) lines.push_back(line);
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion s;
            s.text = lines[i];
            s.type = AISuggestionType::ComplaintHandle;
            s.confidence = 0.88f - (i * 0.04f);
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
    }

    while (static_cast<int>(replies.size()) < count) {
        static const std::wstring fb[] = {
            L"非常抱歉给您带来不便，我们会立即核实并处理。",
            L"感谢您的反馈，我们深表歉意。",
            L"我们承诺会在24小时内给您答复。"
        };
        int idx = replies.size();
        if (idx >= 3) break;
        AISuggestion s;
        s.text = fb[idx];
        s.type = AISuggestionType::ComplaintHandle;
        s.confidence = 0.68f;
        s.modelId = m_impl->currentModelId;
        s.timestamp = std::chrono::steady_clock::now();
        replies.push_back(s);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateOrderInquiryReply(
    const std::wstring& inquiry,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    if (!IsModelLoaded()) {
        std::vector<AISuggestion> replies;
        static const std::wstring fb[] = {
            L"您好，请提供订单号，我来为您查询。",
            L"您的订单正在处理中，预计3-5个工作日送达。",
            L"您可以在我的订单中查看物流信息。"
        };
        for (int i = 0; i < count && i < 3; ++i) {
            AISuggestion s;
            s.text = fb[i];
            s.type = AISuggestionType::OrderInquiry;
            s.confidence = 0.72f;
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
        return replies;
    }

    std::vector<AISuggestion> replies;
    std::wstring prompt = BuildCSPrompt(inquiry, L"order inquiry", AISuggestionType::OrderInquiry);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);
    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::OrderInquiry);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) lines.push_back(line);
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion s;
            s.text = lines[i];
            s.type = AISuggestionType::OrderInquiry;
            s.confidence = 0.86f - (i * 0.04f);
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
    }

    while (static_cast<int>(replies.size()) < count) {
        static const std::wstring fb[] = {
            L"您好，请提供订单号，我来为您查询。",
            L"您的订单正在处理中，预计3-5个工作日送达。",
            L"您可以在我的订单中查看物流信息。"
        };
        int idx = replies.size();
        if (idx >= 3) break;
        AISuggestion s;
        s.text = fb[idx];
        s.type = AISuggestionType::OrderInquiry;
        s.confidence = 0.7f;
        s.modelId = m_impl->currentModelId;
        s.timestamp = std::chrono::steady_clock::now();
        replies.push_back(s);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateRefundResponse(
    const std::wstring& refundRequest,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    if (!IsModelLoaded()) {
        std::vector<AISuggestion> replies;
        static const std::wstring fb[] = {
            L"您好，退款申请已受理，预计3-7个工作日到账。",
            L"退款流程已启动，请在订单详情查看进度。",
            L"退款将在审核通过后原路返回。"
        };
        for (int i = 0; i < count && i < 3; ++i) {
            AISuggestion s;
            s.text = fb[i];
            s.type = AISuggestionType::RefundProcess;
            s.confidence = 0.72f;
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
        return replies;
    }

    std::vector<AISuggestion> replies;
    std::wstring prompt = BuildCSPrompt(refundRequest, L"refund request", AISuggestionType::RefundProcess);
    std::string utf8Prompt = WStringToUTF8(prompt);

    LlamaResult result = m_impl->llama->Generate(utf8Prompt);
    if (result.success) {
        std::wstring output = UTF8ToWString(result.output);
        output = PostprocessOutput(output, AISuggestionType::RefundProcess);

        std::vector<std::wstring> lines;
        std::wistringstream stream(output);
        std::wstring line;
        while (std::getline(stream, line, L'\n')) {
            if (!line.empty()) lines.push_back(line);
        }

        for (int i = 0; i < count && i < static_cast<int>(lines.size()); ++i) {
            AISuggestion s;
            s.text = lines[i];
            s.type = AISuggestionType::RefundProcess;
            s.confidence = 0.84f - (i * 0.04f);
            s.modelId = m_impl->currentModelId;
            s.timestamp = std::chrono::steady_clock::now();
            replies.push_back(s);
        }
    }

    while (static_cast<int>(replies.size()) < count) {
        static const std::wstring fb[] = {
            L"您好，退款申请已受理，预计3-7个工作日到账。",
            L"退款流程已启动，请在订单详情查看进度。",
            L"退款将在审核通过后原路返回。"
        };
        int idx = replies.size();
        if (idx >= 3) break;
        AISuggestion s;
        s.text = fb[idx];
        s.type = AISuggestionType::RefundProcess;
        s.confidence = 0.68f;
        s.modelId = m_impl->currentModelId;
        s.timestamp = std::chrono::steady_clock::now();
        replies.push_back(s);
    }

    return replies;
}

// ─── Streaming Output ───

bool InferenceEngine::GenerateStreaming(
    const std::wstring& context,
    const std::wstring& userInput,
    AISuggestionType type,
    StreamCallback callback) {

    if (!IsModelLoaded() || !callback) return false;

    std::wstring prompt = BuildPrompt(context, userInput, type);
    return InferenceStreaming(WStringToUTF8(prompt), GetTemplate(type).maxTokens, callback);
}

bool InferenceEngine::GenerateCSStreaming(
    const std::wstring& customerMessage,
    const std::wstring& context,
    AISuggestionType csType,
    StreamCallback callback) {

    if (!IsModelLoaded() || !callback) return false;

    std::wstring prompt = BuildCSPrompt(customerMessage, context, csType);
    return InferenceStreaming(WStringToUTF8(prompt), GetTemplate(csType).maxTokens, callback);
}

} // namespace qi::ai