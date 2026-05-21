#include "quickinput/cs/cs_manager.h"
#include <algorithm>
#include <sstream>

namespace qi {
namespace cs {

CustomerServiceManager& CustomerServiceManager::GetInstance() {
    static CustomerServiceManager instance;
    return instance;
}

CustomerServiceManager::CustomerServiceManager() = default;
CustomerServiceManager::~CustomerServiceManager() {
    Shutdown();
}

bool CustomerServiceManager::Initialize(
    const std::wstring& modelPath,
    const std::wstring& kbPath) {

    std::lock_guard<std::mutex> lock(m_mutex);

    // 初始化 AI 引擎
    if (m_config.aiEnabled) {
        m_aiEngine = std::make_unique<qi::ai::InferenceEngine>();
        if (!modelPath.empty()) {
            m_aiEngine->Initialize(modelPath);
        }
    }

    // 初始化知识库
    if (m_config.kbEnabled) {
        m_kb = std::make_unique<qi::knowledge::KnowledgeBase>();
        if (!kbPath.empty()) {
            m_kb->Initialize(kbPath);
        }
    }

    m_isReady = true;
    return true;
}

void CustomerServiceManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_aiEngine) {
        m_aiEngine->Shutdown();
        m_aiEngine.reset();
    }

    if (m_kb) {
        m_kb->Shutdown();
        m_kb.reset();
    }

    m_isReady = false;
}

ContextManager::DialogMode CustomerServiceManager::DetectScene(const std::wstring& message) {
    return ContextManager::GetInstance().DetectDialogMode(message);
}

std::vector<CSSuggestion> CustomerServiceManager::GenerateSuggestions(
    const std::wstring& customerMessage,
    int maxSuggestions) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isReady) return {};

    // 1. 识别场景类型
    auto dialogMode = DetectScene(customerMessage);

    // 2. 映射场景到知识库分类
    KbCategory kbFilter = KbCategory::General;
    AISuggestionType aiType = AISuggestionType::CustomerService;

    switch (dialogMode) {
        case ContextManager::DialogMode::Complaint:
            kbFilter = KbCategory::ComplaintHandle;
            aiType = AISuggestionType::ComplaintHandle;
            break;
        case ContextManager::DialogMode::OrderQuery:
            kbFilter = KbCategory::OrderInquiry;
            aiType = AISuggestionType::OrderInquiry;
            break;
        case ContextManager::DialogMode::Refund:
            kbFilter = KbCategory::RefundProcess;
            aiType = AISuggestionType::RefundProcess;
            break;
        case ContextManager::DialogMode::Inquiry:
            kbFilter = KbCategory::FaqReply;
            aiType = AISuggestionType::FaqReply;
            break;
        default:
            kbFilter = KbCategory::General;
            aiType = AISuggestionType::CustomerService;
            break;
    }

    // 3. 并行执行知识库检索和AI推理
    std::vector<KbResult> kbResults;
    std::vector<AISuggestion> aiResults;

    if (m_config.kbEnabled && m_kb) {
        kbResults = m_kb->HybridSearch(customerMessage, maxSuggestions * 2, kbFilter);
    }

    if (m_config.aiEnabled && m_aiEngine && m_aiEngine->IsModelLoaded()) {
        // 获取对话上下文
        auto& ctxMgr = ContextManager::GetInstance();
        std::wstring context = ctxMgr.ExtractConversationContext(300);

        switch (aiType) {
            case AISuggestionType::ComplaintHandle:
                aiResults = m_aiEngine->GenerateComplaintResponse(customerMessage, context, 3);
                break;
            case AISuggestionType::OrderInquiry:
                aiResults = m_aiEngine->GenerateOrderInquiryReply(customerMessage, 3);
                break;
            case AISuggestionType::RefundProcess:
                aiResults = m_aiEngine->GenerateRefundResponse(customerMessage, 3);
                break;
            case AISuggestionType::FaqReply:
                // FAQ 模式：使用知识库匹配 + AI 生成
                {
                    std::wstring kbMatch;
                    if (!kbResults.empty()) {
                        kbMatch = kbResults[0].entry.content;
                    }
                    aiResults = m_aiEngine->GenerateFaqReply(customerMessage, kbMatch, 3);
                }
                break;
            default:
                aiResults = m_aiEngine->GenerateCustomerServiceReply(customerMessage, context, 3);
                break;
        }
    }

    // 4. 合并结果
    return MergeSuggestions(kbResults, aiResults, maxSuggestions);
}

