#include "quickinput/core/context_manager.h"
#include "quickinput/core/input_monitor.h"
#include <iostream>
#include <sstream>

namespace qi {

ContextManager* ContextManager::s_instance = nullptr;

ContextManager& ContextManager::GetInstance() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        s_instance = new ContextManager();
    });
    return *s_instance;
}

ContextManager::ContextManager() : m_lastUpdate(std::chrono::steady_clock::now()) {
    // 初始化上下文
    m_context.composition = L"";
    m_context.committedText = L"";
    m_context.surroundingText = L"";
    m_context.cursorPos = 0;
    m_context.isChineseMode = true;
    m_context.isFullWidth = false;
    m_context.mode = InputMode::Hybrid;
}

ContextManager::~ContextManager() {
    Shutdown();
}

bool ContextManager::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 注册全局键盘钩子
    m_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                       GetModuleHandle(nullptr), 0);
    if (!m_keyboardHook) {
        OutputDebugStringW(L"[QuickInput] Failed to install keyboard hook\n");
        return false;
    }

    OutputDebugStringW(L"[QuickInput] Context Manager initialized successfully\n");
    return true;
}

void ContextManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
}

bool ContextManager::UpdateContext(HWND activeWindow) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_activeWindow = activeWindow;
    m_lastUpdate = std::chrono::steady_clock::now();

    if (!activeWindow || !IsWindow(activeWindow)) {
        return false;
    }

    // 获取窗口文本和光标位置
    std::wstring windowText = GetTextFromWindow(activeWindow);
    if (!windowText.empty()) {
        m_context.surroundingText = windowText;
        m_context.cursorPos = GetCaretPosition(activeWindow);
    }

    // 更新上下文信息
    return true;
}

std::wstring ContextManager::GetWindowTitle() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_activeWindow || !IsWindow(m_activeWindow)) {
        return L"";
    }

    int length = GetWindowTextLengthW(m_activeWindow);
    if (length <= 0) {
        return L"";
    }

    std::wstring title(length + 1, L'\0');
    GetWindowTextW(m_activeWindow, &title[0], length + 1);
    title.resize(length);
    return title;
}

std::wstring ContextManager::GetClassName() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_activeWindow || !IsWindow(m_activeWindow)) {
        return L"";
    }

    int length = GetClassNameW(m_activeWindow, nullptr, 0);
    if (length <= 0) {
        return L"";
    }

    std::wstring className(length, L'\0');
    GetClassNameW(m_activeWindow, &className[0], length);
    return className;
}

std::wstring ContextManager::GetTextAroundCursor(int offsetBefore, int offsetAfter) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_context.surroundingText.empty()) {
        return L"";
    }

    int textLen = static_cast<int>(m_context.surroundingText.length());
    int startPos = std::max(0, m_context.cursorPos - offsetBefore);
    int endPos = std::min(textLen, m_context.cursorPos + offsetAfter);

    if (startPos >= endPos) {
        return L"";
    }

    return m_context.surroundingText.substr(startPos, endPos - startPos);
}

std::vector<std::wstring> ContextManager::ExtractParagraphs(const std::wstring& fullText) const {
    std::vector<std::wstring> paragraphs;
    std::wistringstream stream(fullText);
    std::wstring line;

    while (std::getline(stream, line)) {
        // 移除行首尾的空白字符
        size_t start = line.find_first_not_of(L" \t\r\n");
        if (start != std::wstring::npos) {
            size_t end = line.find_last_not_of(L" \t\r\n");
            paragraphs.push_back(line.substr(start, end - start + 1));
        }
    }

    return paragraphs;
}

void ContextManager::SetComposition(const std::wstring& code) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_context.composition = code;
}

void ContextManager::SetCommittedText(const std::wstring& text) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_context.committedText = text;
}

void ContextManager::SetCursorPos(int pos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_context.cursorPos = pos;
}

void ContextManager::SetMode(InputMode mode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_context.mode = mode;
}

void ContextManager::SetChineseMode(bool chinese) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_context.isChineseMode = chinese;
}

void ContextManager::SetFullWidthMode(bool fullWidth) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_context.isFullWidth = fullWidth;
}

// 静态方法实现

