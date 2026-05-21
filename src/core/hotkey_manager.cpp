#include "quickinput/core/hotkey_manager.h"
#include "quickinput/core/common.h"
#include "quickinput/core/utils.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace qi {

// 单例实例指针
HotkeyManager* HotkeyManager::s_instance = nullptr;

// 获取单例实例（线程安全）
HotkeyManager& HotkeyManager::GetInstance() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        s_instance = new HotkeyManager();
    });
    return *s_instance;
}

// 构造函数
HotkeyManager::HotkeyManager() {
    OutputDebugStringW((L"[QuickInput] Info: 快捷键管理器已创建\n").c_str());
}

// 析构函数
HotkeyManager::~HotkeyManager() {
    Shutdown();
    OutputDebugStringW((L"[QuickInput] Info: 快捷键管理器已销毁\n").c_str());
}

// 初始化
bool HotkeyManager::Initialize(HWND hMainWindow) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_hMainWindow != nullptr) {
        LOG_WARN(L"快捷键管理器已经初始化，跳过重复初始化");
        return true;
    }

    m_hMainWindow = hMainWindow;

    // 加载配置文件
    std::wstring configPath = utils::GetConfigDirectory() + L"\\hotkeys.json";
    if (!LoadConfig(configPath)) {
        LOG_WARN(L"无法加载热键配置，使用默认配置");
        m_config = HotkeyConfig::GetDefault();
    }

    // 注册所有默认热键
    bool success = true;

    // AI联想切换: Ctrl+Alt+A
    success &= RegisterHotkey(1001, m_config.hotkeyAICtrlAlt, m_config.modifierCtrlAlt);

    // 知识库联想切换: Ctrl+Alt+K
    success &= RegisterHotkey(1002, m_config.hotkeyKBCtrlAlt, m_config.modifierCtrlAlt);

    // 保存到知识库: Ctrl+Alt+.
    success &= RegisterHotkey(1003, m_config.hotkeySaveCtrlAlt, m_config.modifierCtrlAlt);

    // 打开配置面板: Ctrl+Alt+,
    success &= RegisterHotkey(1004, m_config.hotkeyConfigCtrlAlt, m_config.modifierCtrlAlt);

    // 切换输入模式: Ctrl+Alt+M
    success &= RegisterHotkey(1005, m_config.hotkeyModeCtrlAlt, m_config.modifierCtrlAlt);

    if (success) {
        OutputDebugStringW((L"[QuickInput] Info: 快捷键管理器初始化成功\n").c_str());
        ApplyCurrentConfig();
    } else {
        OutputDebugStringW((L"[QuickInput] Error: 快捷键管理器初始化失败\n").c_str());
    }

    return success;
}

// 关闭
void HotkeyManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 注销所有热键
    for (const auto& pair : m_hotkeys) {
        UnregisterHotkey(pair.first);
    }
    m_hotkeys.clear();

    m_hMainWindow = nullptr;
    OutputDebugStringW((L"[QuickInput] Info: 快捷键管理器已关闭\n").c_str());
}

// 注册热键
bool HotkeyManager::RegisterHotkey(uint32_t id, WORD vkCode, WORD modifiers) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_hMainWindow == nullptr) {
        LOG_ERROR(L"窗口句柄为空，无法注册热键 ID={}", id);
        return false;
    }

    // 检查是否已存在相同ID的热键
    if (m_hotkeys.find(id) != m_hotkeys.end()) {
        OutputDebugStringW((L"[QuickInput] Warning: 热键 ID=" + std::to_wstring(id) + L" 已存在，先注销旧的热键\n").c_str());
        UnregisterHotkey(id);
    }

    // 调用Windows API注册热键
    if (!RegisterHotKey(m_hMainWindow, id, modifiers, vkCode)) {
        DWORD error = GetLastError();
        OutputDebugStringW((L"[QuickInput] Error: 注册热键失败 ID=" + std::to_wstring(id) + L", VK=" + std::to_wstring(vkCode) + L", Modifiers=0x" + std::to_wstring(modifiers) + L", Error=" + std::to_wstring(error) + L"\n").c_str());
        return false;
    }

    // 保存热键信息
    m_hotkeys[id] = {vkCode, modifiers};
    OutputDebugStringW((L"[QuickInput] Info: 热键注册成功 ID=" + std::to_wstring(id) + L", VK=" + std::to_wstring(vkCode) + L", Modifiers=0x" + std::to_wstring(modifiers) + L"\n").c_str());

    return true;
}

