#include "quickinput/core/wubi_engine.h"
#include "quickinput/core/wubi_types.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdint>

namespace qi {

// 搜狗 .scel 文件格式常量
namespace scel {
    constexpr uint32_t kMagic = 0x40000000;
    constexpr size_t kHeaderSize = 0x120;
    constexpr size_t kIndexOffset = 0x2628;
    constexpr size_t kPinyinIndexOffset = 0x1540;
    constexpr size_t kPinyinDataOffset = 0x2628;
    constexpr uint16_t kTypeWord = 0x0000;
    constexpr uint16_t kTypePinyin = 0x0001;
}

// 搜狗 .uwl 用户词库格式常量
namespace uwl {
    constexpr uint32_t kMagic = 0x00000001;
    constexpr size_t kHeaderSize = 0x10;
}

WubiEngine::WubiEngine() {
    InitFuzzyMap();
}

WubiEngine::~WubiEngine() = default;

// 容错码映射初始化
void WubiEngine::InitFuzzyMap() {
    m_fuzzyMap.clear();
    // 横区（gfdsa）
    m_fuzzyMap[L'g'] = {L'f', L'h', L't', L'y'};
    m_fuzzyMap[L'f'] = {L'g', L'd', L'r', L'v'};
    m_fuzzyMap[L'd'] = {L'f', L's', L'e', L'c'};
    m_fuzzyMap[L's'] = {L'd', L'a', L'w', L'x'};
    m_fuzzyMap[L'a'] = {L's', L'q', L'z'};
    // 竖区（hjklm）
    m_fuzzyMap[L'h'] = {L'g', L'j', L'y', L'n'};
    m_fuzzyMap[L'j'] = {L'h', L'k', L'u', L'm'};
    m_fuzzyMap[L'k'] = {L'j', L'l', L'i'};
    m_fuzzyMap[L'l'] = {L'k', L'o'};
    m_fuzzyMap[L'm'] = {L'j', L'n'};
    // 撇区（trewq）
    m_fuzzyMap[L't'] = {L'r', L'g', L'y'};
    m_fuzzyMap[L'r'] = {L't', L'e', L'f', L'b'};
    m_fuzzyMap[L'e'] = {L'r', L'w', L'd'};
    m_fuzzyMap[L'w'] = {L'e', L'q', L's'};
    m_fuzzyMap[L'q'] = {L'w', L'a'};
    // 捺区（yuiop）
    m_fuzzyMap[L'y'] = {L't', L'u', L'h', L'g'};
    m_fuzzyMap[L'u'] = {L'y', L'i', L'j', L'h'};
    m_fuzzyMap[L'i'] = {L'u', L'o', L'k', L'j'};
    m_fuzzyMap[L'o'] = {L'i', L'p', L'l'};
    m_fuzzyMap[L'p'] = {L'o', L'l'};
    // 折区（nbvcx）
    m_fuzzyMap[L'n'] = {L'b', L'm', L'h', L'j'};
    m_fuzzyMap[L'b'] = {L'n', L'v', L'f', L'g'};
    m_fuzzyMap[L'v'] = {L'b', L'c', L'f'};
    m_fuzzyMap[L'c'] = {L'v', L'x', L'd'};
    m_fuzzyMap[L'x'] = {L'c', L'z', L's'};
    m_fuzzyMap[L'z'] = {L'x', L'a'};
}

// 码表加载

bool WubiEngine::LoadTable(const std::wstring& scelPath, const std::wstring& uwlPath) {
    std::unique_lock lock(m_mutex);
    bool ok = true;
    if (!scelPath.empty()) {
        if (!ParseScelFile(scelPath)) {
            if (!ParseTextFile(scelPath)) {
                ok = false;
            }
        }
    }
    if (!uwlPath.empty()) {
        if (!ParseUwlFile(uwlPath)) {
            ok = false;
        }
    }
    BuildAssocIndex();
    return ok;
}

bool WubiEngine::LoadTextTable(const std::wstring& textPath) {
    std::unique_lock lock(m_mutex);
    bool ok = ParseTextFile(textPath);
    BuildAssocIndex();
    return ok;
}

void WubiEngine::LoadBuiltinTable() {
    std::unique_lock lock(m_mutex);
    for (const auto& entry : kLevel1Codes) {
        AddEntry(entry.code, entry.word, entry.frequency, false);
    }
    for (const auto& entry : kLevel2Codes) {
        AddEntry(entry.code, entry.word, entry.frequency, false);
    }
    BuildAssocIndex();
}

// 编码查询

std::vector<CandidateItem> WubiEngine::Query(const std::wstring& code, int maxResults) {
    std::shared_lock lock(m_mutex);
    if (code.empty() || maxResults <= 0) return {};
    std::wstring normCode = NormalizeCode(code);
    if (normCode.empty() || normCode.size() > kWubiMaxCodeLen) return {};
    std::vector<CandidateItem> results;
    // 精确匹配
    auto it = m_codeIndex.find(normCode);
    if (it != m_codeIndex.end()) {
        for (size_t idx : it->second) {
            if (idx < m_entries.size()) {
                const auto& entry = m_entries[idx];
                results.emplace_back(entry.word, CandidateCategory::WubiNormal,
                    static_cast<float>(entry.frequency) / 20000.0f);
                results.back().sourceIndex = static_cast<uint32_t>(idx);
            }
        }
    }
    // 前缀匹配
    if (normCode.size() < kWubiMaxCodeLen) {
        for (auto& kv : m_codeIndex) {
            const auto& key = kv.first;
            const auto& indices = kv.second;
            if (key.size() > normCode.size() &&
                key.compare(0, normCode.size(), normCode) == 0) {
                for (size_t idx : indices) {
                    if (idx < m_entries.size()) {
                        const auto& entry = m_entries[idx];
                        float prefixPenalty = 0.5f;
                        results.emplace_back(entry.word, CandidateCategory::WubiNormal,
                            static_cast<float>(entry.frequency) / 20000.0f * prefixPenalty);
                        results.back().sourceIndex = static_cast<uint32_t>(idx);
                    }
                }
            }
        }
    }
    SortCandidates(results);
    // 去重
    {
        std::unordered_set<std::wstring> seen;
        std::vector<CandidateItem> deduped;
        deduped.reserve(results.size());
        for (auto& item : results) {
            if (seen.insert(item.text).second) {
                deduped.push_back(std::move(item));
            }
        }
        results = std::move(deduped);
    }
    if (static_cast<int>(results.size()) > maxResults) {
        results.resize(maxResults);
    }
    return results;
}

// 模糊查询

std::vector<CandidateItem> WubiEngine::FuzzyQuery(const std::wstring& code, int maxResults) {
    std::shared_lock lock(m_mutex);
    if (code.empty() || maxResults <= 0) return {};
    std::wstring normCode = NormalizeCode(code);
    if (normCode.empty()) return {};
    std::vector<std::wstring> fuzzyCodes = GenerateFuzzyCodes(normCode);
    std::unordered_set<std::wstring> seen;
    std::vector<CandidateItem> results;
    for (const auto& fuzzyCode : fuzzyCodes) {
        auto it = m_codeIndex.find(fuzzyCode);
        if (it != m_codeIndex.end()) {
            for (size_t idx : it->second) {
                if (idx < m_entries.size()) {
                    const auto& entry = m_entries[idx];
                    if (seen.insert(entry.word).second) {
                        float fuzzyPenalty = 0.3f;
                        results.emplace_back(entry.word, CandidateCategory::WubiNormal,
                            static_cast<float>(entry.frequency) / 20000.0f * fuzzyPenalty);
                        results.back().sourceIndex = static_cast<uint32_t>(idx);
                    }
                }
            }
        }
    }
    SortCandidates(results);
    if (static_cast<int>(results.size()) > maxResults) {
        results.resize(maxResults);
    }
    return results;
}

// 词组联想

std::vector<CandidateItem> WubiEngine::Associate(const std::wstring& prefix, int maxResults) {
    std::shared_lock lock(m_mutex);
    if (prefix.empty() || maxResults <= 0) return {};
    std::vector<CandidateItem> results;
    std::wstring firstChar = prefix.substr(0, 1);
    auto it = m_assocIndex.find(firstChar);
    if (it != m_assocIndex.end()) {
        for (size_t idx : it->second) {
            if (idx < m_entries.size()) {
                const auto& entry = m_entries[idx];
                if (entry.word.size() > 1) {
                    results.emplace_back(entry.word, CandidateCategory::WubiNormal,
                        static_cast<float>(entry.frequency) / 20000.0f);
                    results.back().sourceIndex = static_cast<uint32_t>(idx);
                }
            }
        }
    }
    SortCandidates(results);
    if (static_cast<int>(results.size()) > maxResults) {
        results.resize(maxResults);
    }
    return results;
}

// 用户词管理

bool WubiEngine::AddCustomWord(const std::wstring& word, const std::wstring& code) {
    if (word.empty() || code.empty()) return false;
    std::wstring normCode = NormalizeCode(code);
    if (!IsValidWubiCode(normCode)) return false;
    std::unique_lock lock(m_mutex);
    // 检查是否已存在
    auto it = m_codeIndex.find(normCode);
    if (it != m_codeIndex.end()) {
        for (size_t idx : it->second) {
            if (idx < m_entries.size() && m_entries[idx].word == word) {
                return true;
            }
        }
    }
    AddEntry(normCode, word, 1000, true);
    TableEntry custom;
    custom.code = normCode;
    custom.word = word;
    custom.frequency = 1000;
    custom.isCustom = true;
    custom.isPinned = false;
    m_customEntries.push_back(std::move(custom));
    BuildAssocIndex();
    return true;
}

bool WubiEngine::PinWord(const std::wstring& word) {
    if (word.empty()) return false;
    std::unique_lock lock(m_mutex);
    m_pinnedWords.insert(word);
    for (auto& entry : m_entries) {
        if (entry.word == word) entry.isPinned = true;
    }
    return true;
}

bool WubiEngine::UnpinWord(const std::wstring& word) {
    if (word.empty()) return false;
    std::unique_lock lock(m_mutex);
    m_pinnedWords.erase(word);
    for (auto& entry : m_entries) {
        if (entry.word == word) entry.isPinned = false;
    }
    return true;
}

// 保存用户词库

bool WubiEngine::SaveUserTable(const std::wstring& path) {
    std::shared_lock lock(m_mutex);
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    // UTF-8 BOM
    const uint8_t bom[] = {0xEF, 0xBB, 0xBF};
    file.write(reinterpret_cast<const char*>(bom), 3);
    for (const auto& entry : m_customEntries) {
        std::string code = WideToUtf8(entry.code);
        std::string word = WideToUtf8(entry.word);
        file << code << '\t' << word << '\t' << entry.frequency << '\n';
    }
    file.close();
    return true;
}

// 向索引中添加条目

void WubiEngine::AddEntry(const std::wstring& code, const std::wstring& word,
                          uint32_t frequency, bool isCustom) {
    auto it = m_codeIndex.find(code);
    if (it != m_codeIndex.end()) {
        for (size_t idx : it->second) {
            if (idx < m_entries.size() && m_entries[idx].word == word) {
                if (frequency > m_entries[idx].frequency) {
                    m_entries[idx].frequency = frequency;
                }
                return;
            }
        }
    }
    TableEntry entry;
    entry.code = code;
    entry.word = word;
    entry.frequency = frequency;
    entry.isCustom = isCustom;
    entry.isPinned = (m_pinnedWords.count(word) > 0);
    size_t idx = m_entries.size();
    m_entries.push_back(std::move(entry));
    m_codeIndex[code].push_back(idx);
}

// 构建联想索引

void WubiEngine::BuildAssocIndex() {
    m_assocIndex.clear();
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].word.size() >= 2) {
            std::wstring firstChar = m_entries[i].word.substr(0, 1);
            m_assocIndex[firstChar].push_back(i);
        }
    }
}

