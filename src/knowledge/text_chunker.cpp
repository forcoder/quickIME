#include "quickinput/knowledge/text_chunker.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace qi::knowledge {

TextChunker::TextChunker(const ChunkConfig& config)
    : m_config(config)
{
}

void TextChunker::SetConfig(const ChunkConfig& config) {
    m_config = config;
}

std::vector<std::wstring> TextChunker::Chunk(const std::wstring& text) const {
    if (text.empty()) return {};

    // 第一级：按段落切分
    std::vector<std::wstring> paragraphs = SplitByParagraph(text);

    // 第二级：对过长的段落按句子切分
    std::vector<std::wstring> sentences;
    sentences.reserve(paragraphs.size());

    if (m_config.respectSentence) {
        for (const auto& para : paragraphs) {
            if (para.size() <= m_config.maxSize) {
                sentences.push_back(para);
            } else {
                auto parts = SplitBySentence(para);
                sentences.insert(sentences.end(), parts.begin(), parts.end());
            }
        }
    } else {
        sentences = std::move(paragraphs);
    }

    // 第三级：对仍然过长的句子按固定长度切分
    std::vector<std::wstring> chunks;
    chunks.reserve(sentences.size());

    for (const auto& sent : sentences) {
        if (sent.size() <= m_config.maxSize) {
            chunks.push_back(sent);
        } else {
            auto parts = SplitByFixedLength(sent);
            chunks.insert(chunks.end(), parts.begin(), parts.end());
        }
    }

    // 添加重叠区域
    if (m_config.overlap > 0 && chunks.size() > 1) {
        AddOverlap(chunks);
    }

    // 过滤空块
    chunks.erase(
        std::remove_if(chunks.begin(), chunks.end(),
            [](const std::wstring& c) { return c.empty(); }),
        chunks.end());

    return chunks;
}

std::vector<std::wstring> TextChunker::SplitByParagraph(const std::wstring& text) const {
    std::vector<std::wstring> paragraphs;
    std::wstring current;
    size_t i = 0;

    while (i < text.size()) {
        // 读取一行
        size_t lineEnd = text.find(L'\n', i);
        if (lineEnd == std::wstring::npos) {
            lineEnd = text.size();
        }

        std::wstring line = text.substr(i, lineEnd - i);

        // 去除行尾 \r
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }

        // 判断是否为空行
        bool isEmptyLine = std::all_of(line.begin(), line.end(),
            [](wchar_t c) { return std::iswspace(c); });

        if (isEmptyLine) {
            // 空行 => 段落边界
            if (!current.empty()) {
                // 去除首尾空白
                size_t start = current.find_first_not_of(L" \t\r\n");
                size_t end = current.find_last_not_of(L" \t\r\n");
                if (start != std::wstring::npos) {
                    paragraphs.push_back(current.substr(start, end - start + 1));
                }
                current.clear();
            }
        } else {
            if (!current.empty()) {
                current += L'\n';
            }
            current += line;
        }

        i = lineEnd + 1;
    }

    // 处理最后一段
    if (!current.empty()) {
        size_t start = current.find_first_not_of(L" \t\r\n");
        size_t end = current.find_last_not_of(L" \t\r\n");
        if (start != std::wstring::npos) {
            paragraphs.push_back(current.substr(start, end - start + 1));
        }
    }

    return paragraphs;
}

std::vector<std::wstring> TextChunker::SplitBySentence(const std::wstring& text) const {
    std::vector<std::wstring> sentences;
    std::wstring current;

    // 句子结束标记：。！？；…
    auto isSentenceEnd = [](wchar_t c) -> bool {
        return c == L'。' || c == L'！' || c == L'？' || c == L'；' || c == L'…';
    };

    for (size_t i = 0; i < text.size(); ++i) {
        current += text[i];

        if (isSentenceEnd(text[i])) {
            // 检查是否是省略号 …（可能连续多个）
            if (text[i] == L'…' && i + 1 < text.size() && text[i + 1] == L'…') {
                continue;
            }

            // 如果当前累积超过最大大小，先切分
            if (current.size() >= m_config.maxSize) {
                sentences.push_back(std::move(current));
                current.clear();
            }
        }

        // 强制切分：超过最大大小的 1.5 倍时强制断开
        if (current.size() >= m_config.maxSize * 3 / 2) {
            sentences.push_back(std::move(current));
            current.clear();
        }
    }

    if (!current.empty()) {
        sentences.push_back(std::move(current));
    }

    // 合并过短的句子（低于 maxSize 的 1/4）
    std::vector<std::wstring> merged;
    std::wstring accumulator;

    for (auto& sent : sentences) {
        if (accumulator.empty()) {
            accumulator = std::move(sent);
        } else if (accumulator.size() + sent.size() <= m_config.maxSize) {
            accumulator += sent;
        } else {
            merged.push_back(std::move(accumulator));
            accumulator = std::move(sent);
        }
    }
    if (!accumulator.empty()) {
        merged.push_back(std::move(accumulator));
    }

    return merged;
}

std::vector<std::wstring> TextChunker::SplitByFixedLength(const std::wstring& text) const {
    std::vector<std::wstring> chunks;
    const size_t step = (m_config.maxSize > m_config.overlap)
        ? (m_config.maxSize - m_config.overlap)
        : m_config.maxSize;

    for (size_t i = 0; i < text.size(); i += step) {
        size_t len = (std::min)(m_config.maxSize, text.size() - i);
        chunks.push_back(text.substr(i, len));
    }

    return chunks;
}

void TextChunker::AddOverlap(std::vector<std::wstring>& chunks) const {
    if (chunks.size() <= 1 || m_config.overlap == 0) return;

    // 从后往前处理，避免索引偏移问题
    for (int i = static_cast<int>(chunks.size()) - 1; i > 0; --i) {
        const auto& prev = chunks[i - 1];
        if (prev.size() > m_config.overlap) {
            std::wstring overlap = prev.substr(prev.size() - m_config.overlap);
            chunks[i] = overlap + chunks[i];
        }
    }
}

} // namespace qi::knowledge
