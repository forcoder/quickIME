#include "quickinput/knowledge/vector_index.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace qi::knowledge {

BruteForceIndex::BruteForceIndex(size_t dim) : m_dim(dim)
{
}

bool BruteForceIndex::Add(const VectorItem& item) {
    // 检查维度一致性
    if (item.embedding.size() != m_dim) {
        return false;
    }

    // 查找是否已存在相同 ID
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [&](const VectorItem& i) { return i.id == item.id; });

    if (it != m_items.end()) {
        // 更新现有条目
        it->embedding = item.embedding;
        return true;
    } else {
        // 添加新条目
        m_items.push_back(item);
        return true;
    }
}

bool BruteForceIndex::Remove(int64_t id) {
    auto it = std::remove_if(m_items.begin(), m_items.end(),
        [&](const VectorItem& item) { return item.id == id; });
    if (it != m_items.end()) {
        m_items.erase(it, m_items.end());
        return true;
    }
    return false;
}

std::vector<VectorItem> BruteForceIndex::Search(
    const std::vector<float>& queryEmbedding,
    size_t maxResults) {

    std::vector<std::pair<int64_t, float>> scores;

    for (const auto& item : m_items) {
        float sim = CosineSimilarity(queryEmbedding, item.embedding);
        if (sim > 1e-6f) { // 过滤极小相似度
            scores.emplace_back(item.id, sim);
        }
    }

    // 按相似度降序排序
    std::sort(scores.begin(), scores.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // 限制结果数量并返回
    size_t n = (std::min)(scores.size(), maxResults);
    std::vector<VectorItem> results;
    results.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        int64_t id = scores[i].first;
        // 找到对应条目
        auto it = std::find_if(m_items.begin(), m_items.end(),
            [&](const VectorItem& item) { return item.id == id; });
        if (it != m_items.end()) {
            VectorItem result = *it;
            result.similarity = scores[i].second;
            results.push_back(std::move(result));
        }
    }

    return results;
}

float BruteForceIndex::CosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b) {

    if (a.empty() || b.empty()) return 0.0f;

    double dot = 0.0;
    double normA = 0.0;
    double normB = 0.0;

    size_t n = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        dot += static_cast<double>(a[i]) * b[i];
        normA += static_cast<double>(a[i]) * a[i];
        normB += static_cast<double>(b[i]) * b[i];
    }

    if (normA < 1e-8 || normB < 1e-8) return 0.0f;

    return static_cast<float>(dot / (std::sqrt(normA) * std::sqrt(normB)));
}

} // namespace qi::knowledge