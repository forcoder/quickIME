#pragma once

#include "quickinput/core/common.h"
#include <windows.h>
#include <chrono>
#include <mutex>

namespace qi {

// 输入上下文信息
struct InputContext {
    std::wstring    composition;        // 当前输入的五笔编码串
    std::wstring    committedText;      // 已上屏的文本
    std::wstring    surroundingText;    // 光标周围文本（上下文）
    int             cursorPos;          // 光标位置
    bool            isChineseMode;      // 中文模式
    bool            isFullWidth;        // 全角模式
    InputMode       mode;               // 当前输入模式

    InputContext() : cursorPos(0), isChineseMode(true), isFullWidth(false), mode(InputMode::Hybrid) {}
};

// 上下文抓取器
class ContextManager {
public:
    static ContextManager& GetInstance();

    // 初始化（注册全局键盘钩子）
    bool Initialize();
    void Shutdown();

    // ── 实时上下文获取 ──
    // 从活动窗口获取光标周围文本
    bool UpdateContext(HWND activeWindow);

    // 获取当前完整上下文
    const InputContext& GetContext() const { return m_context; }

    // ── 窗口管理 ──
    // 获取活动窗口信息
    HWND GetActiveWindow() const { return m_activeWindow; }
    std::wstring GetWindowTitle() const;
    std::wstring GetClassName() const;

    // ── 文本提取 ──
    // 从窗口获取指定范围的文本
    std::wstring GetTextAroundCursor(int offsetBefore = 50, int offsetAfter = 50) const;

    // 智能分段（按段落、句子边界）
    std::vector<std::wstring> ExtractParagraphs(const std::wstring& fullText) const;

    // ── 状态管理 ──
    void SetComposition(const std::wstring& code);
    void SetCommittedText(const std::wstring& text);
    void SetCursorPos(int pos);
    void SetMode(InputMode mode);
    void SetChineseMode(bool chinese);
    void SetFullWidthMode(bool fullWidth);

    // ── 客服场景上下文 ──
    // 提取对话上下文（用于客服建议）
    std::wstring ExtractConversationContext(int maxChars = 500) const;
    // 识别对话模式
    enum class DialogMode : uint8_t {
        Unknown = 0,
        Inquiry,        // 咨询
        Complaint,      // 投诉
        OrderQuery,     // 订单查询
        Refund,         // 退款
        General         // 一般对话
    };
    DialogMode DetectDialogMode(const std::wstring& text) const;
    // 提取关键信息（订单号、日期等）
    std::vector<std::wstring> ExtractKeyInfo(const std::wstring& text) const;
    // 获取最近N条对话
    std::vector<std::wstring> GetRecentMessages(int count = 5) const;

private:
    ContextManager();
    ~ContextManager();

    // 全局键盘钩子（WH_KEYBOARD_LL）
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    // 窗口消息处理（WM_GETTEXT, WM_GETTEXTLENGTH）
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 文本提取辅助
    std::wstring GetTextFromWindow(HWND hwnd, int maxLength = 32767) const;
    std::wstring GetTextInRange(int start, int end) const;

    // 线程安全
    mutable std::mutex m_mutex;

    // 上下文数据
    InputContext m_context;

    // 窗口句柄
    HWND m_activeWindow = nullptr;

    // 键盘钩子
    HHOOK m_keyboardHook = nullptr;

    // 上次更新
    std::chrono::steady_clock::time_point m_lastUpdate;

    // 单例
    static ContextManager* s_instance;
};

} // namespace qi