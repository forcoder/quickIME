#include "quickinput/ui/candidate_manager.h"
#include <algorithm>
#include <unordered_set>
#include <gdiplus.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std;

namespace qi::ui {

// 候选词去重比较器
struct CandidateItemEqual {
    bool operator()(const CandidateItem& a, const CandidateItem& b) const {
        return a.text == b.text && a.category == b.category;
    }
};

// 候选词排序比较器（按类别优先级排序）
struct CandidateItemComparator {
    int GetCategoryPriority(CandidateCategory category) const {
        switch (category) {
            case CandidateCategory::Word:       return 100; // 普通词汇
            case CandidateCategory::AI:         return 90;  // AI生成
            case CandidateCategory::Knowledge: return 80;  // 知识库
            case CandidateCategory::Frequent:   return 70;  // 高频词
            case CandidateCategory::Recent:     return 60;  // 最近使用
            default:                            return 50;  // 其他
        }
    }

    bool operator()(const CandidateItem& a, const CandidateItem& b) const {
        int priorityA = GetCategoryPriority(a.category);
        int priorityB = GetCategoryPriority(b.category);

        if (priorityA != priorityB) {
            return priorityA > priorityB; // 优先级高的在前
        }

        // 同优先级下，按频率降序排列
        if (a.frequency != b.frequency) {
            return a.frequency > b.frequency;
        }

        // 最后按文本排序
        return a.text < b.text;
    }
};

CandidateManager::CandidateManager()
    : m_gdiplusToken(0)
    , m_currentPage(0)
    , m_totalPages(0)
    , m_state(CandidateState::Hidden)
    , m_style{}
    , m_hParent(nullptr)
    , m_hwndCandidate(nullptr)
    , m_mouseHover(false)
    , m_mouseDown(false)
    , m_selectedIndex(-1) {
}

CandidateManager::~CandidateManager() {
    Shutdown();
}

bool CandidateManager::Initialize(HWND hParent) {
    m_hParent = hParent;

    // 设置默认样式
    m_style.fontFamily = L"微软雅黑";
    m_style.fontSize = 12;
    m_style.textColor = RGB(0, 0, 0);
    m_style.bgColor = RGB(255, 255, 255);
    m_style.selectedBgColor = RGB(200, 230, 255);
    m_style.selectedTextColor = RGB(0, 0, 0);
    m_style.pagePadding = 5;
    m_style.itemSpacing = 2;
    m_style.maxItemsPerPage = 9;  // 每页9个候选
    m_style.maxPages = 5;
    m_style.showCategoryTags = true;
    m_style.categoryTagFont = L"Arial";
    m_style.tagFontSize = 9;

    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

    return true;
}

void CandidateManager::Shutdown() {
    if (m_gdiplusToken != 0) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
}

void CandidateManager::AddCandidates(const std::vector<CandidateItem>& items) {
    if (items.empty()) {
        ClearCandidates();
        return;
    }

    // 复制并去重排序
    std::vector<CandidateItem> newCandidates = items;
    DeduplicateAndSort(newCandidates);

    m_candidates = std::move(newCandidates);

    // 更新分页信息
    UpdatePagination();

    // 显示候选栏
    m_state = CandidateState::Normal;
}

void CandidateManager::ClearCandidates() {
    m_candidates.clear();
    m_currentPage = 0;
    m_totalPages = 0;
    m_selectedIndex = -1;
    m_state = CandidateState::Empty;
}

void CandidateManager::SetCurrentPage(int page) {
    if (page >= 0 && page < m_totalPages) {
        m_currentPage = page;
    }
}

int CandidateManager::GetCurrentPage() const {
    return m_currentPage;
}

int CandidateManager::GetTotalPages() const {
    return m_totalPages;
}

CandidateState CandidateManager::GetState() const {
    return m_state;
}

void CandidateManager::Show() {
    if (m_state != CandidateState::Hidden) {
        Invalidate();
    }
    m_state = CandidateState::Normal;
}

void CandidateManager::Hide() {
    m_state = CandidateState::Hidden;
    m_selectedIndex = -1;
}

void CandidateManager::UpdatePosition() {
    // 获取当前光标位置
    POINT ptCursor;
    if (GetCursorPos(&ptCursor)) {
        // 将光标位置转换为屏幕坐标
        RECT rcScreen;
        rcScreen.left = ptCursor.x;
        rcScreen.top = ptCursor.y;
        rcScreen.right = ptCursor.x + 1;
        rcScreen.bottom = ptCursor.y + 1;

        // 计算候选栏位置（在光标上方）
        SIZE size = CalculateSize();
        rcScreen.top -= size.cy + 5;  // 距离光标5像素
        rcScreen.bottom = rcScreen.top + size.cy;
        rcScreen.right = rcScreen.left + size.cx;

        // TODO: 实际应用中需要创建或更新候选栏窗口的位置
        // 这里简化处理，实际实现需要与输入法框架集成
    }
}

void CandidateManager::Invalidate() {
    if (m_hParent) {
        RECT rcInvalid;
        SIZE size = CalculateSize();
        GetWindowRect(m_hParent, &rcInvalid);
        ScreenToClient(m_hParent, reinterpret_cast<LPPOINT>(&rcInvalid));
        ScreenToClient(m_hParent, reinterpret_cast<LPPOINT>(&rcInvalid) + 1);

        rcInvalid.right = rcInvalid.left + size.cx;
        rcInvalid.bottom = rcInvalid.top + size.cy;

        InvalidateRect(m_hParent, &rcInvalid, FALSE);
    }
}

void CandidateManager::Render(HDC hdc) const {
    if (m_state == CandidateState::Hidden || m_candidates.empty()) {
        return;
    }

    RECT rcClient;
    GetClientRect(reinterpret_cast<HWND>(hdc), &rcClient);

    // 绘制背景
    HBRUSH hBrush = CreateSolidBrush(m_style.bgColor);
    FillRect(hdc, &rcClient, hBrush);
    DeleteObject(hBrush);

    // 计算候选项布局
    SIZE size = CalculateSize();
    int itemWidth = (size.cx - 2 * m_style.pagePadding) / 3;  // 每行3个
    int itemHeight = (size.cy - 2 * m_style.pagePadding - 2 * m_style.itemSpacing) / 3;

    // 当前页面的候选词范围
    int startIndex = m_currentPage * m_style.maxItemsPerPage;
    int endIndex = min(startIndex + m_style.maxItemsPerPage, static_cast<int>(m_candidates.size()));

    // 绘制候选词
    for (int i = startIndex; i < endIndex; ++i) {
        int displayIndex = i - startIndex;
        int row = displayIndex / 3;
        int col = displayIndex % 3;

        RECT rcItem;
        rcItem.left = m_style.pagePadding + col * (itemWidth + m_style.itemSpacing);
        rcItem.top = m_style.pagePadding + row * (itemHeight + m_style.itemSpacing);
        rcItem.right = rcItem.left + itemWidth;
        rcItem.bottom = rcItem.top + itemHeight;

        bool isSelected = (displayIndex == m_selectedIndex);

        DrawCandidateItem(hdc, rcItem, m_candidates[i], isSelected);
    }
}

bool CandidateManager::HandleMouseMove(int x, int y) {
    if (m_state == CandidateState::Hidden) {
        return false;
    }

    SIZE size = CalculateSize();
    int itemWidth = (size.cx - 2 * m_style.pagePadding) / 3;
    int itemHeight = (size.cy - 2 * m_style.pagePadding - 2 * m_style.itemSpacing) / 3;

    int startIndex = m_currentPage * m_style.maxItemsPerPage;
    int endIndex = min(startIndex + m_style.maxItemsPerPage, static_cast<int>(m_candidates.size()));

    bool found = false;
    for (int i = startIndex; i < endIndex; ++i) {
        int displayIndex = i - startIndex;
        int row = displayIndex / 3;
        int col = displayIndex % 3;

        RECT rcItem;
        rcItem.left = m_style.pagePadding + col * (itemWidth + m_style.itemSpacing);
        rcItem.top = m_style.pagePadding + row * (itemHeight + m_style.itemSpacing);
        rcItem.right = rcItem.left + itemWidth;
        rcItem.bottom = rcItem.top + itemHeight;

        if (PtInRect(&rcItem, {static_cast<LONG>(x), static_cast<LONG>(y)})) {
            m_selectedIndex = displayIndex;
            found = true;
            break;
        }
    }

    return found;
}

bool CandidateManager::HandleLButtonDown(int x, int y) {
    if (HandleMouseMove(x, y)) {
        m_mouseDown = true;
        return true;
    }
    return false;
}

bool CandidateManager::HandleLButtonUp(int x, int y) {
    if (m_mouseDown) {
        HandleMouseMove(x, y);
        if (m_selectedIndex >= 0) {
            SelectCandidateByIndex(m_selectedIndex);
        }
        m_mouseDown = false;
        return true;
    }
    return false;
}

bool CandidateManager::HandleKeyDown(WORD vkCode) {
    if (m_state == CandidateState::Hidden) {
        return false;
    }

    switch (vkCode) {
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
            // 方向键导航
            if (vkCode == VK_UP || vkCode == VK_DOWN) {
                int direction = (vkCode == VK_DOWN) ? 1 : -1;
                int newIndex = m_selectedIndex + direction;
                if (newIndex >= 0 && newIndex < static_cast<int>(m_candidates.size())) {
                    m_selectedIndex = newIndex;
                    Invalidate();
                    return true;
                }
            } else {
                // 左右键翻页
                int pages = (m_candidates.size() + m_style.maxItemsPerPage - 1) / m_style.maxItemsPerPage;
                if (pages <= 1) return false;

                int newPage = (vkCode == VK_LEFT) ? m_currentPage - 1 : m_currentPage + 1;
                if (newPage >= 0 && newPage < pages) {
                    SetCurrentPage(newPage);
                    m_selectedIndex = 0;
                    Invalidate();
                    return true;
                }
            }
            break;

        case VK_RETURN:
            if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_candidates.size())) {
                SelectCandidateByIndex(m_selectedIndex);
                return true;
            }
            break;

