#include "quickinput/ime/text_service.h"
#include "quickinput/ime/key_handler.h"
#include "quickinput/ime/composition.h"

#include <msctf.h>
#include <ctffunc.h>
#include <tchar.h>

namespace qi {
namespace ime {

// ═══════════════════════════════════════════════════════════════
// Construction / Destruction
// ═══════════════════════════════════════════════════════════════

TextService::TextService()
    : m_refCount(1)
    , m_threadMgr(nullptr)
    , m_clientId(TF_CLIENTID_NULL)
    , m_context(nullptr)
    , m_composition(nullptr)
    , m_isActivated(FALSE)
    , m_isThreadFocused(FALSE)
    , m_isComposing(FALSE)
{
}

TextService::~TextService()
{
    // 确保资源已释放
    (void)Deactivate();
}

// ═══════════════════════════════════════════════════════════════
// IUnknown
// ═══════════════════════════════════════════════════════════════

HRESULT STDMETHODCALLTYPE TextService::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) return E_INVALIDARG;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, __uuidof(IUnknown)) ||
        IsEqualIID(riid, __uuidof(ITfTextInputProcessor)) ||
        IsEqualIID(riid, __uuidof(ITfTextInputProcessorEx)))
    {
        *ppvObject = static_cast<ITfTextInputProcessorEx*>(this);
    }
    else if (IsEqualIID(riid, __uuidof(ITfKeyEventSink)))
    {
        *ppvObject = static_cast<ITfKeyEventSink*>(this);
    }
    else if (IsEqualIID(riid, __uuidof(ITfCompositionSink)))
    {
        *ppvObject = static_cast<ITfCompositionSink*>(this);
    }
    else if (IsEqualIID(riid, __uuidof(ITfDisplayAttributeProvider)))
    {
        *ppvObject = static_cast<ITfDisplayAttributeProvider*>(this);
    }

    if (*ppvObject)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE TextService::AddRef()
{
    return ::InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE TextService::Release()
{
    LONG ref = ::InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        delete this;
    }
    return static_cast<ULONG>(ref);
}

// ═══════════════════════════════════════════════════════════════
// ITfTextInputProcessor
// ═══════════════════════════════════════════════════════════════

HRESULT STDMETHODCALLTYPE TextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId)
{
    // 默认调用 ActivateEx，无额外标志
    return ActivateEx(pThreadMgr, tfClientId, 0);
}

HRESULT STDMETHODCALLTYPE TextService::Deactivate()
{
    // 停用子模块
    if (m_compositionManager)
    {
        if (m_context)
        {
            m_compositionManager->EndComposition(m_context, false);
        }
        m_compositionManager->Uninitialize();
        m_compositionManager.reset();
    }

    m_keyHandler.reset();

    // 释放 TSF 对象
    ReleaseComposition();
    ReleaseContext();

    if (m_threadMgr)
    {
        m_threadMgr->Release();
        m_threadMgr = nullptr;
    }

    m_clientId = TF_CLIENTID_NULL;
    m_isActivated = FALSE;
    m_isThreadFocused = FALSE;

    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// ITfTextInputProcessorEx
// ═══════════════════════════════════════════════════════════════

HRESULT STDMETHODCALLTYPE TextService::ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags)
{
    if (!pThreadMgr) return E_INVALIDARG;

    // 保存线程管理器和客户端 ID
    m_threadMgr = pThreadMgr;
    m_threadMgr->AddRef();
    m_clientId = tfClientId;
    m_isActivated = TRUE;

    // 创建子模块
    m_compositionManager = std::make_unique<CompositionManager>();
    m_keyHandler = std::make_unique<KeyHandler>();

    // 注意：此时还没有 ITfContext，需要在 OnSetFocus 或按键事件时获取
    // KeyHandler 和 CompositionManager 的 Initialize 需要延迟到有 Context 时

    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// ITfKeyEventSink
// ═══════════════════════════════════════════════════════════════

HRESULT STDMETHODCALLTYPE TextService::OnSetFocus(BOOL fForeground)
{
    m_isThreadFocused = fForeground;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE TextService::OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_keyHandler || !m_isActivated) return S_OK;

    // 确保上下文可用
    if (pContext && !m_context)
    {
        m_context = pContext;
        m_context->AddRef();
    }

    return m_keyHandler->OnTestKeyDown(wParam, lParam, pfEaten);
}

