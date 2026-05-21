#ifndef QUICKINPUT_UI_CANDIDATE_MANAGER_H_
#define QUICKINPUT_UI_CANDIDATE_MANAGER_H_

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include "quickinput/common/common.h"

namespace qi::ui {

// 候选栏样式配置
struct CandidateStyle {
    // 字体设置
    std::wstring fontFamily;        // 字体族名
    int          fontSize;          // 字号
    COLORREF     textColor;         // 文本颜色
    COLORREF     bgColor;           // 背景色
    COLORREF     selectedBgColor;   // 选中项背景色
    COLORREF     selectedTextColor; // 选中项文本色

    // 布局设置
    int          pagePadding;       // 页边距
    int          itemSpacing;       // 条目间距
    int          maxItemsPerPage;   // 每页最大条目数
    int          maxPages;          // 最大页数

    // AI/知识库标签样式
    bool         showCategoryTags;  // 显示分类标签
    std::wstring categoryTagFont;   // 标签字体
    int          tagFontSize;      // 标签字号

    // 分类背景色（用于区分不同来源）
    COLORREF     wubiBgColor;       // 五笔候选背景
    COLORREF     knowledgeBgColor;  // 知识库背景
    COLORREF     faqBgColor;       // FAQ回复背景
    COLORREF     workPhraseBgColor; // 工作话术背景
    COLORREF     quickReplyBgColor; // 快捷回复背景
    COLORREF     aiBgColor;        // AI建议背景
    COLORREF     frequentBgColor;  // 高频词背景

    // 悬停预览设置
    bool         enableHoverPreview;    // 启用悬停预览
    int          hoverPreviewDelay;     // 悬停延迟（毫秒）
    int          previewMaxWidth;       // 预览最大宽度
    int          previewPadding;        // 预览内边距
};

// 候选来源分组（用于Tab切换）
enum class CandidateSourceGroup : uint8_t {
    All = 0,          // 显示全部
    Wubi,             // 五笔候选
    Knowledge,        // 知识库相关
    AI,               // AI建议
    CustomerService,   // 客服场景
    GroupCount
};

// 候选栏状态
enum class CandidateState : uint8_t {
    Hidden = 0,     // 隐藏
    Empty,          // 空（无候选）
    Normal,         // 正常显示
    Error           // 错误状态
};

class CandidateManager {
public:
    CandidateManager();
    ~CandidateManager();

    // ── 初始化 ──
    bool Initialize(HWND hParent);
    void Shutdown();

    // ── 候选词管理 ──
    // 添加候选词（自动去重，按类别排序）
    void AddCandidates(const std::vector<CandidateItem>& items);

    // 清空候选词
    void ClearCandidates();

    // 分页控制
    void SetCurrentPage(int page);
    int GetCurrentPage() const;
    int GetTotalPages() const;

    // ── 状态管理 ──
    CandidateState GetState() const;
    void Show();
    void Hide();
    void UpdatePosition();  // 更新候选栏位置（跟随光标）

    // ── 渲染控制 ──
    void Invalidate();      // 重绘
    void Render(HDC hdc) const; // 绘制到设备上下文

    // ── 事件处理 ──
    // 鼠标消息
    bool HandleMouseMove(int x, int y);
    bool HandleLButtonDown(int x, int y);
    bool HandleLButtonUp(int x, int y);

    // 键盘消息
    bool HandleKeyDown(WORD vkCode);
    bool HandleKeyUp(WORD vkCode);
    bool HandleChar(WCHAR ch);

    // 选择候选词
    bool SelectCandidate(int index);  // 数字键选择
    bool SelectCandidateByIndex(int displayIndex); // 显示索引选择

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // 当前候选词列表（已去重、已排序）
    std::vector<CandidateItem> m_candidates;

    // 当前显示页面
    int m_currentPage;
    int m_totalPages;

    // 候选栏状态
    CandidateState m_state;

    // 样式配置
    CandidateStyle m_style;

    // 父窗口句柄
    HWND m_hParent;
    HWND m_hwndCandidate; // 候选栏窗口（如果需要独立窗口）

    // 鼠标状态
    bool m_mouseHover;
    bool m_mouseDown;

    // 当前选中的条目索引
    int m_selectedIndex;

    // ── 辅助方法 ──
    // 去重和排序
    void DeduplicateAndSort(std::vector<CandidateItem>& items);

    // 按类别排序优先级
    int GetCategoryPriority(CandidateCategory category) const;

    // 格式化候选词显示文本（添加标签）
    std::wstring FormatDisplayText(const CandidateItem& item) const;

    // 计算候选栏尺寸
    SIZE CalculateSize() const;

    // 绘制单个候选项
    void DrawCandidateItem(HDC hdc, const RECT& rcItem,
                          const CandidateItem& item, bool isSelected) const;

    // 绘制分类标签
    void DrawCategoryTag(HDC hdc, const RECT& rcTag,
                        CandidateCategory category) const;
};

} // namespace qi::ui

#endif // QUICKINPUT_UI_CANDIDATE_MANAGER_H_