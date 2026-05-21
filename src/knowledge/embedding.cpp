#include "quickinput/knowledge/embedding.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_set>

namespace qi::knowledge {

TfidfEmbedding::TfidfEmbedding(size_t dim)
    : m_dim(dim)
{
}

void TfidfEmbedding::AddDocument(const std::wstring& doc) {
    auto tokens = Tokenize(doc);
    std::unordered_set<std::wstring> uniqueTokens(tokens.begin(), tokens.end());
    for (const auto& token : uniqueTokens) {
        ++m_idf[token]; // 计数文档频率
    }
    ++m_totalDocs;
}

std::vector<float> TfidfEmbedding::Compute(const std::wstring& text) {
    auto tokens = Tokenize(text);
    auto tf = ComputeTf(text);

    // 计算 TF-IDF 权重
    std::unordered_map<std::wstring, double> tfidf;
    for (const auto& [term, freq] : tf) {
        double idf = std::log(static_cast<double>(m_totalDocs) / (m_idf[term] + 1));
        tfidf[term] = freq * idf;
    }

    return SparseToDense(tf, tfidf);
}

std::vector<std::wstring> TfidfEmbedding::Tokenize(const std::wstring& text) const {
    std::vector<std::wstring> tokens;
    std::wstring current;

    for (wchar_t c : text) {
        if (std::iswalnum(c) || c == L'_' || c == L'-') {
            current += c;
        } else {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    // 过滤停用词（简单版）
    static const std::unordered_set<std::wstring> stopWords = {
        L"的", L"了", L"和", L"是", L"在", L"我", L"有", L"你", L"他", L"这",
        L"就", L"也", L"都", L"要", L"而", L"及", L"或", L"与", L"为", L"以",
        L"a", L"an", L"the", L"and", L"or", L"but", L"in", L"on", L"at", L"to",
        L"for", L"of", L"with", L"by", L"from", L"up", L"about", L"into",
        L"through", L"during", L"before", L"after", L"above", L"below",
        L"between", L"among", L"within", L"without", L"against", L"along",
        L"across", L"behind", L"beneath", L"beside", L"near", L"over",
        L"under", L"around", L"among", L"upon", L"within", L"outside",
        L"beyond", L"except", L"including", L"per", L"via", L"vs", L"etc"
    };

    tokens.erase(
        std::remove_if(tokens.begin(), tokens.end(),
            [&](const std::wstring& t) { return stopWords.count(t) > 0; }),
        tokens.end());

    return tokens;
}

std::unordered_map<std::wstring, double> TfidfEmbedding::ComputeTf(const std::wstring& text) const {
    auto tokens = Tokenize(text);
    std::unordered_map<std::wstring, int> counts;
    int total = tokens.size();

    for (const auto& token : tokens) {
        ++counts[token];
    }

    std::unordered_map<std::wstring, double> tf;
    for (const auto& [term, count] : counts) {
        tf[term] = static_cast<double>(count) / total;
    }

    return tf;
}

std::vector<float> TfidfEmbedding::SparseToDense(
    const std::unordered_map<std::wstring, double>& tf,
    const std::unordered_map<std::wstring, double>& tfidf) const {

    std::vector<float> vec(m_dim, 0.0f);

    // 使用哈希值将 term 映射到向量位置
    for (const auto& [term, value] : tfidf) {
        size_t pos = std::hash<std::wstring>{}(term) % m_dim;
        vec[pos] = static_cast<float>(value);
    }

    // L2 归一化
    float norm = 0.0f;
    for (float v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);

    if (norm > 1e-6f) {
        for (auto& v : vec) {
            v /= norm;
        }
    }

    return vec;
}

// MiniLMEmbedding 实现占位（未来扩展）
MiniLMEmbedding::MiniLMEmbedding(const std::wstring&) {
    // TODO: 加载 ONNX Runtime 模型
}

MiniLMEmbedding::~MiniLMEmbedding() = default;

std::vector<float> MiniLMEmbedding::Compute(const std::wstring&) {
    // TODO: 调用 ONNX 推理
    return std::vector<float>(384, 0.0f); // 返回零向量作为占位
}

} // namespace qi::knowledge