std::vector<CSSuggestion> CustomerServiceManager::MergeSuggestions(
    const std::vector<KbResult>& kbResults,
    const std::vector<AISuggestion>& aiResults,
    int maxCount) {

    std::vector<CSSuggestion> merged;

    // 添加知识库结果
    for (const auto& r : kbResults) {
        if (merged.size() >= static_cast<size_t>(maxCount)) break;

        CSSuggestion s;
        s.text = r.entry.content;
        s.source = SuggestionSource::KnowledgeBase;
        s.confidence = r.similarity * m_config.kbWeight;
        s.kbCategory = r.entry.category;
        merged.push_back(s);
    }

    // 添加AI结果
    for (const auto& r : aiResults) {
        if (merged.size() >= static_cast<size_t>(maxCount)) break;

        CSSuggestion s;
        s.text = r.text;
        s.source = SuggestionSource::AIGenerated;
        s.confidence = r.confidence * m_config.aiWeight;
        s.aiType = r.type;
        merged.push_back(s);
    }

    // 按置信度排序
    std::sort(merged.begin(), merged.end(),
        [](const CSSuggestion& a, const CSSuggestion& b) {
            return a.confidence > b.confidence;
        });

    // 过滤低置信度
    float minConf = m_config.minConfidence;
    merged.erase(
        std::remove_if(merged.begin(), merged.end(),
            [minConf](const CSSuggestion& s) { return s.confidence < minConf; }),
        merged.end());

    // 截断
    if (merged.size() > static_cast<size_t>(maxCount)) {
        merged.resize(maxCount);
    }

    return merged;
}

std::vector<KbResult> CustomerServiceManager::SearchKnowledgeBase(
    const std::wstring& query,
    int maxResults,
    KbCategory filter) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_kb) return {};
    return m_kb->HybridSearch(query, maxResults, filter);
}

std::vector<AISuggestion> CustomerServiceManager::GenerateAIResponse(
    const std::wstring& message,
    AISuggestionType type,
    int count) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_aiEngine || !m_aiEngine->IsModelLoaded()) return {};

    auto& ctxMgr = ContextManager::GetInstance();
    std::wstring context = ctxMgr.ExtractConversationContext(300);

    switch (type) {
        case AISuggestionType::CustomerService:
            return m_aiEngine->GenerateCustomerServiceReply(message, context, count);
        case AISuggestionType::FaqReply:
            return m_aiEngine->GenerateFaqReply(message, L"", count);
        case AISuggestionType::ComplaintHandle:
            return m_aiEngine->GenerateComplaintResponse(message, context, count);
        case AISuggestionType::OrderInquiry:
            return m_aiEngine->GenerateOrderInquiryReply(message, count);
        case AISuggestionType::RefundProcess:
            return m_aiEngine->GenerateRefundResponse(message, count);
        default:
            return m_aiEngine->GenerateCustomerServiceReply(message, context, count);
    }
}

bool CustomerServiceManager::ImportKbFile(const std::wstring& filePath, KbCategory category) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_kb) return false;
    return m_kb->ImportFile(filePath, category);
}

bool CustomerServiceManager::ImportKbText(const std::wstring& text, KbCategory category) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_kb) return false;
    return m_kb->ImportText(text, category);
}

std::vector<KnowledgeBase::CategoryStats> CustomerServiceManager::GetKbStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_kb) return {};
    return m_kb->GetStats();
}

} // namespace cs
} // namespace qi