        case VK_ESCAPE:
            Hide();
            return true;
    }

    return false;
}

bool CandidateManager::HandleKeyUp(WORD vkCode) {
    return false; // 不需要处理 KeyUp
}

bool CandidateManager::HandleChar(WCHAR ch) {
    if (m_state == CandidateState::Hidden) {
        return false;
    }

    // 数字键选择候选词
    if (ch >= L'1' && ch <= L'9') {
        int index = ch - L'1';
        if (index < static_cast<int>(m_candidates.size())) {
            SelectCandidate(index);
            return true;
        }
    }

    return false;
}

bool CandidateManager::SelectCandidate(int index) {
    if (index >= 0 && index < static_cast<int>(m_candidates.size())) {
        SelectCandidateByIndex(index);
        return true;
    }
    return false;
}

bool CandidateManager::SelectCandidateByIndex(int displayIndex) {
    if (displayIndex < 0 || displayIndex >= static_cast<int>(m_candidates.size())) {
        return false;
    }

    // 返回选中的候选词
    const CandidateItem& selected = m_candidates[displayIndex];

    // TODO: 实际应用中需要将选中的候选词提交到输入框
    // 这里只是演示逻辑

    Hide(); // 隐藏候选栏
    return true;
}

void CandidateManager::DeduplicateAndSort(std::vector<CandidateItem>& items) {
    // 去重
    std::sort(items.begin(), items.end(), CandidateItemComparator());
    items.erase(
        std::unique(items.begin(), items.end(), CandidateItemEqual()),
        items.end()
    );

    // 再次排序（确保去重后保持正确的顺序）
    std::sort(items.begin(), items.end(), CandidateItemComparator());
}

