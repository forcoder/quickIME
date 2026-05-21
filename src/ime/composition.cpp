#include "quickinput/ime/composition.h"

#include <msctf.h>
#include <ctffunc.h>
#include <tchar.h>

namespace qi {
namespace ime {

// ═══════════════════════════════════════════════════════════════
// Construction / Destruction
// ═══════════════════════════════════════════════════════════════

CompositionManager::CompositionManager()
    : m_threadMgr(nullptr)
    , m_clientId(TF_CLIENTID_NULL)
    , m_context(nullptr)
    , m_composition(nullptr)
    , m_displayAttrInfo(nullptr)
    , m_isComposing(false)
    , m_isInitialized(false)
    , m_selectedIndex(0)
    , m_pageIndex(0)
    , m_wubiEngine(nullptr)
{
}

CompositionManager::~CompositionManager()
{
    Uninitialize();
}

// ═══════════════════════════════════════════════════════════════
// Initialization
// ═══════════════════════════════════════════════════════════════

HRESULT CompositionManager::Initialize(ITfThreadMgr* pThreadMgr, TfClientId clientId, WubiEngine* pWubiEngine)
{
    m_threadMgr = pThreadMgr;
    m_clientId = clientId;
    m_wubiEngine = pWubiEngine;
    m_isInitialized = true;
    return S_OK;
}

void CompositionManager::Uninitialize()
{
    m_isComposing = false;
    m_isInitialized = false;
    m_code.clear();
    m_candidates.clear();
    m_selectedIndex = 0;
    m_pageIndex = 0;
    m_context = nullptr;
    m_composition = nullptr;
    m_displayAttrInfo = nullptr;
    m_threadMgr = nullptr;
    m_wubiEngine = nullptr;
}

// ═══════════════════════════════════════════════════════════════
// Composition lifecycle
// ═══════════════════════════════════════════════════════════════

HRESULT CompositionManager::StartComposition(ITfContext* pContext)
{
    if (!pContext || !m_threadMgr) return E_INVALIDARG;
    if (m_isComposing) return S_OK;  // 已经在组合中

    m_context = pContext;

    // 获取编辑会话
    ITfEditSession* pEditSession = nullptr;

    // 通过 ITfContext 创建组合
    HRESULT hr = m_context->RequestEditSession(
        m_clientId,
        nullptr,  // 使用默认编辑会话
        TF_ES_SYNC | TF_ES_READWRITE,
        &hr
    );

    if (FAILED(hr))
    {
        // 如果 RequestEditSession 失败，尝试直接创建
        // 注意：实际 TSF 中需要在编辑会话内操作
    }

    // 使用 ITfContextComposition 接口创建组合
    ITfContextComposition* pContextComposition = nullptr;
    hr = m_context->QueryInterface(IID_ITfContextComposition,
                                    reinterpret_cast<void**>(&pContextComposition));
    if (FAILED(hr)) return hr;

    ITfComposition* pComposition = nullptr;
    // 创建组合，范围从当前光标位置开始
    ITfRange* pRange = nullptr;

    // 获取当前选区/光标位置
    ITfSelection* pSelection = nullptr;
    TfActiveSelEnd selEnd = TF_AE_NONE;
    ULONG fetched = 0;

    hr = m_context->GetSelection(m_clientId, TF_DEFAULT_EDIT_COOKIE, 1,
                                  &pSelection, &selEnd);
    if (SUCCEEDED(hr) && pSelection)
    {
        hr = pSelection->GetText(m_clientId, TF_DEFAULT_EDIT_COOKIE,
                                  0, nullptr, 0, &fetched);
        pSelection->Release();
    }

    // 创建组合
    hr = pContextComposition->StartComposition(
        m_clientId,
        pRange,
        this,  // ITfCompositionSink
        &pComposition
    );

    pContextComposition->Release();

    if (SUCCEEDED(hr) && pComposition)
    {
        m_composition = pComposition;
        m_isComposing = true;
        m_code.clear();
        m_candidates.clear();
        m_selectedIndex = 0;
        m_pageIndex = 0;
    }

    return hr;
}

HRESULT CompositionManager::EndComposition(ITfContext* pContext, bool commitText)
{
    if (!m_isComposing || !m_composition) return S_OK;

    if (commitText && !m_code.empty())
    {
        // 提交文本
        CommitString(pContext, m_code);
    }

    // 结束组合
    if (m_composition)
    {
        // 在编辑会话内结束组合
        m_composition->EndComposition(TF_DEFAULT_EDIT_COOKIE);
        m_composition->Release();
        m_composition = nullptr;
    }

    m_isComposing = false;
    m_code.clear();
    m_candidates.clear();
    m_selectedIndex = 0;
    m_pageIndex = 0;

    return S_OK;
}

HRESULT CompositionManager::UpdateComposition(ITfContext* pContext, const std::wstring& newComposition)
{
    if (!m_isComposing || !m_composition) return E_FAIL;

    m_code = newComposition;

    // 更新组合文本显示
    HRESULT hr = SetCompositionString(pContext, newComposition);
    if (FAILED(hr)) return hr;

    // 设置显示属性（下划线）
    hr = SetCompositionDisplayAttributes(pContext);

    return hr;
}

HRESULT CompositionManager::TerminateComposition(ITfContext* pContext)
{
    if (!m_isComposing) return S_OK;

    // 终止组合（不提交文本）
    if (m_composition)
    {
        m_composition->EndComposition(TF_DEFAULT_EDIT_COOKIE);
        m_composition->Release();
        m_composition = nullptr;
    }

    m_isComposing = false;
    m_code.clear();
    m_candidates.clear();
    m_selectedIndex = 0;
    m_pageIndex = 0;

    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// Code management
// ═══════════════════════════════════════════════════════════════

void CompositionManager::AppendCode(wchar_t ch)
{
    // 五笔编码最多 4 个字母
    if (m_code.length() < 4)
    {
        m_code += ch;
    }
}

std::wstring CompositionManager::Backspace()
{
    if (!m_code.empty())
    {
        m_code.pop_back();
    }
    return m_code;
}

void CompositionManager::ClearCode()
{
    m_code.clear();
    m_candidates.clear();
    m_selectedIndex = 0;
    m_pageIndex = 0;
}

// ═══════════════════════════════════════════════════════════════
// Candidate management
// ═══════════════════════════════════════════════════════════════

std::wstring CompositionManager::SelectCandidate(int index)
{
    if (index >= 0 && index < static_cast<int>(m_candidates.size()))
    {
        m_selectedIndex = index;
        return m_candidates[index].text;
    }
    return std::wstring();
}

bool CompositionManager::PageCandidates(int direction)
{
    int newPage = m_pageIndex + direction;
    if (newPage < 0) return false;

    int maxPage = static_cast<int>(m_candidates.size()) / CANDIDATES_PER_PAGE;
    if (newPage > maxPage) return false;

    m_pageIndex = newPage;
    return true;
}

bool CompositionManager::RefreshCandidates()
{
    m_candidates.clear();
    m_selectedIndex = 0;
    m_pageIndex = 0;

    if (m_code.empty()) return false;

    // 如果有五笔引擎，使用引擎查询
    if (m_wubiEngine)
    {
        // TODO: 调用 m_wubiEngine->Lookup(m_code, m_candidates)
        // 暂时返回空，等待引擎模块实现
        return !m_candidates.empty();
    }

    // 无引擎时返回空
    return false;
}

// ═══════════════════════════════════════════════════════════════
// Commit
// ═══════════════════════════════════════════════════════════════

std::wstring CompositionManager::CommitSelected(ITfContext* pContext)
{
    std::wstring text;

    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_candidates.size()))
    {
        text = m_candidates[m_selectedIndex].text;
    }
    else if (!m_candidates.empty())
    {
        text = m_candidates[0].text;
    }
    else
    {
        text = m_code;
    }

