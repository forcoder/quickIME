#include "quickinput/core/wubi_types.h"
#include <algorithm>
#include <cctype>

namespace qi {

// ═══════════════════════════════════════════════════════════════
// WubiCode 实现
// ═══════════════════════════════════════════════════════════════

WubiCode::WubiCode(const std::wstring& code) {
    length = 0;
    for (wchar_t ch : code) {
        if (length >= kWubiMaxCodeLen) break;
        if (ch >= L'A' && ch <= L'Z') {
            chars[length++] = ch - L'A' + L'a';  // 转小写
        } else if (ch >= L'a' && ch <= L'z') {
            chars[length++] = ch;
        }
        // 忽略非法字符
    }
}

std::wstring WubiCode::ToString() const {
    return std::wstring(chars.data(), length);
}

bool WubiCode::operator==(const WubiCode& other) const {
    if (length != other.length) return false;
    for (uint8_t i = 0; i < length; ++i) {
        if (chars[i] != other.chars[i]) return false;
    }
    return true;
}

bool WubiCode::operator<(const WubiCode& other) const {
    uint8_t minLen = (std::min)(length, other.length);
    for (uint8_t i = 0; i < minLen; ++i) {
        if (chars[i] != other.chars[i]) return chars[i] < other.chars[i];
    }
    return length < other.length;
}

WubiCode WubiCode::Prefix(uint8_t len) const {
    WubiCode result;
    result.length = (std::min)(len, length);
    for (uint8_t i = 0; i < result.length; ++i) {
        result.chars[i] = chars[i];
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════
// WubiCodeHash 实现
// ═══════════════════════════════════════════════════════════════

size_t WubiCodeHash::operator()(const WubiCode& code) const {
    // FNV-1a hash
    size_t hash = 14695981039346656037ULL;
    for (uint8_t i = 0; i < code.length; ++i) {
        hash ^= static_cast<size_t>(code.chars[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// ═══════════════════════════════════════════════════════════════
// 全局函数实现
// ═══════════════════════════════════════════════════════════════

bool IsValidWubiCode(const std::wstring& code) {
    if (code.empty() || code.size() > kWubiMaxCodeLen) {
        return false;
    }
    for (wchar_t ch : code) {
        if (!((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z'))) {
            return false;
        }
    }
    return true;
}

std::wstring NormalizeCode(const std::wstring& code) {
    std::wstring result;
    result.reserve(code.size());
    for (wchar_t ch : code) {
        if (ch >= L'A' && ch <= L'Z') {
            result.push_back(ch - L'A' + L'a');  // 大写转小写
        } else if (ch >= L'a' && ch <= L'z') {
            result.push_back(ch);
        }
        // 忽略其他字符
    }
    // 截断到最大长度
    if (result.size() > kWubiMaxCodeLen) {
        result.resize(kWubiMaxCodeLen);
    }
    return result;
}

} // namespace qi
