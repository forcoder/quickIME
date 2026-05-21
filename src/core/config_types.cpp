#include "quickinput/core/config_types.h"
#include <fstream>

namespace qi {

// 默认配置实现
HotkeyConfig HotkeyConfig::GetDefault() {
    return HotkeyConfig();
}

bool HotkeyConfig::Validate() const {
    if (aiSuggestionCount < 1 || aiSuggestionCount > 10) {
        OutputDebugStringW((L"[QuickInput] Error: AI建议条数超出范围: " + std::to_wstring(aiSuggestionCount) + L"\n").c_str());
        return false;
    }
    if (kbWeight < 0 || kbWeight > 100) {
        OutputDebugStringW((L"[QuickInput] Error: 知识库权重超出范围: " + std::to_wstring(kbWeight) + L"\n").c_str());
        return false;
    }
    if (triggerCharCount < 1 || triggerCharCount > 10) {
        OutputDebugStringW((L"[QuickInput] Error: 触发联想的最小字数超出范围: " + std::to_wstring(triggerCharCount) + L"\n").c_str());
        return false;
    }
    if (suggestionDelayMs < 0 || suggestionDelayMs > 5000) {
        OutputDebugStringW((L"[QuickInput] Error: 联想延迟超出范围: " + std::to_wstring(suggestionDelayMs) + L"ms\n").c_str());
        return false;
    }

    // 验证热键是否有效（基本ASCII字符）
    auto validateHotkey = [](WORD vkCode) -> bool {
        return (vkCode >= VK_F1 && vkCode <= VK_F24) ||
               (vkCode >= 'A' && vkCode <= 'Z') ||
               (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) ||
               (vkCode == VK_SPACE || vkCode == VK_RETURN || vkCode == VK_BACK ||
                vkCode == VK_TAB || vkCode == VK_ESCAPE);
    };

    if (!validateHotkey(hotkeyAICtrlAlt)) {
        OutputDebugStringW((L"[QuickInput] Error: 无效的AI联想热键: " + std::to_wstring(hotkeyAICtrlAlt) + L"\n").c_str());
        return false;
    }
    if (!validateHotkey(hotkeyKBCtrlAlt)) {
        OutputDebugStringW((L"[QuickInput] Error: 无效的知识库热键: " + std::to_wstring(hotkeyKBCtrlAlt) + L"\n").c_str());
        return false;
    }
    if (!validateHotkey(hotkeySaveCtrlAlt)) {
        OutputDebugStringW((L"[QuickInput] Error: 无效的保存热键: " + std::to_wstring(hotkeySaveCtrlAlt) + L"\n").c_str());
        return false;
    }
    if (!validateHotkey(hotkeyConfigCtrlAlt)) {
        OutputDebugStringW((L"[QuickInput] Error: 无效的配置热键: " + std::to_wstring(hotkeyConfigCtrlAlt) + L"\n").c_str());
        return false;
    }
    if (!validateHotkey(hotkeyModeCtrlAlt)) {
        OutputDebugStringW((L"[QuickInput] Error: 无效的模式切换热键: " + std::to_wstring(hotkeyModeCtrlAlt) + L"\n").c_str());
        return false;
    }

    return true;
}

} // namespace qi