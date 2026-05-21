#pragma once

#include "quickinput/core/common.h"
#include "quickinput/core/context_manager.h"
#include "quickinput/ai/inference_engine.h"
#include "quickinput/knowledge/knowledge_base.h"
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace qi {
namespace cs {

// 客服建议来源
enum class SuggestionSource : uint8_t {
    KnowledgeBase,      // 知识库精确匹配
    AIGenerated,        // AI 生成
    Hybrid,             // 混合（知识库+AI）
    Template            // 固定模板
};

// 客服建议项
struct CSSuggestion {
    std::wstring    text;           // 建议文本
    SuggestionSource source;        // 来源
    float           confidence;     // 置信度
    KbCategory      kbCategory;     // 知识库分类（如果来自知识库）
    AISuggestionType aiType;        // AI 类型（如果来自AI）

    CSSuggestion()
        : source(SuggestionSource::Template)
        , confidence(0.0f)
        , kbCategory(KbCategory::General)
        , aiType(AISuggestionType::ShortReply)
    {}
};

// 客服场景管理器
// 负责协调知识库检索和AI推理，生成客服回复建议
class CustomerServiceManager {
public:
    static CustomerServiceManager& GetInstance();

    // ── 初始化 ──
    bool Initialize(
        const std::wstring& modelPath,
        const std::wstring& kbPath);
    void Shutdown();

    // ── 核心功能 ──
    // 根据对话上下文生成客服建议
    std::vector<CSSuggestion> GenerateSuggestions(
        const std::wstring& customerMessage,
        int maxSuggestions = 5);

    // 手动触发知识库搜索
    std::vector<KbResult> SearchKnowledgeBase(
        const std::wstring& query,
        int maxResults = 5,
        KbCategory filter = KbCategory::General);

    // 手动触发AI推理
    std::vector<AISuggestion> GenerateAIResponse(
        const std::wstring& message,
        AISuggestionType type,
        int count = 3);

    // ── 知识库管理 ──
    bool ImportKbFile(const std::wstring& filePath, KbCategory category);
    bool ImportKbText(const std::wstring& text, KbCategory category);
    std::vector<KnowledgeBase::CategoryStats> GetKbStats();

    // ── 配置 ──
    struct Config {
        bool aiEnabled = true;
        bool kbEnabled = true;
        float aiWeight = 0.5f;      // AI建议权重
        float kbWeight = 0.5f;      // 知识库建议权重
        int maxSuggestions = 5;     // 最大建议数
        float minConfidence = 0.3f; // 最低置信度
    };

    void SetConfig(const Config& config) { m_config = config; }
    const Config& GetConfig() const { return m_config; }

    // ── 状态 ──
    bool IsReady() const { return m_isReady; }
    bool IsAIEnabled() const { return m_config.aiEnabled && m_aiEngine && m_aiEngine->IsModelLoaded(); }
    bool IsKBEnabled() const { return m_config.kbEnabled && m_kb; }

private:
    CustomerServiceManager();
    ~CustomerServiceManager();

    // 合并知识和AI建议
    std::vector<CSSuggestion> MergeSuggestions(
        const std::vector<KbResult>& kbResults,
        const std::vector<AISuggestion>& aiResults,
        int maxCount);

    // 判断场景类型
    ContextManager::DialogMode DetectScene(const std::wstring& message);

    // 组件
    std::unique_ptr<qi::ai::InferenceEngine> m_aiEngine;
    std::unique_ptr<qi::knowledge::KnowledgeBase> m_kb;
    Config m_config;
    bool m_isReady = false;
    std::mutex m_mutex;
};

} // namespace cs
} // namespace qi
