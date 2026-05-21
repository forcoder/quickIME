#ifndef QUICKINPUT_CORE_CONFIG_TYPES_H_
#define QUICKINPUT_CORE_CONFIG_TYPES_H_

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace qi {

// 输入模式枚举
enum class InputMode {
    Wubi,        // 纯五笔模式
    Pinyin,      // 拼音模式
    Hybrid       // 混合模式
};

// AI 联想配置
struct AIAssociationConfig {
    bool enabled = true;           // 是否启用AI联想
    int maxSuggestions = 5;          // 最大建议条数 (1-10)
    int delayMs = 200;               // 联想延迟（毫秒）
    float confidenceThreshold = 0.6f; // 置信度阈值 (0.0-1.0)
};

// 知识库配置
struct KnowledgeBaseConfig {
    bool enabled = true;             // 是否启用知识库联想
    int weight = 30;                 // 知识库权重 (0-100)
    std::string dbPath = L"knowledge.db"; // 数据库路径
    int maxEntries = 1000;           // 最大条目数
};

// 热键配置
struct HotkeyConfig {
    // AI联想相关
    bool aiEnabled = true;           // AI联想开关
    bool knowledgeEnabled = true;      // 知识库联想开关
    int aiSuggestionCount = 5;         // AI建议条数
    int kbWeight = 30;               // 知识库权重 (0-100)
    int triggerCharCount = 2;        // 触发联想的最小字数
    int suggestionDelayMs = 200;     // 联想延迟（毫秒）

    // 热键定义
    WORD hotkeyAICtrlAlt = VK_OEM_4; // Ctrl+Alt+A - AI联想切换 (默认: [ )
    WORD hotkeyKBCtrlAlt = VK_OEM_6; // Ctrl+Alt+K - 知识库联想切换 (默认: ] )
    WORD hotkeySaveCtrlAlt = VK_OEM_PERIOD; // Ctrl+Alt+. - 保存到知识库 (默认: . )
    WORD hotkeyConfigCtrlAlt = VK_OEM_COMMA; // Ctrl+Alt+, - 打开配置面板 (默认: , )
    WORD hotkeyModeCtrlAlt = VK_OEM_1; // Ctrl+Alt+M - 切换输入模式 (默认: ; )

    // 修饰键掩码
    WORD modifierCtrlAlt = MOD_CONTROL | MOD_ALT;

    // 输入模式
    InputMode inputMode = InputMode::Hybrid;

    // 验证配置有效性
    bool Validate() const {
        if (aiSuggestionCount < 1 || aiSuggestionCount > 10) return false;
        if (kbWeight < 0 || kbWeight > 100) return false;
        if (triggerCharCount < 1 || triggerCharCount > 10) return false;
        if (suggestionDelayMs < 0 || suggestionDelayMs > 5000) return false;
        return true;
    }

    // 获取默认配置
    static HotkeyConfig GetDefault() {
        return HotkeyConfig();
    }
};

// JSON序列化辅助函数
inline void to_json(nlohmann::json& j, const HotkeyConfig& config) {
    j = nlohmann::json{
        {"aiEnabled", config.aiEnabled},
        {"knowledgeEnabled", config.knowledgeEnabled},
        {"aiSuggestionCount", config.aiSuggestionCount},
        {"kbWeight", config.kbWeight},
        {"triggerCharCount", config.triggerCharCount},
        {"suggestionDelayMs", config.suggestionDelayMs},
        {"hotkeyAICtrlAlt", static_cast<int>(config.hotkeyAICtrlAlt)},
        {"hotkeyKBCtrlAlt", static_cast<int>(config.hotkeyKBCtrlAlt)},
        {"hotkeySaveCtrlAlt", static_cast<int>(config.hotkeySaveCtrlAlt)},
        {"hotkeyConfigCtrlAlt", static_cast<int>(config.hotkeyConfigCtrlAlt)},
        {"hotkeyModeCtrlAlt", static_cast<int>(config.hotkeyModeCtrlAlt)},
        {"modifierCtrlAlt", static_cast<int>(config.modifierCtrlAlt)},
        {"inputMode", static_cast<int>(config.inputMode)}
    };
}

inline void from_json(const nlohmann::json& j, HotkeyConfig& config) {
    j.at("aiEnabled").get_to(config.aiEnabled);
    j.at("knowledgeEnabled").get_to(config.knowledgeEnabled);
    j.at("aiSuggestionCount").get_to(config.aiSuggestionCount);
    j.at("kbWeight").get_to(config.kbWeight);
    j.at("triggerCharCount").get_to(config.triggerCharCount);
    j.at("suggestionDelayMs").get_to(config.suggestionDelayMs);
    j.at("hotkeyAICtrlAlt").get_to(config.hotkeyAICtrlAlt);
    j.at("hotkeyKBCtrlAlt").get_to(config.hotkeyKBCtrlAlt);
    j.at("hotkeySaveCtrlAlt").get_to(config.hotkeySaveCtrlAlt);
    j.at("hotkeyConfigCtrlAlt").get_to(config.hotkeyConfigCtrlAlt);
    j.at("hotkeyModeCtrlAlt").get_to(config.hotkeyModeCtrlAlt);
    j.at("modifierCtrlAlt").get_to(config.modifierCtrlAlt);
    j.at("inputMode").get_to(config.inputMode);
}

} // namespace qi

#endif // QUICKINPUT_CORE_CONFIG_TYPES_H_