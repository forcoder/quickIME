#pragma once

#include "quickinput/core/common.h"

#include <msctf.h>
#include <ctffunc.h>
#include <atlbase.h>

#include <string>
#include <vector>
#include <memory>

// 前向声明
namespace qi {
    class WubiEngine;
    class CandidateWindow;
}

namespace qi {
namespace ime {

/**
 * @class CompositionManager
 * @brief 组合文本管理器
 *
 * 管理当前输入的五笔编码串，负责：
 * - 与 TSF ITfComposition 交互
 * - 设置组合文本的显示（下划线样式）
 * - 管理候选栏的显示/隐藏
 * - 处理编码到候选词的转换流程
 * - 上屏文本（提交到编辑器）
 */
class CompositionManager
{
public:
    CompositionManager();
    ~CompositionManager();

    /**
     * @brief 初始化管理器
     * @param pThreadMgr 线程管理器
     * @param clientId 客户端 ID
     * @param pWubiEngine 五笔引擎指针（非拥有）
     * @return HRESULT
     */
    HRESULT Initialize(ITfThreadMgr* pThreadMgr, TfClientId clientId, WubiEngine* pWubiEngine);

    /**
     * @brief 清理资源
     */
    void Uninitialize();

    // ── Composition lifecycle ──

    /**
     * @brief 开始新的组合输入
     * @param pContext TSF 编辑上下文
     * @return HRESULT
     */
    HRESULT StartComposition(ITfContext* pContext);

    /**
     * @brief 结束组合输入（上屏或取消）
     * @param pContext TSF 编辑上下文
     * @param commitText 是否提交文本（true=上屏，false=取消）
     * @return HRESULT
     */
    HRESULT EndComposition(ITfContext* pContext, bool commitText);

    /**
     * @brief 更新组合文本（用户输入新编码时调用）
     * @param pContext TSF 编辑上下文
     * @param newComposition 新的编码串
     * @return HRESULT
     */
    HRESULT UpdateComposition(ITfContext* pContext, const std::wstring& newComposition);

    /**
     * @brief 终止组合输入（由 TSF 调用）
     * @param pContext TSF 编辑上下文
     * @return HRESULT
     */
    HRESULT TerminateComposition(ITfContext* pContext);

    // ── Code management ──

    /**
     * @brief 追加编码字符
     * @param ch 要追加的字符（a-z）
     */
    void AppendCode(wchar_t ch);

    /**
     * @brief 删除最后一个编码字符
     * @return 删除后的编码串
     */
    std::wstring Backspace();

    /**
     * @brief 清空编码
     */
    void ClearCode();

    /**
     * @brief 获取当前编码串
     */
    const std::wstring& GetCode() const { return m_code; }

    /**
     * @brief 是否处于组合输入状态
     */
    bool IsComposing() const { return m_isComposing; }

    // ── Candidate management ──

    /**
     * @brief 获取当前候选词列表
     */
    const std::vector<CandidateItem>& GetCandidates() const { return m_candidates; }

    /**
     * @brief 获取当前选中的候选词索引
     */
    int GetSelectedIndex() const { return m_selectedIndex; }

    /**
     * @brief 选择候选词
     * @param index 候选词索引
     * @return 选中的候选词文本
     */
    std::wstring SelectCandidate(int index);

    /**
     * @brief 翻页候选词列表
     * @param direction 翻页方向（+1=下一页，-1=上一页）
     * @return 是否成功翻页
     */
    bool PageCandidates(int direction);

    /**
     * @brief 刷新候选词列表（编码改变后调用）
     * @return 是否有候选词
     */
    bool RefreshCandidates();

    // ── Commit ──

    /**
     * @brief 上屏当前选中的候选词
     * @param pContext TSF 编辑上下文
     * @return 上屏的文本
     */
    std::wstring CommitSelected(ITfContext* pContext);

    /**
     * @brief 上屏原始编码（无匹配候选词时）
     * @param pContext TSF 编辑上下文
     * @return 上屏的文本
     */
    std::wstring CommitRawCode(ITfContext* pContext);

    // ── Display ──

    /**
     * @brief 设置组合文本的显示属性（下划线）
     * @param pContext TSF 编辑上下文
     * @return HRESULT
     */
    HRESULT SetCompositionDisplayAttributes(ITfContext* pContext);

    /**
     * @brief 获取关联的 ITfComposition 对象
     */
    ITfComposition* GetComposition() const { return m_composition; }

private:
    /**
     * @brief 设置组合文本内容到 TSF
     * @param pContext TSF 编辑上下文
     * @param text 要显示的文本
     * @return HRESULT
     */
    HRESULT SetCompositionString(ITfContext* pContext, const std::wstring& text);

    /**
     * @brief 提交文本到编辑器
     * @param pContext TSF 编辑上下文
     * @param text 要提交的文本
     * @return HRESULT
     */
    HRESULT CommitString(ITfContext* pContext, const std::wstring& text);

    // ── TSF core ──
    ITfThreadMgr*           m_threadMgr;        ///< 线程管理器
    TfClientId              m_clientId;         ///< 客户端 ID
    ITfContext*             m_context;          ///< 当前编辑上下文
    ITfComposition*         m_composition;      ///< TSF 组合文本对象
    ITfDisplayAttributeInfo* m_displayAttrInfo; ///< 显示属性信息

    // ── Composition state ──
    bool                    m_isComposing;      ///< 是否处于组合输入状态
    bool                    m_isInitialized;    ///< 是否已初始化

    // ── Code & candidates ──
    std::wstring            m_code;             ///< 当前五笔编码串
    std::vector<CandidateItem> m_candidates;    ///< 当前候选词列表
    int                     m_selectedIndex;    ///< 当前选中的候选词索引
    int                     m_pageIndex;        ///< 当前页码（0-based）
    static constexpr int    CANDIDATES_PER_PAGE = 9;  ///< 每页候选词数量

    // ── Engine ──
    WubiEngine*             m_wubiEngine;       ///< 五笔引擎（非拥有）
};

} // namespace ime
} // namespace qi
