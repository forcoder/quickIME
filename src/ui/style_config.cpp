#include "quickinput/ui/style_config.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace qi::ui {

const StyleConfig::CandidateStyle StyleConfig::kDefaultStyle = {
    // 字体设置
    L"微软雅黑",           // fontFamily
    12,                     // fontSize
    RGB(0, 0, 0),          // textColor
    RGB(255, 255, 255),   // bgColor
    RGB(200, 230, 255),   // selectedBgColor
    RGB(0, 0, 0),          // selectedTextColor

    // 布局设置
    5,                     // pagePadding
    2,                     // itemSpacing
    9,                     // maxItemsPerPage
    5,                     // maxPages

    // AI/知识库标签样式
    true,                  // showCategoryTags
    L"Arial",              // categoryTagFont
    9                      // tagFontSize
};

StyleConfig::StyleConfig() {
    ResetToDefault();
}

StyleConfig::~StyleConfig() {
}

bool StyleConfig::LoadFromFile(const std::wstring& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        // 读取文件内容
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

        file.close();

        // 解析JSON格式的配置（简化实现）
        return LoadFromJson(content);
    } catch (...) {
        file.close();
        return false;
    }
}

bool StyleConfig::SaveToFile(const std::wstring& filePath) const {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        // 转换为JSON字符串
        std::string jsonStr = ToJson();

        file << jsonStr;
        file.close();
        return true;
    } catch (...) {
        file.close();
        return false;
    }
}

bool StyleConfig::LoadFromJson(const std::string& jsonStr) {
    // 简化的JSON解析实现
    // 实际应用中应该使用专业的JSON库

    std::string str = jsonStr;

    // 移除空白字符
    str.erase(std::remove_if(str.begin(), str.end(),
                            [](unsigned char x) { return std::isspace(x); }),
             str.end());

    // 解析fontFamily
    size_t pos = str.find("\"fontFamily\":");
    if (pos != std::string::npos) {
        pos += 14; // "\"fontFamily\":".length()
        size_t end = str.find_first_of("\",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.fontFamily = std::wstring(value.begin(), value.end());
        }
    }

    // 解析fontSize
    pos = str.find("\"fontSize\":");
    if (pos != std::string::npos) {
        pos += 11; // "\"fontSize\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.fontSize = std::stoi(value);
        }
    }

    // 解析textColor
    pos = str.find("\"textColor\":");
    if (pos != std::string::npos) {
        pos += 12; // "\"textColor\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            if (value.length() >= 6) {
                int r = std::stoi(value.substr(0, 2), nullptr, 16);
                int g = std::stoi(value.substr(2, 2), nullptr, 16);
                int b = std::stoi(value.substr(4, 2), nullptr, 16);
                m_style.textColor = RGB(r, g, b);
            }
        }
    }

    // 解析bgColor
    pos = str.find("\"bgColor\":");
    if (pos != std::string::npos) {
        pos += 10; // "\"bgColor\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            if (value.length() >= 6) {
                int r = std::stoi(value.substr(0, 2), nullptr, 16);
                int g = std::stoi(value.substr(2, 2), nullptr, 16);
                int b = std::stoi(value.substr(4, 2), nullptr, 16);
                m_style.bgColor = RGB(r, g, b);
            }
        }
    }

    // 解析selectedBgColor
    pos = str.find("\"selectedBgColor\":");
    if (pos != std::string::npos) {
        pos += 17; // "\"selectedBgColor\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            if (value.length() >= 6) {
                int r = std::stoi(value.substr(0, 2), nullptr, 16);
                int g = std::stoi(value.substr(2, 2), nullptr, 16);
                int b = std::stoi(value.substr(4, 2), nullptr, 16);
                m_style.selectedBgColor = RGB(r, g, b);
            }
        }
    }

    // 解析selectedTextColor
    pos = str.find("\"selectedTextColor\":");
    if (pos != std::string::npos) {
        pos += 20; // "\"selectedTextColor\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            if (value.length() >= 6) {
                int r = std::stoi(value.substr(0, 2), nullptr, 16);
                int g = std::stoi(value.substr(2, 2), nullptr, 16);
                int b = std::stoi(value.substr(4, 2), nullptr, 16);
                m_style.selectedTextColor = RGB(r, g, b);
            }
        }
    }

    // 解析pagePadding
    pos = str.find("\"pagePadding\":");
    if (pos != std::string::npos) {
        pos += 14; // "\"pagePadding\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.pagePadding = std::stoi(value);
        }
    }

    // 解析itemSpacing
    pos = str.find("\"itemSpacing\":");
    if (pos != std::string::npos) {
        pos += 14; // "\"itemSpacing\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.itemSpacing = std::stoi(value);
        }
    }

    // 解析maxItemsPerPage
    pos = str.find("\"maxItemsPerPage\":");
    if (pos != std::string::npos) {
        pos += 18; // "\"maxItemsPerPage\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.maxItemsPerPage = std::stoi(value);
        }
    }

    // 解析maxPages
    pos = str.find("\"maxPages\":");
    if (pos != std::string::npos) {
        pos += 11; // "\"maxPages\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.maxPages = std::stoi(value);
        }
    }

    // 解析showCategoryTags
    pos = str.find("\"showCategoryTags\":");
    if (pos != std::string::npos) {
        pos += 19; // "\"showCategoryTags\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.showCategoryTags = (value == "true");
        }
    }

    // 解析categoryTagFont
    pos = str.find("\"categoryTagFont\":");
    if (pos != std::string::npos) {
        pos += 18; // "\"categoryTagFont\":".length()
        size_t end = str.find_first_of("\",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.categoryTagFont = std::wstring(value.begin(), value.end());
        }
    }

    // 解析tagFontSize
    pos = str.find("\"tagFontSize\":");
    if (pos != std::string::npos) {
        pos += 14; // "\"tagFontSize\":".length()
        size_t end = str.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string value = str.substr(pos, end - pos);
            m_style.tagFontSize = std::stoi(value);
        }
    }

    return Validate();
}