// 注销热键
bool HotkeyManager::UnregisterHotkey(uint32_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_hotkeys.find(id);
    if (it == m_hotkeys.end()) {
        OutputDebugStringW((L"[QuickInput] Warning: 尝试注销不存在的热键 ID=" + std::to_wstring(id) + L"\n").c_str());
        return false;
    }

    // 调用Windows API注销热键
    if (!UnregisterHotKey(m_hMainWindow, id)) {
        DWORD error = GetLastError();
        OutputDebugStringW((L"[QuickInput] Error: 注销热键失败 ID=" + std::to_wstring(id) + L", Error=" + std::to_wstring(error) + L"\n").c_str());
        return false;
    }

    // 从映射中移除
    m_hotkeys.erase(it);
    OutputDebugStringW((L"[QuickInput] Info: 热键注销成功 ID=" + std::to_wstring(id) + L"\n").c_str());

    return true;
}

// 处理消息
LRESULT HotkeyManager::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (uMsg == WM_HOTKEY) {
        uint32_t hotkeyId = static_cast<uint32_t>(wParam);
        auto it = m_hotkeys.find(hotkeyId);

        if (it != m_hotkeys.end()) {
            WORD vkCode = it->second.first;
            WORD modifiers = it->second.second;

            OutputDebugStringW((L"[QuickInput] Debug: 触发热键 ID=" + std::to_wstring(hotkeyId) + L", VK=" + std::to_wstring(vkCode) + L", Modifiers=0x" + std::to_wstring(modifiers) + L"\n").c_str());

            // 根据热键ID执行相应操作
            switch (hotkeyId) {
                case 1001: // Ctrl+Alt+A - AI联想切换
                    OnAIAssociationToggle();
                    break;
                case 1002: // Ctrl+Alt+K - 知识库联想切换
                    OnKnowledgeBaseToggle();
                    break;
                case 1003: // Ctrl+Alt+. - 保存到知识库
                    OnSaveToKnowledgeBase();
                    break;
                case 1004: // Ctrl+Alt+, - 打开配置面板
                    OnOpenConfigPanel();
                    break;
                case 1005: // Ctrl+Alt+M - 切换输入模式
                    OnModeChanged();
                    break;
                default:
                    OutputDebugStringW((L"[QuickInput] Warning: 未知的热键ID: " + std::to_wstring(hotkeyId) + L"\n").c_str());
                    break;
            }

            return 0; // 消息已处理
        } else {
            OutputDebugStringW((L"[QuickInput] Warning: 收到未知热键消息 ID=" + std::to_wstring(hotkeyId) + L"\n").c_str());
        }
    }

    return DefWindowProc(m_hMainWindow, uMsg, wParam, lParam);
}

// 检查AI联想是否启用
bool HotkeyManager::IsAIAssociationEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_aiEnabled;
}

// 检查知识库联想是否启用
bool HotkeyManager::IsKnowledgeBaseEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_knowledgeEnabled;
}

// 获取配置
const HotkeyConfig& HotkeyManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

// 加载配置
bool HotkeyManager::LoadConfig(const std::wstring& configPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        // 检查文件是否存在
        if (!utils::FileExists(configPath)) {
            LOG_INFO(L"配置文件不存在: {}", configPath);
            return false;
        }

        // 读取文件内容
        std::ifstream file(configPath);
        if (!file.is_open()) {
            LOG_ERROR(L"无法打开配置文件: {}", configPath);
            return false;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        // 反序列化配置
        m_config = j.get<HotkeyConfig>();

        // 验证配置
        if (!m_config.Validate()) {
            LOG_ERROR(L"配置文件包含无效数据，使用默认配置");
            m_config = HotkeyConfig::GetDefault();
            return false;
        }

        OutputDebugStringW((L"[QuickInput] Info: 配置加载成功: " + configPath + L"\n").c_str());
        return true;

    } catch (const std::exception& e) {
        OutputDebugStringW((L"[QuickInput] Error: 加载配置文件时发生异常: " + configPath + L", 错误: " + std::string(e.what()) + L"\n").c_str());
        return false;
    } catch (...) {
        OutputDebugStringW((L"[QuickInput] Error: 加载配置文件时发生未知异常: " + configPath + L"\n").c_str());
        return false;
    }
}

// 保存配置
bool HotkeyManager::SaveConfig(const std::wstring& configPath) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        // 确保配置有效
        if (!m_config.Validate()) {
            LOG_ERROR(L"尝试保存无效的配置");
            return false;
        }

        // 创建目录（如果不存在）
        std::wstring dir = utils::GetParentDirectory(configPath);
        if (!utils::CreateDirectory(dir)) {
            LOG_ERROR(L"无法创建配置目录: {}", dir);
            return false;
        }

        // 序列化配置
        nlohmann::json j = m_config;

        // 写入文件
        std::ofstream file(configPath);
        if (!file.is_open()) {
            LOG_ERROR(L"无法打开配置文件进行写入: {}", configPath);
            return false;
        }

        file << j.dump(4); // 缩进4个空格
        file.close();

        OutputDebugStringW((L"[QuickInput] Info: 配置保存成功: " + configPath + L"\n").c_str());
        return true;

    } catch (const std::exception& e) {
        OutputDebugStringW((L"[QuickInput] Error: 保存配置文件时发生异常: " + configPath + L", 错误: " + std::string(e.what()) + L"\n").c_str());
        return false;
    } catch (...) {
        OutputDebugStringW((L"[QuickInput] Error: 保存配置文件时发生未知异常: " + configPath + L"\n").c_str());
        return false;
    }
}