// 排序候选结果

void WubiEngine::SortCandidates(std::vector<CandidateItem>& items) {
    std::stable_sort(items.begin(), items.end(),
        [](const CandidateItem& a, const CandidateItem& b) {
            return a.score > b.score;
        });
}

// 生成模糊编码列表

std::vector<std::wstring> WubiEngine::GenerateFuzzyCodes(const std::wstring& code) {
    std::vector<std::wstring> result;
    result.push_back(code);
    // 长度容错：补全最后一个字符
    if (code.size() < kWubiMaxCodeLen) {
        result.push_back(code + code.back());
    }
    // 长度容错：去掉最后一个字符
    if (code.size() > 1) {
        result.push_back(code.substr(0, code.size() - 1));
    }
    // 单字符替换容错
    for (size_t i = 0; i < code.size(); ++i) {
        auto it = m_fuzzyMap.find(code[i]);
        if (it != m_fuzzyMap.end()) {
            for (wchar_t replacement : it->second) {
                std::wstring fuzzy = code;
                fuzzy[i] = replacement;
                result.push_back(fuzzy);
            }
        }
    }
    // 去重
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

// 解析搜狗 .scel 二进制格式

bool WubiEngine::ParseScelFile(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    uint8_t header[1024] = {};
    file.read(reinterpret_cast<char*>(header), 1024);
    if (static_cast<size_t>(file.gcount()) < 1024) return false;
    uint32_t magic = *reinterpret_cast<uint32_t*>(header);
    if (magic != scel::kMagic) return false;
    // 读取拼音表
    uint16_t pyCount = 0;
    file.seekg(scel::kPinyinIndexOffset);
    file.read(reinterpret_cast<char*>(&pyCount), 2);
    struct PyIndexEntry { uint16_t offset; uint16_t length; };
    std::vector<PyIndexEntry> pyIndex(pyCount);
    for (uint16_t i = 0; i < pyCount; ++i) {
        file.read(reinterpret_cast<char*>(&pyIndex[i].offset), 2);
        file.read(reinterpret_cast<char*>(&pyIndex[i].length), 2);
    }
    std::vector<std::string> pinyins;
    for (uint16_t i = 0; i < pyCount; ++i) {
        file.seekg(scel::kPinyinDataOffset + pyIndex[i].offset);
        std::string py(pyIndex[i].length, '\0');
        file.read(py.data(), pyIndex[i].length);
        pinyins.push_back(py);
    }
    // 解析词条
    size_t wordOffset = scel::kIndexOffset;
    file.seekg(wordOffset);
    while (file.good()) {
        uint16_t samePyCount = 0;
        file.read(reinterpret_cast<char*>(&samePyCount), 2);
        if (!file.good()) break;
        uint16_t pyIndexLen = 0;
        file.read(reinterpret_cast<char*>(&pyIndexLen), 2);
        if (!file.good()) break;
        std::vector<uint16_t> pyIndices(pyIndexLen / 2);
        for (auto& idx : pyIndices) {
            file.read(reinterpret_cast<char*>(&idx), 2);
        }
        for (uint16_t s = 0; s < samePyCount; ++s) {
            uint16_t wordLen = 0;
            file.read(reinterpret_cast<char*>(&wordLen), 2);
            if (!file.good()) break;
            std::wstring word(wordLen / 2, L'\0');
            file.read(reinterpret_cast<char*>(word.data()), wordLen);
            uint32_t freq = 0;
            file.read(reinterpret_cast<char*>(&freq), 4);
            uint16_t extraLen = 0;
            file.read(reinterpret_cast<char*>(&extraLen), 2);
            if (extraLen > 0) {
                file.seekg(extraLen, std::ios::cur);
            }
            if (!pyIndices.empty() && pyIndices[0] < pinyins.size()) {
                const std::string& py = pinyins[pyIndices[0]];
                bool isWubi = true;
                for (char c : py) {
                    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
                        isWubi = false;
                        break;
                    }
                }
                if (isWubi && !word.empty()) {
                    std::wstring wcode = Utf8ToWide(py);
                    std::wstring normCode = NormalizeCode(wcode);
                    if (IsValidWubiCode(normCode)) {
                        AddEntry(normCode, word, freq, false);
                    }
                }
            }
        }
    }
    return true;
}

