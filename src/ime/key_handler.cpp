#include "quickinput/ime/key_handler.h"
#include "quickinput/ime/composition.h"

#include <msctf.h>

namespace qi {
namespace ime {

// ═══════════════════════════════════════════════════════════════
// Virtual key code constants
// ═══════════════════════════════════════════════════════════════

// 使用标准 VK_* 常量，不需要额外定义

// ═══════════════════════════════════════════════════════════════
// Construction / Destruction
// ═══════════════════════════════════════════════════════════════

KeyHandler::KeyHandler()
    : m_context(nullptr)
    , m_compositionMgr(nullptr)
    , m_isInitialized(false)
{
}

KeyHandler::~KeyHandler()
{
    // 不拥有 m_context 和 m_compositionMgr，无需释放
}

// ═══════════════════════════════════════════════════════════════
// Initialization
// ═══════════════════════════════════════════════════════════════

HRESULT KeyHandler::Initialize(ITfContext* pContext, CompositionManager* pCompositionMgr)
{
    m_context = pContext;
    m_compositionMgr = pCompositionMgr;
    m_isInitialized = true;
    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// Key event handlers
// ═══════════════════════════════════════════════════════════════

HRESULT KeyHandler::OnTestKeyDown(WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_isInitialized) return S_OK;

    // 英文模式下只处理切换键
    if (!m_state.isChineseMode)
    {
        if (wParam == VK_SHIFT)
        {
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    // 中文模式下判断是否需要处理
    if (IsWubiCodeChar(wParam) || IsNumberKey(wParam))
    {
        *pfEaten = TRUE;
    }
    else if (wParam == VK_BACK || wParam == VK_RETURN || wParam == VK_ESCAPE ||
             wParam == VK_SPACE || wParam == VK_OEM_PERIOD ||
             wParam == VK_PRIOR || wParam == VK_NEXT ||  // PageUp/PageDown
             wParam == VK_SHIFT || wParam == VK_CONTROL)
    {
        *pfEaten = TRUE;
    }

    return S_OK;
}

HRESULT KeyHandler::OnTestKeyUp(WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_isInitialized) return S_OK;

    // Shift 键抬起时可能需要处理中英文切换
    if (wParam == VK_SHIFT && m_state.isShiftDown)
    {
        *pfEaten = TRUE;
    }

    return S_OK;
}

HRESULT KeyHandler::OnKeyDown(WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_isInitialized) return S_OK;

    // ── 更新修饰键状态 ──
    if (wParam == VK_SHIFT) m_state.isShiftDown = true;
    if (wParam == VK_CONTROL) m_state.isCtrlDown = true;
    if (wParam == VK_MENU) m_state.isAltDown = true;

    // ── 英文模式：只处理切换键 ──
    if (!m_state.isChineseMode)
    {
        if (wParam == VK_SHIFT)
        {
            // 检测是否为单独的 Shift 按下（用于切换中英文）
            // 注意：这里简化处理，实际需要检测 Shift 按下后是否有其他键
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    // ── 中文模式 ──

    // 处理 Shift+Space 全角半角切换
    if (wParam == VK_SPACE && m_state.isShiftDown)
    {
        return HandleShiftSpaceToggle(pfEaten);
    }

    // 处理 Ctrl+. 中英文标点切换
    if (wParam == VK_OEM_PERIOD && m_state.isCtrlDown)
    {
        return HandleCtrlDotToggle(pfEaten);
    }

    // 处理 Shift 切换中英文
    if (wParam == VK_SHIFT)
    {
        return HandleShiftToggle(pfEaten);
    }

    // 处理字母键（五笔编码）
    if (IsWubiCodeChar(wParam))
    {
        return HandleAlphaKey(wParam, pfEaten);
    }

    // 处理数字选词
    if (IsNumberKey(wParam) && m_compositionMgr && m_compositionMgr->IsComposing())
    {
        return HandleNumberSelection(wParam, pfEaten);
    }

    // 处理功能键
    return HandleFunctionKey(wParam, pfEaten);
}

HRESULT KeyHandler::OnKeyUp(WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_isInitialized) return S_OK;

    // 更新修饰键状态
    if (wParam == VK_SHIFT)
    {
        // 如果之前 Shift 被按下且没有其他键，则切换中英文
        if (m_state.isShiftDown && m_state.isChineseMode)
        {
            ToggleChineseMode();
            *pfEaten = TRUE;
        }
        m_state.isShiftDown = false;
    }
    if (wParam == VK_CONTROL)
    {
        m_state.isCtrlDown = false;
    }
    if (wParam == VK_MENU)
    {
        m_state.isAltDown = false;
    }

    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// State modifiers
// ═══════════════════════════════════════════════════════════════

void KeyHandler::ToggleChineseMode()
{
    m_state.isChineseMode = !m_state.isChineseMode;
}

void KeyHandler::ToggleFullWidth()
{
    m_state.isFullWidth = !m_state.isFullWidth;
}

void KeyHandler::TogglePunctuationMode()
{
    m_state.isPunctuationMode = !m_state.isPunctuationMode;
}

// ═══════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════

bool KeyHandler::IsWubiCodeChar(WPARAM vkCode) const
{
    return (vkCode >= 'A' && vkCode <= 'Z');
}

bool KeyHandler::IsNumberKey(WPARAM vkCode) const
{
    return (vkCode >= '1' && vkCode <= '9');
}

HRESULT KeyHandler::HandleAlphaKey(WPARAM wParam, BOOL* pfEaten)
{
    if (!m_compositionMgr) return E_FAIL;

    wchar_t ch = static_cast<wchar_t>(wParam);

    if (!m_compositionMgr->IsComposing())
    {
        // 开始新的组合输入
        if (m_context)
        {
            HRESULT hr = m_compositionMgr->StartComposition(m_context);
            if (FAILED(hr)) return hr;
        }
    }

    // 追加编码字符
    m_compositionMgr->AppendCode(ch);

    // 刷新候选词
    m_compositionMgr->RefreshCandidates();

    // 更新组合文本显示
    if (m_context)
    {
        m_compositionMgr->UpdateComposition(m_context, m_compositionMgr->GetCode());
    }

    *pfEaten = TRUE;
    return S_OK;
}

HRESULT KeyHandler::HandleFunctionKey(WPARAM wParam, BOOL* pfEaten)
{
    if (!m_compositionMgr) return E_FAIL;

    switch (wParam)
    {
    case VK_BACK:
        if (m_compositionMgr->IsComposing())
        {
            std::wstring code = m_compositionMgr->Backspace();
            if (code.empty())
            {
                // 编码已清空，结束组合
                if (m_context)
                {
                    m_compositionMgr->EndComposition(m_context, false);
                }
            }
            else
            {
                // 更新显示
                m_compositionMgr->RefreshCandidates();
                if (m_context)
                {
                    m_compositionMgr->UpdateComposition(m_context, code);
                }
            }
            *pfEaten = TRUE;
        }
        break;

    case VK_RETURN:
        if (m_compositionMgr->IsComposing())
        {
            if (m_compositionMgr->GetCandidates().empty())
            {
                // 无候选词，上屏原始编码
                if (m_context)
                {
                    m_compositionMgr->CommitRawCode(m_context);
                }
            }
            else
            {
                // 上屏首选
                if (m_context)
                {
                    m_compositionMgr->CommitSelected(m_context);
                }
            }
            if (m_context)
            {
                m_compositionMgr->EndComposition(m_context, true);
            }
            *pfEaten = TRUE;
        }
        break;

    case VK_ESCAPE:
        if (m_compositionMgr->IsComposing())
        {
            if (m_context)
            {
                m_compositionMgr->EndComposition(m_context, false);
            }
            *pfEaten = TRUE;
        }
        break;

    case VK_SPACE:
        if (m_compositionMgr->IsComposing())
        {
            // 空格上屏首选
            if (m_context)
            {
                m_compositionMgr->CommitSelected(m_context);
                m_compositionMgr->EndComposition(m_context, true);
            }
            *pfEaten = TRUE;
        }
        break;

    case VK_PRIOR:  // PageUp
        if (m_compositionMgr->IsComposing())
        {
            m_compositionMgr->PageCandidates(-1);
            *pfEaten = TRUE;
        }
        break;

    case VK_NEXT:  // PageDown
        if (m_compositionMgr->IsComposing())
        {
            m_compositionMgr->PageCandidates(1);
            *pfEaten = TRUE;
        }
        break;

    default:
        break;
    }

    return S_OK;
}

HRESULT KeyHandler::HandleShiftToggle(BOOL* pfEaten)
{
    // Shift 键按下时的处理
    // 注意：实际中英文切换在 OnKeyUp 中完成（检测单独的 Shift 按下）
    *pfEaten = TRUE;
    return S_OK;
}

HRESULT KeyHandler::HandleShiftSpaceToggle(BOOL* pfEaten)
{
    ToggleFullWidth();
    *pfEaten = TRUE;
    return S_OK;
}

HRESULT KeyHandler::HandleCtrlDotToggle(BOOL* pfEaten)
{
    TogglePunctuationMode();
    *pfEaten = TRUE;
    return S_OK;
}

HRESULT KeyHandler::HandleNumberSelection(WPARAM wParam, BOOL* pfEaten)
{
    if (!m_compositionMgr) return E_FAIL;

    // 数字 1-9 对应候选词 1-9
    int index = static_cast<int>(wParam) - '1';
    const auto& candidates = m_compositionMgr->GetCandidates();

    if (index >= 0 && index < static_cast<int>(candidates.size()))
    {
        m_compositionMgr->SelectCandidate(index);
        if (m_context)
        {
            m_compositionMgr->CommitSelected(m_context);
            m_compositionMgr->EndComposition(m_context, true);
        }
    }

    *pfEaten = TRUE;
    return S_OK;
}

HRESULT KeyHandler::HandlePageKey(WPARAM wParam, BOOL* pfEaten)
{
    if (!m_compositionMgr) return E_FAIL;

    int direction = (wParam == VK_NEXT) ? 1 : -1;
    m_compositionMgr->PageCandidates(direction);

    *pfEaten = TRUE;
    return S_OK;
}

wchar_t KeyHandler::GetChinesePunctuation(wchar_t ch) const
{
    // 英文标点到中文标点的映射表
    static const std::unordered_map<wchar_t, wchar_t> punctMap = {
        { L'.',  L'。' },  // 。
        { L',',  L'，' },  // ，
        { L';',  L'；' },  // ；
        { L':',  L'：' },  // ：
        { L'!',  L'！' },  // ！
        { L'?',  L'？' },  // ？
        { L'(',  L'（' },  // （
        { L')',  L'）' },  // ）
        { L'<',  L'《' },  // 《
        { L'>',  L'》' },  // 》
        { L'[',  L'【' },  // 【
        { L']',  L'】' },  // 】
        { L'{',  L'｛' },  // ｛
        { L'}',  L'｝' },  // ｝
        { L'\'', L'‘' },  // '
        { L'"',  L'“' },  // "
        { L'/',  L'／' },  // ／
        { L'\\', L'＼' },  // ＼
        { L'@',  L'＠' },  // ＠
        { L'#',  L'＃' },  // ＃
        { L'$',  L'＄' },  // ＄
        { L'%',  L'％' },  // ％
        { L'^',  L'＾' },  // ＾
        { L'&',  L'＆' },  // ＆
        { L'*',  L'＊' },  // ＊
        { L'+',  L'＋' },  // ＋
        { L'=',  L'＝' },  // ＝
        { L'_',  L'＿' },  // ＿
        { L'-',  L'－' },  // －
        { L'`',  L'―' },  // ―
        { L'~',  L'～' },  // ～
    };

    auto it = punctMap.find(ch);
    if (it != punctMap.end())
    {
        return it->second;
    }
    return ch;
}

// ── 客服场景处理实现 ──

HRESULT KeyHandler::HandleCSModeKey(WPARAM wParam, BOOL* pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    // Alt+1~9: 快速选择建议
    if (m_state.isAltDown && wParam >= '1' && wParam <= '9') {
        // TODO: 从候选栏获取对应索引的建议并发送
        *pfEaten = TRUE;
        return S_OK;
    }

    // Alt+S: 发送当前选中的建议
    if (m_state.isAltDown && (wParam == 'S' || wParam == 's')) {
        // TODO: 发送当前选中的建议到剪贴板/输入框
        *pfEaten = TRUE;
        return S_OK;
    }

    // Alt+R: 刷新建议
    if (m_state.isAltDown && (wParam == 'R' || wParam == 'r')) {
        // TODO: 触发知识库+AI建议刷新
        *pfEaten = TRUE;
        return S_OK;
    }

    return S_OK;
}

HRESULT KeyHandler::SendSuggestionToClipboard(const std::wstring& text) {
    if (text.empty()) return E_INVALIDARG;

    // 打开剪贴板
    if (!OpenClipboard(nullptr)) return E_FAIL;

    // 清空剪贴板
    EmptyClipboard();

    // 分配全局内存
    size_t size = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) {
        CloseClipboard();
        return E_FAIL;
    }

    // 复制文本到全局内存
    wchar_t* pGlobal = static_cast<wchar_t*>(GlobalLock(hGlobal));
    if (pGlobal) {
        wcscpy_s(pGlobal, text.size() + 1, text.c_str());
        GlobalUnlock(hGlobal);
    }

    // 设置剪贴板数据
    SetClipboardData(CF_UNICODETEXT, hGlobal);
    CloseClipboard();

    return S_OK;
}

HRESULT KeyHandler::TriggerSuggestions(const std::wstring& context) {
    // TODO: 调用知识库检索和AI推理生成建议
    // 1. 从上下文提取客户消息
    // 2. 调用知识库 HybridSearch
    // 3. 调用 AI GenerateCustomerServiceReply
    // 4. 将结果推送到候选栏
    return S_OK;
}

} // namespace ime
} // namespace qi