int CandidateManager::GetCategoryPriority(CandidateCategory category) const {
    switch (category) {
        case CandidateCategory::Word:       return 100; // 普通词汇
        case CandidateCategory::AI:         return 90;  // AI生成
        case CandidateCategory::Knowledge: return 80;  // 知识库
        case CandidateCategory::Frequent:   return 70;  // 高频词
        case CandidateCategory::Recent:     return 60;  // 最近使用
        default:                            return 50;  // 其他
    }
}

std::wstring CandidateManager::FormatDisplayText(const CandidateItem& item) const {
    std::wstring result = item.text;

    if (m_style.showCategoryTags) {
        // 添加分类标签
        std::wstring tag;
        switch (item.category) {
            case CandidateCategory::AI:
                tag = L"[AI]";
                break;
            case CandidateCategory::Knowledge:
                tag = L"[知]";
                break;
            case CandidateCategory::Frequent:
                tag = L"[频]";
                break;
            case CandidateCategory::Recent:
                tag = L"[近]";
                break;
            default:
                tag = L"";
                break;
        }

        if (!tag.empty()) {
            result += L" ";
            result += tag;
        }
    }

    return result;
}

SIZE CandidateManager::CalculateSize() const {
    if (m_candidates.empty()) {
        return {0, 0};
    }

    int maxItems = min(static_cast<int>(m_candidates.size()), m_style.maxItemsPerPage);
    int rows = (maxItems + 2) / 3;  // 每行3个，向上取整

    int width = m_style.pagePadding * 2 + 3 * ((m_style.pagePadding + m_style.itemSpacing));
    int height = m_style.pagePadding * 2 + rows * m_style.itemSpacing +
                 rows * (m_style.fontSize + 4);  // 字体高度+上下边距

    return {width, height};
}