// 解析搜狗 .uwl 用户词库格式

bool WubiEngine::ParseUwlFile(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    uint8_t header[16] = {};
    file.read(reinterpret_cast<char*>(header), 16);
    if (static_cast<size_t>(file.gcount()) < 16) return false;
    uint32_t magic = *reinterpret_cast<uint32_t*>(header);
    if (magic != uwl::kMagic) return false;
    uint32_t wordCount = 0;
    file.seekg(4);
    file.read(reinterpret_cast<char*>(&wordCount), 4);
    file.seekg(uwl::kHeaderSize);
    for (uint32_t i = 0; i < wordCount && file.good(); ++i) {
        uint8_t codeLen = 0;
        file.read(reinterpret_cast<char*>(&codeLen), 1);
        std::string code(codeLen, '\0');
        file.read(code.data(), codeLen);
        uint16_t wordLen = 0;
        file.read(reinterpret_cast<char*>(&wordLen), 2);
        std::wstring word(wordLen / 2, L'\0');
        file.read(reinterpret_cast<char*>(word.data()), wordLen);
        uint32_t freq = 0;
        file.read(reinterpret_cast<char*>(&freq), 4);
        uint8_t flag = 0;
        file.read(reinterpret_cast<char*>(&flag), 1);
        if (!code.empty() && !word.empty()) {
            std::wstring wcode = Utf8ToWide(code);
            std::wstring normCode = NormalizeCode(wcode);
            if (IsValidWubiCode(normCode)) {
                AddEntry(normCode, word, freq, true);
                TableEntry custom;
                custom.code = normCode;
                custom.word = word;
                custom.frequency = freq;
                custom.isCustom = true;
                custom.isPinned = false;
                m_customEntries.push_back(std::move(custom));
            }
        }
    }
    return true;
}