HRESULT STDMETHODCALLTYPE TextService::OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_keyHandler || !m_isActivated) return S_OK;

    return m_keyHandler->OnTestKeyUp(wParam, lParam, pfEaten);
}

HRESULT STDMETHODCALLTYPE TextService::OnKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_isActivated) return S_OK;

    // 确保上下文可用
    if (pContext && !m_context)
    {
        m_context = pContext;
        m_context->AddRef();

        // 延迟初始化子模块
        if (m_compositionManager && !m_compositionManager->IsComposing())
        {
            m_compositionManager->Initialize(m_threadMgr, m_clientId, nullptr);
        }
        if (m_keyHandler)
        {
            m_keyHandler->Initialize(m_context, m_compositionManager.get());
        }
    }

    if (m_keyHandler)
    {
        return m_keyHandler->OnKeyDown(wParam, lParam, pfEaten);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TextService::OnKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_keyHandler || !m_isActivated) return S_OK;

    return m_keyHandler->OnKeyUp(wParam, lParam, pfEaten);
}

HRESULT STDMETHODCALLTYPE TextService::OnPreservedKey(ITfContext* pContext, REFGUID rguid, BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    // 保留键暂不处理
    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// ITfCompositionSink
// ═══════════════════════════════════════════════════════════════

HRESULT STDMETHODCALLTYPE TextService::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition)
{
    if (!m_compositionManager) return S_OK;

    // 通知组合管理器终止组合
    if (m_context)
    {
        m_compositionManager->TerminateComposition(m_context);
    }

    m_isComposing = FALSE;
    ReleaseComposition();

    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// ITfDisplayAttributeProvider
// ═══════════════════════════════════════════════════════════════

HRESULT STDMETHODCALLTYPE TextService::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum)
{
    if (!ppEnum) return E_INVALIDARG;

    // 创建显示属性枚举器（简化实现，返回单个属性）
    // 实际项目中应实现完整的 IEnumTfDisplayAttributeInfo
    *ppEnum = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextService::GetDisplayAttributeInfo(REFGUID guidInfo, ITfDisplayAttributeInfo** ppInfo)
{
    if (!ppInfo) return E_INVALIDARG;
    *ppInfo = nullptr;

    // 返回 QuickInput 的显示属性
    if (IsEqualGUID(guidInfo, QI_GUID_DISPLAY_ATTR))
    {
        // 实际项目中应创建并返回 ITfDisplayAttributeInfo 实例
        return E_NOTIMPL;
    }

    return TF_E_DISPLAYATTRIBUTENOTPROVIDED;
}

// ═══════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════

HRESULT TextService::EnsureContext()
{
    if (m_context) return S_OK;

    if (!m_threadMgr) return E_FAIL;

    ITfContext* pContext = nullptr;
    HRESULT hr = m_threadMgr->GetFocus(&pContext);
    if (SUCCEEDED(hr) && pContext)
    {
        m_context = pContext;
        // 注意：GetFocus 返回的对象引用计数已 +1，不需要再 AddRef
    }

    return hr;
}

HRESULT TextService::ReleaseContext()
{
    if (m_context)
    {
        m_context->Release();
        m_context = nullptr;
    }
    return S_OK;
}

HRESULT TextService::ReleaseComposition()
{
    if (m_composition)
    {
        m_composition->Release();
        m_composition = nullptr;
    }
    return S_OK;
}

} // namespace ime
} // namespace qi
