#pragma once

#include "quickinput/core/common.h"
#include <cstdint>
#include <array>
#include <string_view>

namespace qi {

// 五笔版本枚举
enum class WubiVersion : uint8_t {
    Wubi86 = 86,    // 86版（最常用）
    Wubi98 = 98,    // 98版
};

// 五笔编码最大长度（单字最多4码，识别码最多5码，但标准查询用4码）
constexpr size_t kWubiMaxCodeLen = 4;
constexpr size_t kWubiMinCodeLen = 1;

// 五笔编码类型：固定长度的小写字母序列
struct WubiCode {
    std::array<wchar_t, kWubiMaxCodeLen> chars{};
    uint8_t length = 0;

    WubiCode() = default;

    // 从字符串构造，自动规范化为小写
    explicit WubiCode(const std::wstring& code);

    // 转为小写宽字符串
    std::wstring ToString() const;

    // 比较操作（用于 map 查找）
    bool operator==(const WubiCode& other) const;
    bool operator<(const WubiCode& other) const;

    // 获取前缀编码（用于前缀匹配）
    WubiCode Prefix(uint8_t len) const;
};

// 为 WubiCode 提供哈希支持
struct WubiCodeHash {
    size_t operator()(const WubiCode& code) const;
};

// 编码验证：检查是否为合法的五笔编码（仅包含 a-z，长度 1-4）
bool IsValidWubiCode(const std::wstring& code);

// 编码规范化：转为小写，去除非法字符
std::wstring NormalizeCode(const std::wstring& code);

// 检查字符是否为合法的五笔编码字符（a-z）
inline bool IsWubiChar(wchar_t ch) {
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

} // namespace qi
