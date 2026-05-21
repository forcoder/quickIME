#include "quickinput/core/input_monitor.h"
#include "quickinput/core/context_manager.h"
#include <iostream>

namespace qi {

InputMonitor* InputMonitor::s_instance = nullptr;

InputMonitor& InputMonitor::GetInstance() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        s_instance = new InputMonitor();
    });
    return *s_instance;
}

InputMonitor::InputMonitor() {
    // 获取上下文管理器实例
    m_contextManager = &ContextManager::GetInstance();
}

InputMonitor::~InputMonitor() {
    Shutdown();
}

bool InputMonitor::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 注册全局键盘钩子
    m_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                       GetModuleHandle(nullptr), 0);
    if (!m_keyboardHook) {
        OutputDebugStringW(L"[QuickInput] Failed to install keyboard hook for input monitor\n");
        return false;
    }

    OutputDebugStringW(L"[QuickInput] Input Monitor initialized successfully\n");
    return true;
}

void InputMonitor::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    StopMonitoring();

    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
}

void InputMonitor::SetEventCallback(const InputEventCallback& callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventCallback = callback;
}

bool InputMonitor::StartMonitoring() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isMonitoring) {
        return true; // 已经在监控中
    }

    if (!m_keyboardHook) {
        return false;
    }

    m_isMonitoring = true;
    OutputDebugStringW(L"[QuickInput] Input monitoring started\n");
    return true;
}

void InputMonitor::StopMonitoring() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isMonitoring) {
        return;
    }

    m_isMonitoring = false;
    OutputDebugStringW(L"[QuickInput] Input monitoring stopped\n");
}

bool InputMonitor::IsWubiCompositionMode() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查当前是否是五笔编码输入模式
    // 这需要检查上下文管理器中的状态
    if (m_contextManager) {
        const auto& context = m_contextManager->GetContext();
        // 如果正在输入编码且没有提交文本，可能是五笔模式
        return !context.composition.empty() && context.committedText.empty();
    }

    return false;
}

bool InputMonitor::IsCompositionEvent(const InputEvent& event) const {
    return event.type == InputEventType::Composition;
}

bool InputMonitor::IsDeleteOperation(const InputEvent& event) const {
    return event.type == InputEventType::Delete ||
           IsBackspaceKey(event) ||
           (event.wParam == VK_DELETE);
}

bool InputMonitor::IsEnterKey(const InputEvent& event) const {
    return event.wParam == VK_RETURN || event.wParam == VK_EXECUTE;
}

bool InputMonitor::IsBackspaceKey(const InputEvent& event) const {
    return event.wParam == VK_BACK;
}

const InputContext& InputMonitor::GetCurrentContext() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_contextManager) {
        return m_contextManager->GetContext();
    }

    // 返回默认上下文
    static InputContext defaultContext;
    return defaultContext;
}

// 静态方法实现

LRESULT CALLBACK InputMonitor::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        const KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

        auto& monitor = GetInstance();

        if (monitor.m_isMonitoring) {
            monitor.HandleKeyboardEvent(kbStruct, wParam);
        }
    }

    // 调用下一个钩子
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void InputMonitor::HandleKeyboardEvent(const KBDLLHOOKSTRUCT* kbStruct, WPARAM wParam) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新修饰键状态
    UpdateModifierKeys(kbStruct, wParam);

    // 创建输入事件
    InputEvent event = CreateInputEvent(wParam, reinterpret_cast<LPARAM>(kbStruct));

    // 设置扫描码和扩展键信息
    event.scanCode = kbStruct->scanCode;
    event.isExtended = (kbStruct->flags & LLKHF_EXTENDED) != 0;

    // 根据按键类型确定事件类型
    switch (wParam) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            event.type = InputEventType::KeyDown;
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            event.type = InputEventType::KeyUp;
            break;
        case WM_CHAR:
        case WM_SYSCHAR:
            event.type = InputEventType::CharInput;
            // 尝试获取字符文本
            if (kbStruct->vkCode >= 'A' && kbStruct->vkCode <= 'Z') {
                event.text = static_cast<wchar_t>(kbStruct->vkCode + 32);
            } else {
                event.text = static_cast<wchar_t>(kbStruct->vkCode);
            }
            break;
        default:
            event.type = InputEventType::Other;
            break;
    }

    // 保存最后的事件
    m_lastEvent = event;

    // 触发回调
    if (m_eventCallback) {
        m_eventCallback(event);
    }
}

void InputMonitor::UpdateModifierKeys(const KBDLLHOOKSTRUCT* kbStruct, WPARAM wParam) {
    bool wasAltDown = m_isAltDown;
    bool wasCtrlDown = m_isCtrlDown;
    bool wasShiftDown = m_isShiftDown;

    switch (kbStruct->vkCode) {
        case VK_MENU:     // Alt key
            m_isAltDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            break;
        case VK_CONTROL:  // Ctrl key
            m_isCtrlDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            break;
        case VK_SHIFT:    // Shift key
            m_isShiftDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            break;
    }

    // 只有在状态发生变化时才记录
    if (m_isAltDown != wasAltDown || m_isCtrlDown != wasCtrlDown || m_isShiftDown != wasShiftDown) {
        // 更新修饰键状态到事件对象中
        m_lastEvent.isAltDown = m_isAltDown;
        m_lastEvent.isCtrlDown = m_isCtrlDown;
        m_lastEvent.isShiftDown = m_isShiftDown;
    }
}

InputEvent InputMonitor::CreateInputEvent(WPARAM wParam, LPARAM lParam) const {
    InputEvent event;
    event.wParam = wParam;
    event.lParam = lParam;
    event.time = GetMessageTime(); // 获取消息时间戳

    // 初始化修饰键状态
    event.isAltDown = m_isAltDown;
    event.isCtrlDown = m_isCtrlDown;
    event.isShiftDown = m_isShiftDown;

    return event;
}

} // namespace qi