void CandidateManager::DrawCandidateItem(HDC hdc, const RECT& rcItem,
                                        const CandidateItem& item, bool isSelected) const {
    // 绘制选中背景
    if (isSelected) {
        HBRUSH hBrush = CreateSolidBrush(m_style.selectedBgColor);
        FillRect(hdc, &rcItem, hBrush);
        DeleteObject(hBrush);
    }

    // 绘制边框
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));

    Rectangle(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    // 绘制文本
    std::wstring displayText = FormatDisplayText(item);

    // 使用 GDI+ 绘制抗锯齿文本
    Graphics graphics(hdc);
    Font font(hdc, m_style.fontFamily.c_str());
    SolidBrush brush(isSelected ? m_style.selectedTextColor : m_style.textColor);

    RectF layoutRect(static_cast<float>(rcItem.left + 2),
                     static_cast<float>(rcItem.top + 2),
                     static_cast<float>(rcItem.right - rcItem.left - 4),
                     static_cast<float>(rcItem.bottom - rcItem.top - 4));

    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
    graphics.DrawString(displayText.c_str(), -1, &font, layoutRect, nullptr, &brush);
}

void CandidateManager::DrawCategoryTag(HDC hdc, const RECT& rcTag,
                                      CandidateCategory category) const {
    // 绘制分类标签背景
    HBRUSH hBrush = CreateSolidBrush(RGB(100, 150, 200));
    FillRect(hdc, &rcTag, hBrush);
    DeleteObject(hBrush);

    // 绘制标签文本
    std::wstring tagText;
    switch (category) {
        case CandidateCategory::AI:
            tagText = L"AI";
            break;
        case CandidateCategory::Knowledge:
            tagText = L"知";
            break;
        case CandidateCategory::Frequent:
            tagText = L"频";
            break;
        case CandidateCategory::Recent:
            tagText = L"近";
            break;
        default:
            return;
    }

    Font font(hdc, m_style.categoryTagFont.c_str(), m_style.tagFontSize);
    SolidBrush brush(RGB(255, 255, 255));
    RectF layoutRect(static_cast<float>(rcTag.left), static_cast<float>(rcTag.top),
                     static_cast<float>(rcTag.right - rcTag.left),
                     static_cast<float>(rcTag.bottom - rcTag.top));

    Graphics graphics(hdc);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
    graphics.DrawString(tagText.c_str(), -1, &font, layoutRect, nullptr, &brush);
}

// 更新分页信息
void CandidateManager::UpdatePagination() {
    if (m_candidates.empty()) {
        m_totalPages = 0;
        m_currentPage = 0;
        return;
    }

    m_totalPages = (m_candidates.size() + m_style.maxItemsPerPage - 1) / m_style.maxItemsPerPage;
    if (m_totalPages == 0) {
        m_totalPages = 1;
    }
    m_currentPage = min(m_currentPage, m_totalPages - 1);
}

} // namespace qi::ui