// 更新配置
void HotkeyManager::UpdateConfig(const HotkeyConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (config.Validate()) {
        m_config = config;
        ApplyCurrentConfig();

        // 通知配置变更
        if (m_onConfigChanged) {
            m_onConfigChanged();
        }

        OutputDebugStringW((L"[QuickInput] Info: 配置已更新\n").c_str());
    } else {
        OutputDebugStringW((L"[QuickInput] Error: 尝试更新无效的配置\n").c_str());
    }
}

// 通知AI联想状态变化
void HotkeyManager::NotifyAIAssociationToggled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aiEnabled = enabled;
    OutputDebugStringW((L"[QuickInput] Info: AI联想状态已切换为: " + std::wstring(enabled ? L"启用" : L"禁用") + L"\n").c_str());
}

// 通知知识库联想状态变化
void HotkeyManager::NotifyKnowledgeBaseToggled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_knowledgeEnabled = enabled;
    OutputDebugStringW((L"[QuickInput] Info: 知识库联想状态已切换为: " + std::wstring(enabled ? L"启用" : L"禁用") + L"\n").c_str());
}

// 通知模式变化
void HotkeyManager::NotifyModeChanged(InputMode mode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentMode = mode;
    OutputDebugStringW((L"[QuickInput] Info: 输入模式已切换为: " + std::to_wstring(static_cast<int>(mode)) + L"\n").c_str());
}

// 私有方法实现

// AI联想切换
void HotkeyManager::OnAIAssociationToggle() {
    m_aiEnabled = !m_aiEnabled;
    NotifyAIAssociationToggled(m_aiEnabled);
    OutputDebugStringW((L"[QuickInput] Info: 通过热键切换AI联想: " + std::wstring(m_aiEnabled ? L"启用" : L"禁用") + L"\n").c_str());
}

// 知识库联想切换
void HotkeyManager::OnKnowledgeBaseToggle() {
    m_knowledgeEnabled = !m_knowledgeEnabled;
    NotifyKnowledgeBaseToggled(m_knowledgeEnabled);
    OutputDebugStringW((L"[QuickInput] Info: 通过热键切换知识库联想: " + std::wstring(m_knowledgeEnabled ? L"启用" : L"禁用") + L"\n").c_str());
}

// 保存到知识库
void HotkeyManager::OnSaveToKnowledgeBase() {
    OutputDebugStringW((L"[QuickInput] Info: 通过热键触发保存到知识库\n").c_str());
    // TODO: 通知其他模块保存当前输入到知识库
}

// 打开配置面板
void HotkeyManager::OnOpenConfigPanel() {
    OutputDebugStringW((L"[QuickInput] Info: 通过热键打开配置面板\n").c_str());
    // TODO: 通知UI模块显示配置对话框
}

// 切换输入模式
void HotkeyManager::OnModeChanged() {
    InputMode newMode = static_cast<InputMode>((static_cast<int>(m_currentMode) + 1) % 3);
    m_currentMode = newMode;
    NotifyModeChanged(newMode);
    OutputDebugStringW((L"[QuickInput] Info: 通过热键切换输入模式: " + std::to_wstring(static_cast<int>(newMode)) + L"\n").c_str());
}

// 应用当前配置
void HotkeyManager::ApplyCurrentConfig() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 重新注册所有热键以应用新的热键设置
    bool success = true;

    success &= RegisterHotkey(1001, m_config.hotkeyAICtrlAlt, m_config.modifierCtrlAlt);
    success &= RegisterHotkey(1002, m_config.hotkeyKBCtrlAlt, m_config.modifierCtrlAlt);
    success &= RegisterHotkey(1003, m_config.hotkeySaveCtrlAlt, m_config.modifierCtrlAlt);
    success &= RegisterHotkey(1004, m_config.hotkeyConfigCtrlAlt, m_config.modifierCtrlAlt);
    success &= RegisterHotkey(1005, m_config.hotkeyModeCtrlAlt, m_config.modifierCtrlAlt);

    if (success) {
        OutputDebugStringW((L"[QuickInput] Info: 配置已应用到热键系统\n").c_str());
    } else {
        OutputDebugStringW((L"[QuickInput] Error: 应用配置到热键系统失败\n").c_str());
    }
}

} // namespace qi