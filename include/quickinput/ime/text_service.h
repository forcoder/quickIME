#pragma once

#include "quickinput/core/common.h"

#include <msctf.h>
#include <ctffunc.h>
#include <atlbase.h>

#include <memory>

// 前向声明
namespace qi {
    class KeyHandler;
    class CompositionManager;
    class CandidateWindow;
    class WubiEngine;
    class Config;
}

namespace qi {
namespace ime {

/**
 * @class TextService
 * @brief QuickInput 输入法文字服务的核心类
 *
 * 实现 TSF(Text Services Framework) 所需的 COM 接口：
 * - ITfTextInputProcessorEx: 输入法生命周期管理
 * - ITfKeyEventSink: 键盘事件处理
 * - ITfCompositionSink: 组合文本事件
 * - ITfDisplayAttributeProvider: 显示属性（下划线样式）
 *
 * 每个线程/文档对应一个 TextService 实例，由 TSF 管理器通过
 * ITfTextInputProcessorEx::Activate() 激活。
 */
class TextService : public ITfTextInputProcessorEx,
                    public ITfKeyEventSink,
                    public ITfCompositionSink,
                    public ITfDisplayAttributeProvider
{
public:
    // ── Construction / Destruction ──

    TextService();
    ~TextService();

    // ── IUnknown ──
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG   STDMETHODCALLTYPE AddRef() override;
    ULONG   STDMETHODCALLTYPE Release() override;

    // ── ITfTextInputProcessor ──
    HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) override;
    HRESULT STDMETHODCALLTYPE Deactivate() override;

    // ── ITfTextInputProcessorEx ──
    HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags) override;

    // ── ITfKeyEventSink ──
    HRESULT STDMETHODCALLTYPE OnSetFocus(BOOL fForeground) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    HRESULT STDMETHODCALLTYPE OnKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    HRESULT STDMETHODCALLTYPE OnKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    HRESULT STDMETHODCALLTYPE OnPreservedKey(ITfContext* pContext, REFGUID rguid, BOOL* pfEaten) override;

    // ── ITfCompositionSink ──
    HRESULT STDMETHODCALLTYPE OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) override;

    // ── ITfDisplayAttributeProvider ──
    HRESULT STDMETHODCALLTYPE EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) override;
    HRESULT STDMETHODCALLTYPE GetDisplayAttributeInfo(REFGUID guidInfo, ITfDisplayAttributeInfo** ppInfo) override;

    // ── Accessors ──
    TfClientId              GetClientId() const { return m_clientId; }
    ITfThreadMgr*           GetThreadMgr() const { return m_threadMgr; }
    ITfContext*             GetContext() const { return m_context; }
    KeyHandler*             GetKeyHandler() const { return m_keyHandler.get(); }
    CompositionManager*     GetCompositionManager() const { return m_compositionManager.get(); }
    BOOL                    IsActivated() const { return m_isActivated; }

private:
    // ── Internal helpers ──
    HRESULT EnsureContext();
    HRESULT ReleaseContext();
    HRESULT ReleaseComposition();

    // ── COM reference count ──
    LONG                    m_refCount;

    // ── TSF core objects ──
    ITfThreadMgr*           m_threadMgr;        ///< 线程管理器
    TfClientId              m_clientId;         ///< 客户端 ID（标识此输入法）
    ITfContext*             m_context;          ///< 当前编辑上下文
    ITfComposition*         m_composition;      ///< 当前组合文本对象

    // ── Activation state ──
    BOOL                    m_isActivated;      ///< 是否已激活
    BOOL                    m_isThreadFocused;  ///< 线程是否有焦点
    BOOL                    m_isComposing;      ///< 是否处于组合输入状态

    // ── Sub-modules ──
    std::unique_ptr<KeyHandler>         m_keyHandler;
    std::unique_ptr<CompositionManager> m_compositionManager;
};

} // namespace ime
} // namespace qi