LRESULT CALLBACK ContextManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        const KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

        // 获取当前上下文管理器实例
        auto& contextMgr = GetInstance();

        // 检查是否是我们要处理的窗口
        if (contextMgr.GetActiveWindow()) {
            // 触发上下文更新
            contextMgr.UpdateContext(contextMgr.GetActiveWindow());
        }
    }

    // 调用下一个钩子
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK ContextManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_GETTEXT: {
            // 处理文本请求
            return 0;
        }
        case WM_GETTEXTLENGTH: {
            // 处理文本长度请求
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

std::wstring ContextManager::GetTextFromWindow(HWND hwnd, int maxLength) const {
    if (!hwnd || !IsWindow(hwnd)) {
        return L"";
    }

    // 尝试获取文本长度
    int length = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
    if (length <= 0) {
        return L"";
    }

    // 限制最大长度
    length = std::min(length, maxLength);

    // 获取文本
    std::wstring text(length + 1, L'\0');
    int result = SendMessageW(hwnd, WM_GETTEXT, length + 1, reinterpret_cast<LPARAM>(&text[0]));

    if (result > 0) {
        text.resize(result);
        return text;
    }

    return L"";
}

std::wstring ContextManager::GetTextInRange(int start, int end) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_context.surroundingText.empty()) {
        return L"";
    }

    int textLen = static_cast<int>(m_context.surroundingText.length());

    // 调整边界
    start = std::max(0, start);
    end = std::min(textLen, end);

    if (start >= end) {
        return L"";
    }

    return m_context.surroundingText.substr(start, end - start);
}

// 辅助函数：获取光标位置
int ContextManager::GetCaretPosition(HWND hwnd) const {
    if (!hwnd || !IsWindow(hwnd)) {
        return 0;
    }

    // 使用 GetGUIThreadInfo 获取光标信息
    GUITHREADINFO threadInfo = {};
    threadInfo.cbSize = sizeof(GUITHREADINFO);

    if (GetGUIThreadInfo(GetWindowThreadProcessId(hwnd, nullptr), &threadInfo)) {
        // 如果找到了光标，返回一个合理的默认位置
        // 在实际应用中，可能需要更复杂的逻辑来计算确切的光标位置
        return 10; // 简化处理，返回默认位置
    }

    return 0;
}

// ── 客服场景上下文实现 ──

std::wstring ContextManager::ExtractConversationContext(int maxChars) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_context.surroundingText.empty()) {
        return L"";
    }

    // 提取光标前后的文本作为对话上下文
    std::wstring context = GetTextAroundCursor(maxChars / 2, maxChars / 2);

    // 如果上下文太短，尝试获取更多文本
    if (context.size() < static_cast<size_t>(maxChars / 2)) {
        context = m_context.surroundingText;
        if (context.size() > static_cast<size_t>(maxChars)) {
            context = context.substr(context.size() - maxChars);
        }
    }

    return context;
}

ContextManager::DialogMode ContextManager::DetectDialogMode(const std::wstring& text) const {
    // 关键词匹配识别对话模式
    static const std::vector<std::pair<DialogMode, std::vector<std::wstring>>> keywords = {
        {DialogMode::Complaint, {L"投诉", L"不满意", L"差评", L"举报", L"欺骗", L"虚假宣传", L"态度差", L"服务差"}},
        {DialogMode::OrderQuery, {L"订单", L"快递", L"发货", L"物流", L"到哪了", L"什么时候到", L"运单号", L"查一下"}},
        {DialogMode::Refund, {L"退款", L"退货", L"退钱", L"不要了", L"取消订单", L"申请退", L"退款进度"}},
        {DialogMode::Inquiry, {L"请问", L"想了解", L"咨询", L"怎么", L"如何", L"什么", L"多少", L"价格"}}
    };

    for (const auto& pair : keywords) {
        for (const auto& keyword : pair.second) {
            if (text.find(keyword) != std::wstring::npos) {
                return pair.first;
            }
        }
    }

    return DialogMode::General;
}

std::vector<std::wstring> ContextManager::ExtractKeyInfo(const std::wstring& text) const {
    std::vector<std::wstring> keyInfo;

    // 提取订单号（格式：字母+数字，通常8-20位）
    // 简单实现：查找连续的数字串
    std::wstring currentNumber;
    for (wchar_t c : text) {
        if (std::iswdigit(c)) {
            currentNumber += c;
        } else {
            if (currentNumber.size() >= 6) {
                keyInfo.push_back(currentNumber);
            }
            currentNumber.clear();
        }
    }
    if (currentNumber.size() >= 6) {
        keyInfo.push_back(currentNumber);
    }

    return keyInfo;
}

std::vector<std::wstring> ContextManager::GetRecentMessages(int count) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::wstring> messages;
    auto paragraphs = ExtractParagraphs(m_context.surroundingText);

    // 取最后 count 条
    int start = std::max(0, static_cast<int>(paragraphs.size()) - count);
    for (int i = start; i < static_cast<int>(paragraphs.size()); ++i) {
        if (!paragraphs[i].empty()) {
            messages.push_back(paragraphs[i]);
        }
    }

    return messages;
}

} // namespace qi