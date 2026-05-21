#pragma once

#include "quickinput/core/common.h"
#include "quickinput/core/wubi_types.h"
#include "quickinput/core/wubi_table.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <mutex>
#include <shared_mutex>

namespace qi {

// ═══════════════════════════════════════════════════════════════
// 五笔编码引擎核心类
// 负责码表加载、编码查询、模糊查询、词组联想、用户造词等功能
// ═══════════════════════════════════════════════════════════════
class WubiEngine {
public:
    WubiEngine();
    ~WubiEngine();

    // ── 码表加载 ──

    // 加载搜狗五笔码表文件（.scel 细胞词库 + .uwl 用户词库）
    bool LoadTable(const std::wstring& scelPath, const std::wstring& uwlPath);

    // 加载纯文本格式码表（编码\t词\t词频）
    bool LoadTextTable(const std::wstring& textPath);

    // 加载内置码表（一级简码 + 二级简码）
    void LoadBuiltinTable();

    // ── 编码查询 ──

    // 精确查询：根据五笔编码返回候选词列表
    // code: 五笔编码（如 "aaaa"）
    // maxResults: 最大返回结果数
    std::vector<CandidateItem> Query(const std::wstring& code, int maxResults = 20);

    // 模糊查询：支持容错码（如 gggg 和 ggg 都匹配"一"）
    std::vector<CandidateItem> FuzzyQuery(const std::wstring& code, int maxResults = 20);

    // 词组联想：根据已输入汉字联想后续词组
    // prefix: 已输入的汉字前缀
    std::vector<CandidateItem> Associate(const std::wstring& prefix, int maxResults = 10);

    // ── 用户词管理 ──

    // 手动造词：添加用户自定义词
    bool AddCustomWord(const std::wstring& word, const std::wstring& code);

    // 固顶词管理：固顶词排在候选列表最前面
    bool PinWord(const std::wstring& word);
    bool UnpinWord(const std::wstring& word);

    // 保存用户词库（沿用搜狗 .uwl 格式）
    bool SaveUserTable(const std::wstring& path);

    // ── 版本管理 ──

    void SetVersion(int version) { m_version = (version == 98) ? 98 : 86; }
    int GetVersion() const { return m_version; }

    // ── 统计信息 ──
    size_t GetEntryCount() const { return m_entries.size(); }
    size_t GetCustomCount() const { return m_customEntries.size(); }

private:
    // 码表条目
    struct TableEntry {
        std::wstring code;      // 五笔编码
        std::wstring word;      // 汉字/词组
        uint32_t frequency;     // 词频
        bool isPinned;          // 是否固顶
        bool isCustom;          // 是否用户自定义

        TableEntry() : frequency(0), isPinned(false), isCustom(false) {}
    };

    // 编码索引：编码 -> 条目索引列表
    std::unordered_map<std::wstring, std::vector<size_t>> m_codeIndex;

    // 所有条目（内置 + 文件加载 + 用户自定义）
    std::vector<TableEntry> m_entries;

    // 固顶词集合
    std::unordered_set<std::wstring> m_pinnedWords;

    // 用户自定义词
    std::vector<TableEntry> m_customEntries;

    // 联想索引：首字 -> 词组条目索引
    std::unordered_map<std::wstring, std::vector<size_t>> m_assocIndex;

    // 当前版本（86 或 98）
    int m_version = 86;

    // 线程安全读写锁
    mutable std::shared_mutex m_mutex;

    // 容错码映射（常见错误编码 -> 正确编码的字符替换）
    std::unordered_map<wchar_t, std::vector<wchar_t>> m_fuzzyMap;

    // ── 内部方法 ──

    // 初始化容错码映射表
    void InitFuzzyMap();

    // 解析搜狗 .scel 二进制格式
    bool ParseScelFile(const std::wstring& path);

    // 解析搜狗 .uwl 用户词库格式
    bool ParseUwlFile(const std::wstring& path);

    // 解析纯文本格式（编码\t词\t词频）
    bool ParseTextFile(const std::wstring& path);

    // 生成模糊编码列表
    std::vector<std::wstring> GenerateFuzzyCodes(const std::wstring& code);

    // 向索引中添加条目
    void AddEntry(const std::wstring& code, const std::wstring& word,
                  uint32_t frequency, bool isCustom);

    // 构建联想索引
    void BuildAssocIndex();

    // 排序候选结果（固顶 > 词频 > 编码长度）
    static void SortCandidates(std::vector<CandidateItem>& items);
};

} // namespace qi
