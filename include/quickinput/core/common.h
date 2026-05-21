#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <msctf.h>
#include <ctffunc.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>

// ── Project info ──
#define QI_APP_NAME      _T("QuickInput")
#define QI_APP_NAME_W    L"QuickInput"
#define QI_VERSION_MAJOR 1
#define QI_VERSION_MINOR 0
#define QI_VERSION_PATCH 0

// ── Registry paths ──
#define QI_REG_KEY       _T("SOFTWARE\\QuickInput")
#define QI_CLSID_FORMAT  _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}")

// ── Candidate categories ──
enum class CandidateCategory : uint8_t {
    WubiNormal = 0,     // 普通五笔词库
    WubiCustom,         // 自定义词库
    KnowledgeBase,      // 知识库内容
    AISuggestion,       // AI智能建议
    CategoryCount
};

// ── Input modes ──
enum class InputMode : uint8_t {
    PureWubi = 0,       // 纯五笔模式
    Hybrid,             // 混合智能模式
    PureAI,             // 纯AI补全模式
    ModeCount
};

// ── Candidate item ──
struct CandidateItem {
    std::wstring    text;           // 候选文本
    CandidateCategory category;     // 分类
    float           score;          // 匹配分数 (0.0 - 1.0)
    uint32_t        sourceIndex;    // 来源索引（码表行号/知识库ID等）

    CandidateItem() : category(CandidateCategory::WubiNormal), score(0.0f), sourceIndex(0) {}
    CandidateItem(const std::wstring& t, CandidateCategory c, float s = 0.0f)
        : text(t), category(c), score(s), sourceIndex(0) {}
};

// ── Input context ──
struct InputContext {
    std::wstring    composition;        // 当前输入的五笔编码
    std::wstring    committedText;      // 已上屏的文本
    std::wstring    surroundingText;    // 光标周围文本（上下文）
    int             cursorPos;          // 光标位置
    bool            isChineseMode;      // 中文模式
    bool            isFullWidth;        // 全角模式
    InputMode       mode;               // 当前输入模式

    InputContext() : cursorPos(0), isChineseMode(true), isFullWidth(false), mode(InputMode::Hybrid) {}
};

// ── GUID for QuickInput text service ──
// {B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}
// 实际项目中应使用 guidgen.exe 生成唯一 GUID
constexpr GUID QI_GUID_TEXT_SERVICE = {
    0xb4f3a834, 0x5c91, 0x4e8d, {0x8a, 0x7f, 0x3d, 0x2c, 0x1e, 0x6a, 0x9f, 0x0b}
};

// ── GUID for profile ──
// {C7E8D21A-3F4B-4A6C-9E1D-8B5F7A3C2D0E}
constexpr GUID QI_GUID_PROFILE = {
    0xc7e8d21a, 0x3f4b, 0x4a6c, {0x9e, 0x1d, 0x8b, 0x5f, 0x7a, 0x3c, 0x2d, 0x0e}
};

// ── GUID for display attribute ──
constexpr GUID QI_GUID_DISPLAY_ATTR = {
    0xa1b2c3d4, 0xe5f6, 0x4a7b, {0x8c, 0x9d, 0x0e, 0x1f, 0x2a, 0x3b, 0x4c, 0x5d}
};

// ── Hotkey IDs ──
#define QI_HOTKEY_TOGGLE_AI      1001
#define QI_HOTKEY_TOGGLE_KB      1002
#define QI_HOTKEY_SAVE_TO_KB     1003
#define QI_HOTKEY_OPEN_CONFIG   1004
#define QI_HOTKEY_TOGGLE_MODE   1005

// ── Utility macros ──
#define QI_SAFE_RELEASE(p)  if (p) { (p)->Release(); (p) = nullptr; }
#define QI_SAFE_DELETE(p)   if (p) { delete (p); (p) = nullptr; }
#define QI_SAFE_DELETE_ARRAY(p) if (p) { delete[] (p); (p) = nullptr; }

namespace qi {
    // ── String utilities ──
    std::wstring Utf8ToWide(const std::string& utf8);
    std::string  WideToUtf8(const std::wstring& wide);
    std::wstring ToLower(const std::wstring& str);
    bool         StartsWith(const std::wstring& str, const std::wstring& prefix);

    // ── Path utilities ──
    std::wstring GetAppDataPath();
    std::wstring GetInstallPath();
    std::wstring GetKbPath();
    std::wstring GetModelPath();

    // ── GUID utilities ──
    std::wstring GuidToString(const GUID& guid);
    GUID         StringToGuid(const std::wstring& str);
} // namespace qi
