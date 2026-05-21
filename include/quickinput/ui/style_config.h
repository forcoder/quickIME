#ifndef QUICKINPUT_UI_STYLE_CONFIG_H_
#define QUICKINPUT_UI_STYLE_CONFIG_H_

#include <windows.h>
#include <string>
#include <vector>

namespace qi::ui {

// 样式配置管理器
class StyleConfig {
public:
    StyleConfig();
    ~StyleConfig();

    // 加载配置文件
    bool LoadFromFile(const std::wstring& filePath);

    // 保存配置文件
    bool SaveToFile(const std::wstring& filePath) const;

    // 从JSON字符串加载（简化实现）
    bool LoadFromJson(const std::string& jsonStr);

    // 转换为JSON字符串（简化实现）
    std::string ToJson() const;

    // 样式设置方法
    void SetFontFamily(const std::wstring& fontFamily);
    void SetFontSize(int fontSize);
    void SetTextColor(COLORREF color);
    void SetBackgroundColor(COLORREF color);
    void SetSelectedBgColor(COLORREF color);
    void SetSelectedTextColor(COLORREF color);
    void SetPagePadding(int padding);
    void SetItemSpacing(int spacing);
    void SetMaxItemsPerPage(int maxItems);
    void SetMaxPages(int maxPages);
    void SetShowCategoryTags(bool show);
    void SetCategoryTagFont(const std::wstring& font);
    void SetTagFontSize(int size);

    // 获取样式属性
    const std::wstring& GetFontFamily() const;
    int GetFontSize() const;
    COLORREF GetTextColor() const;
    COLORREF GetBackgroundColor() const;
    COLORREF GetSelectedBgColor() const;
    COLORREF GetSelectedTextColor() const;
    int GetPagePadding() const;
    int GetItemSpacing() const;
    int GetMaxItemsPerPage() const;
    int GetMaxPages() const;
    bool GetShowCategoryTags() const;
    const std::wstring& GetCategoryTagFont() const;
    int GetTagFontSize() const;

    // 验证配置有效性
    bool Validate() const;

    // 重置为默认值
    void ResetToDefault();

private:
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
    };

    CandidateStyle m_style;

    // 默认样式
    static const CandidateStyle kDefaultStyle;
};

} // namespace qi::ui

#endif // QUICKINPUT_UI_STYLE_CONFIG_H_