std::string StyleConfig::ToJson() const {
    std::ostringstream oss;

    oss << "{";
    oss << "\"fontFamily\":\"" << wstring_to_utf8(m_style.fontFamily) << "\",";
    oss << "\"fontSize\":" << m_style.fontSize << ",";
    oss << "\"textColor\":\"" << color_to_hex(m_style.textColor) << "\",";
    oss << "\"bgColor\":\"" << color_to_hex(m_style.bgColor) << "\",";
    oss << "\"selectedBgColor\":\"" << color_to_hex(m_style.selectedBgColor) << "\",";
    oss << "\"selectedTextColor\":\"" << color_to_hex(m_style.selectedTextColor) << "\",";
    oss << "\"pagePadding\":" << m_style.pagePadding << ",";
    oss << "\"itemSpacing\":" << m_style.itemSpacing << ",";
    oss << "\"maxItemsPerPage\":" << m_style.maxItemsPerPage << ",";
    oss << "\"maxPages\":" << m_style.maxPages << ",";
    oss << "\"showCategoryTags\":" << (m_style.showCategoryTags ? "true" : "false") << ",";
    oss << "\"categoryTagFont\":\"" << wstring_to_utf8(m_style.categoryTagFont) << "\",";
    oss << "\"tagFontSize\":" << m_style.tagFontSize;
    oss << "}";

    return oss.str();
}

void StyleConfig::SetFontFamily(const std::wstring& fontFamily) {
    m_style.fontFamily = fontFamily;
}

void StyleConfig::SetFontSize(int fontSize) {
    m_style.fontSize = fontSize;
}

void StyleConfig::SetTextColor(COLORREF color) {
    m_style.textColor = color;
}

void StyleConfig::SetBackgroundColor(COLORREF color) {
    m_style.bgColor = color;
}

void StyleConfig::SetSelectedBgColor(COLORREF color) {
    m_style.selectedBgColor = color;
}

void StyleConfig::SetSelectedTextColor(COLORREF color) {
    m_style.selectedTextColor = color;
}

void StyleConfig::SetPagePadding(int padding) {
    m_style.pagePadding = padding;
}

void StyleConfig::SetItemSpacing(int spacing) {
    m_style.itemSpacing = spacing;
}

void StyleConfig::SetMaxItemsPerPage(int maxItems) {
    m_style.maxItemsPerPage = maxItems;
}

void StyleConfig::SetMaxPages(int maxPages) {
    m_style.maxPages = maxPages;
}

void StyleConfig::SetShowCategoryTags(bool show) {
    m_style.showCategoryTags = show;
}

void StyleConfig::SetCategoryTagFont(const std::wstring& font) {
    m_style.categoryTagFont = font;
}

void StyleConfig::SetTagFontSize(int size) {
    m_style.tagFontSize = size;
}

const std::wstring& StyleConfig::GetFontFamily() const {
    return m_style.fontFamily;
}

int StyleConfig::GetFontSize() const {
    return m_style.fontSize;
}

COLORREF StyleConfig::GetTextColor() const {
    return m_style.textColor;
}

COLORREF StyleConfig::GetBackgroundColor() const {
    return m_style.bgColor;
}

COLORREF StyleConfig::GetSelectedBgColor() const {
    return m_style.selectedBgColor;
}

COLORREF StyleConfig::GetSelectedTextColor() const {
    return m_style.selectedTextColor;
}

int StyleConfig::GetPagePadding() const {
    return m_style.pagePadding;
}

int StyleConfig::GetItemSpacing() const {
    return m_style.itemSpacing;
}

int StyleConfig::GetMaxItemsPerPage() const {
    return m_style.maxItemsPerPage;
}

int StyleConfig::GetMaxPages() const {
    return m_style.maxPages;
}

bool StyleConfig::GetShowCategoryTags() const {
    return m_style.showCategoryTags;
}

const std::wstring& StyleConfig::GetCategoryTagFont() const {
    return m_style.categoryTagFont;
}

int StyleConfig::GetTagFontSize() const {
    return m_style.tagFontSize;
}

bool StyleConfig::Validate() const {
    // 验证字体族名
    if (m_style.fontFamily.empty()) {
        return false;
    }

    // 验证字号范围
    if (m_style.fontSize <= 0 || m_style.fontSize > 72) {
        return false;
    }

    // 验证页边距
    if (m_style.pagePadding < 0 || m_style.pagePadding > 50) {
        return false;
    }

    // 验证条目间距
    if (m_style.itemSpacing < 0 || m_style.itemSpacing > 20) {
        return false;
    }

    // 验证每页最大条目数
    if (m_style.maxItemsPerPage <= 0 || m_style.maxItemsPerPage > 20) {
        return false;
    }

    // 验证最大页数
    if (m_style.maxPages <= 0 || m_style.maxPages > 10) {
        return false;
    }

    // 验证标签字号
    if (m_style.tagFontSize <= 0 || m_style.tagFontSize > 24) {
        return false;
    }

    return true;
}

void StyleConfig::ResetToDefault() {
    m_style = kDefaultStyle;
}

// 辅助函数：将宽字符串转换为UTF-8字符串
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// 辅助函数：将COLORREF转换为十六进制字符串
std::string color_to_hex(COLORREF color) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(2) << ((color >> 16) & 0xFF)
        << std::setw(2) << ((color >> 8) & 0xFF)
        << std::setw(2) << (color & 0xFF);
    return oss.str();
}

} // namespace qi::ui