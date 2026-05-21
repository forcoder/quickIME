#ifndef QUICKINPUT_CORE_HOTKEY_MANAGER_H_
#define QUICKINPUT_CORE_HOTKEY_MANAGER_H_

#include "quickinput/core/config_types.h"
#include "quickinput/core/common.h"
#include <unordered_map>
#include <mutex>
#include <functional>

namespace qi {

class HotkeyManager {
public:
    static HotkeyManager& GetInstance();

    // ── 初始化 ──
    bool Initialize(HWND hMainWindow);
    void Shutdown();

    // ── 注册快捷键 ──
    bool RegisterHotkey(uint32_t id, WORD vkCode, WORD modifiers);
    bool UnregisterHotkey(uint32_t id);

    // ── 事件处理 ──
    // 在窗口消息循环中调用
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // ── 状态查询 ──
    bool IsAIAssociationEnabled() const;
    bool IsKnowledgeBaseEnabled() const;
    const HotkeyConfig& GetConfig() const;

    // ── 配置管理 ──
    bool LoadConfig(const std::wstring& configPath);
    bool SaveConfig(const std::wstring& configPath) const;
    void UpdateConfig(const HotkeyConfig& config);

    // ── 通知其他模块 ──
    void NotifyAIAssociationToggled(bool enabled);
    void NotifyKnowledgeBaseToggled(bool enabled);
    void NotifyModeChanged(InputMode mode);

    // 设置配置变更回调
    void SetConfigChangedCallback(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onConfigChanged = std::move(callback);
    }

    // 获取热键ID对应的虚拟键码和修饰符
    std::pair<WORD, WORD> GetHotkeyInfo(uint32_t id) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_hotkeys.find(id);
        if (it != m_hotkeys.end()) {
            return it->second;
        }
        return {0, 0};
    }

private:
    HotkeyManager();
    ~HotkeyManager();

    // 单例实例
    static HotkeyManager* s_instance;

    // 窗口句柄
    HWND m_hMainWindow = nullptr;

    // 快捷键映射: ID -> {虚拟键码, 修饰符}
    std::unordered_map<uint32_t, std::pair<WORD, WORD>> m_hotkeys;

    // 当前配置
    HotkeyConfig m_config;

    // 当前状态
    bool m_aiEnabled = true;
    bool m_knowledgeEnabled = true;
    InputMode m_currentMode = InputMode::Hybrid;

    // 线程安全
    mutable std::mutex m_mutex;

    // 配置保存回调
    std::function<void()> m_onConfigChanged;

    // 私有方法
    void OnAIAssociationToggle();
    void OnKnowledgeBaseToggle();
    void OnSaveToKnowledgeBase();
    void OnOpenConfigPanel();
    void OnModeChanged();
    void ApplyCurrentConfig();
};

} // namespace qi

#endif // QUICKINPUT_CORE_HOTKEY_MANAGER_H_