// 解析纯文本格式（编码\t词\t词频）

bool WubiEngine::ParseTextFile(const std::wstring& path) {
    std::wifstream file(path);
    if (!file.is_open()) return false;
    // 检测并跳过 UTF-8 BOM
    wchar_t bom[1] = {};
    file.read(bom, 1);
    if (bom[0] != 0xFEFF) {
        file.seekg(0);
    }
    std::wstring line;
    size_t lineNum = 0;
    while (std::getline(file, line)) {
        ++lineNum;
        if (line.empty() || line[0] == L'#') continue;  // 跳过空行和注释
        // 按 Tab 分割
        std::vector<std::wstring> parts;
        std::wistringstream iss(line);
        std::wstring part;
        while (std::getline(iss, part, L'\t')) {
            parts.push_back(part);
        }
        if (parts.size() < 2) continue;  // 至少需要编码和词
        std::wstring code = NormalizeCode(parts[0]);
        if (!IsValidWubiCode(code)) continue;
        std::wstring word = parts[1];
        if (word.empty()) continue;
        uint32_t freq = 100;  // 默认词频
        if (parts.size() >= 3) {
            try {
                freq = static_cast<uint32_t>(std::stoul(parts[2]));
            } catch (...) {
                freq = 100;
            }
        }
        AddEntry(code, word, freq, false);
    }
    return lineNum > 0;
}

} // namespace qi
