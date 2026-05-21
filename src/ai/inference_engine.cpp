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

// ── 客服场景推理实现 ──

std::vector<AISuggestion> InferenceEngine::GenerateCustomerServiceReply(
    const std::wstring& customerMessage,
    const std::wstring& context,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    std::vector<AISuggestion> replies;

    // 客服回复模板库
    static const std::vector<std::wstring> csTemplates = {
        L"您好，感谢您的咨询，我来为您查询一下。",
        L"非常理解您的情况，我们会尽快为您处理。",
        L"您好，关于您的问题，建议您尝试以下方法：",
        L"感谢您的反馈，我们会认真对待您的意见。",
        L"您好，我已经记录了您的问题，会马上为您核实。",
        L"非常抱歉给您带来不便，我们正在积极处理中。",
        L"您好，根据您描述的情况，建议您：",
        L"感谢您的耐心等候，您的诉求我们已经收到。",
        L"您好，这个问题我来帮您解答：",
        L"理解您的心情，我们会竭尽全力为您解决问题。"
    };

    for (int i = 0; i < count && i < static_cast<int>(csTemplates.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = csTemplates[i];
        suggestion.type = AISuggestionType::CustomerService;
        suggestion.confidence = 0.85f - (i * 0.05f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();
        replies.push_back(suggestion);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateFaqReply(
    const std::wstring& question,
    const std::wstring& kbMatch,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    std::vector<AISuggestion> replies;

    // 基于知识库匹配生成回复
    if (!kbMatch.empty()) {
        AISuggestion suggestion;
        suggestion.text = kbMatch.substr(0, 40); // 截断到40字
        suggestion.type = AISuggestionType::FaqReply;
        suggestion.confidence = 0.95f;
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();
        replies.push_back(suggestion);
    }

    // 补充通用FAQ回复
    static const std::vector<std::wstring> faqReplies = {
        L"您可以查看帮助中心获取更多信息。",
        L"如需进一步帮助，请随时联系我们。",
        L"建议您参考产品使用说明进行操作。",
        L"您可以拨打客服热线获取更详细的指导。"
    };

    for (int i = 0; i < count - static_cast<int>(replies.size()) && i < static_cast<int>(faqReplies.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = faqReplies[i];
        suggestion.type = AISuggestionType::FaqReply;
        suggestion.confidence = 0.75f - (i * 0.05f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();
        replies.push_back(suggestion);
    }

    if (replies.size() > static_cast<size_t>(count)) {
        replies.resize(count);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateComplaintResponse(
    const std::wstring& complaint,
    const std::wstring& context,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    std::vector<AISuggestion> replies;

    static const std::vector<std::wstring> complaintReplies = {
        L"非常抱歉给您带来不好的体验，我们会认真对待您的投诉并尽快处理。",
        L"感谢您的反馈，我们对此深表歉意，会立即核实情况并改进。",
        L"理解您的不满，我们承诺会在24小时内给您一个满意的答复。"
    };

    for (int i = 0; i < count && i < static_cast<int>(complaintReplies.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = complaintReplies[i];
        suggestion.type = AISuggestionType::ComplaintHandle;
        suggestion.confidence = 0.88f - (i * 0.04f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();
        replies.push_back(suggestion);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateOrderInquiryReply(
    const std::wstring& inquiry,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    std::vector<AISuggestion> replies;

    static const std::vector<std::wstring> orderReplies = {
        L"您好，请提供订单号，我来为您查询订单状态。",
        L"您好，您可以在'我的订单'中查看最新的物流信息。",
        L"您好，您的订单正在处理中，预计3-5个工作日内发货。"
    };

    for (int i = 0; i < count && i < static_cast<int>(orderReplies.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = orderReplies[i];
        suggestion.type = AISuggestionType::OrderInquiry;
        suggestion.confidence = 0.82f - (i * 0.05f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();
        replies.push_back(suggestion);
    }

    return replies;
}

std::vector<AISuggestion> InferenceEngine::GenerateRefundResponse(
    const std::wstring& refundRequest,
    int count) {

    if (count <= 0 || count > 3) count = 3;

    std::vector<AISuggestion> replies;

    static const std::vector<std::wstring> refundReplies = {
        L"您好，退款申请已受理，预计3-7个工作日到账，请耐心等待。",
        L"您好，退款流程已启动，您可以在订单详情中查看退款进度。",
        L"您好，非常抱歉给您带来不便，退款将在审核通过后原路返回。"
    };

    for (int i = 0; i < count && i < static_cast<int>(refundReplies.size()); ++i) {
        AISuggestion suggestion;
        suggestion.text = refundReplies[i];
        suggestion.type = AISuggestionType::RefundProcess;
        suggestion.confidence = 0.86f - (i * 0.04f);
        suggestion.modelId = m_impl->currentModelId;
        suggestion.timestamp = std::chrono::steady_clock::now();
        replies.push_back(suggestion);
    }

    return replies;
}

} // namespace qi::ai