    if (!text.empty())
    {
        CommitString(pContext, text);
    }

    return text;
}

std::wstring CompositionManager::CommitRawCode(ITfContext* pContext)
{
    if (!m_code.empty())
    {
        CommitString(pContext, m_code);
    }
    return m_code;
}

// ═══════════════════════════════════════════════════════════════
// Display
// ═══════════════════════════════════════════════════════════════

HRESULT CompositionManager::SetCompositionDisplayAttributes(ITfContext* pContext)
{
    if (!pContext || !m_composition) return E_FAIL;

    // 获取组合范围
    ITfRange* pRange = nullptr;
    HRESULT hr = m_composition->GetRange(&pRange);
    if (FAILED(hr) || !pRange) return hr;

    // 设置显示属性
    ITfProperty* pProperty = nullptr;
    hr = pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProperty);
    if (SUCCEEDED(hr) && pProperty)
    {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = static_cast<LONG>(m_displayAttrInfo != nullptr ? 1 : 0);

        pProperty->SetValue(m_clientId, pRange, &var);
        pProperty->Release();
    }

    pRange->Release();
    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// Private helpers
// ═══════════════════════════════════════════════════════════════

HRESULT CompositionManager::SetCompositionString(ITfContext* pContext, const std::wstring& text)
{
    if (!pContext || !m_composition) return E_FAIL;

    // 获取组合范围
    ITfRange* pRange = nullptr;
    HRESULT hr = m_composition->GetRange(&pRange);
    if (FAILED(hr) || !pRange) return hr;

    // 在编辑会话内设置文本
    // 注意：实际 TSF 中需要在 RequestEditSession 回调内执行
    // 这里简化处理
    hr = pRange->SetText(m_clientId, 0, text.c_str(), static_cast<LONG>(text.length()));

    pRange->Release();
    return hr;
}

HRESULT CompositionManager::CommitString(ITfContext* pContext, const std::wstring& text)
{
    if (!pContext || text.empty()) return E_INVALIDARG;

    // 获取当前选区
    ITfInsertAtSelection* pInsertAtSelection = nullptr;
    HRESULT hr = pContext->QueryInterface(IID_ITfInsertAtSelection,
                                            reinterpret_cast<void**>(&pInsertAtSelection));
    if (FAILED(hr)) return hr;

    ITfRange* pRange = nullptr;
    hr = pInsertAtSelection->InsertTextAtSelection(
        m_clientId,
        TF_IAS_QUERYONLY,
        nullptr,
        0,
        &pRange
    );
    pInsertAtSelection->Release();

    if (FAILED(hr) || !pRange) return hr;

    // 在编辑会话内插入文本
    hr = pRange->SetText(m_clientId, 0, text.c_str(), static_cast<LONG>(text.length()));

    // 将光标移到插入文本之后
    ITfContextView* pContextView = nullptr;
    if (SUCCEEDED(pContext->GetActiveView(&pContextView)))
    {
        // 更新光标位置
        pContextView->Release();
    }

    pRange->Release();
    return hr;
}

} // namespace ime
} // namespace qi
