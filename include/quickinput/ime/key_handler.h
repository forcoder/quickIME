#pragma once

#include "quickinput/core/common.h"

#include <msctf.h>

namespace qi {
namespace ime {

// 前向声明
class CompositionManager;
class CandidateWindow;
class WubiEngine;

/**
 * @enum KeyAction
 * @brief 按键处理结果枚举
 */
enum class KeyAction : uint8_t {
    PassThrough,    ///< 不处理，传递给系统
    Handled,        ///< 已处理，不传递
    StartComposition, ///< 开始组合输入
    UpdateComposition, ///< 更新组合文本
    CommitCandidate,   ///< 上屏候选词
    CancelComposition, ///< 取消组合输入
    PageCandidates,    ///< 翻页候选词
    DeleteCode         ///< 删除编码字符
};

/**
 * @struct KeyState
 * @brief 当前输入状态
 */
struct KeyState {
    bool isChineseMode;     ///< 中文/英文模式
    bool isFullWidth;       ///< 全角/半角模式
    bool isPunctuationMode; ///< 中文/英文标点模式
    bool isShiftDown;       ///< Shift 键是否按下
    bool isCtrlDown;        ///< Ctrl 键是否按下

    KeyState()
        : isChineseMode(true)
        , isFullWidth(false)
        , isPunctuationMode(true)
        , isShiftDown(false)
        , isCtrlDown(false)
    {}
};

/**
 * @class KeyHandler
 * @brief 按键处理器
 *
 * 负责处理所有键盘输入，包括：
 * - 中英文切换（Shift 键）
 * - 全角半角切换（Shift+Space）
 * - 中英文标点切换（Ctrl+.）
 * - 数字选词、翻页（PageUp/PageDown）
 * - 退格删除编码
 * - 回车清屏/上屏
 * - 空格上屏首选
 * - Esc 取消输入
 */
class KeyHandler
{
public:
    KeyHandler();
    ~KeyHandler();

    /**
     * @brief 初始化按键处理器
     * @param pContext TSF 编辑上下文
     * @param pCompositionMgr 组合管理器指针
     * @return HRESULT
     */
    HRESULT Initialize(ITfContext* pContext, CompositionManager* pCompositionMgr);

    /**
     * @brief 处理按键按下事件（测试阶段）
     * @param wParam 虚拟键码
     * @param lParam 按键参数
     * @param pfEaten 输出：是否被输入法处理
     * @return HRESULT
     */
    HRESULT OnTestKeyDown(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);

    /**
     * @brief 处理按键按下事件
     * @param wParam 虚拟键码
     * @param lParam 按键参数
     * @param pfEaten 输出：是否被输入法处理
     * @return HRESULT
     */
    HRESULT OnKeyDown(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);

    /**
     * @brief 处理按键弹起事件
     * @param wParam 虚拟键码
     * @param lParam 按键参数
     * @param pfEaten 输出：是否被输入法处理
     * @return HRESULT
     */
    HRESULT OnKeyUp(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);

    /**
     * @brief 处理按键弹起事件（测试阶段）
     */
    HRESULT OnTestKeyUp(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);

    // ── State accessors ──
    const KeyState& GetState() const { return m_state; }
    bool IsChineseMode() const { return m_state.isChineseMode; }
    bool IsFullWidth() const { return m_state.isFullWidth; }
    bool IsPunctuationMode() const { return m_state.isPunctuationMode; }

    // ── State modifiers ──
    void ToggleChineseMode();
    void ToggleFullWidth();
    void TogglePunctuationMode();

private:
    /**
     * @brief 判断是否为五笔编码字符（a-z）
     */
    bool IsWubiCodeChar(WPARAM vkCode) const;

    /**
     * @brief 判断是否为数字键（0-9，用于选词）
     */
    bool IsNumberKey(WPARAM vkCode) const;

    /**
     * @brief 处理字母键输入（五笔编码）
     */
    HRESULT HandleAlphaKey(WPARAM wParam, BOOL* pfEaten);

    /**
     * @brief 处理功能键（Backspace, Enter, Esc, Space 等）
     */
    HRESULT HandleFunctionKey(WPARAM wParam, BOOL* pfEaten);

    /**
     * @brief 处理 Shift 切换中英文
     */
    HRESULT HandleShiftToggle(BOOL* pfEaten);

    /**
     * @brief 处理 Shift+Space 切换全角半角
     */
    HRESULT HandleShiftSpaceToggle(BOOL* pfEaten);

    /**
     * @brief 处理 Ctrl+. 切换中英文标点
     */
    HRESULT HandleCtrlDotToggle(BOOL* pfEaten);

    /**
     * @brief 处理数字选词
     */
    HRESULT HandleNumberSelection(WPARAM wParam, BOOL* pfEaten);

    /**
     * @brief 处理翻页键
     */
    HRESULT HandlePageKey(WPARAM wParam, BOOL* pfEaten);

    /**
     * @brief 获取中文标点映射
     * @param ch 英文标点字符
     * @return 对应的中文标点字符，无映射则返回原字符
     */
    wchar_t GetChinesePunctuation(wchar_t ch) const;

    // ── Members ──
    KeyState                m_state;            ///< 当前输入状态
    ITfContext*             m_context;          ///< TSF 编辑上下文
    CompositionManager*     m_compositionMgr;   ///< 组合管理器（非拥有）
    bool                    m_isInitialized;    ///< 是否已初始化
};

} // namespace ime
} // namespace qi
