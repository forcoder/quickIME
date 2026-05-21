# pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace qi::knowledge {

class EmbeddingModel;

/**
 * @brief 向量索引项
 */
struct VectorItem {
    int64_t     id;
    std::vector<float> embedding;   // 嵌入向量
    float       similarity = 0.0f;  // 相似度分数（用于检索结果排序）

    VectorItem() = default;
    explicit VectorItem(int64_t i) : id(i) {}
};

/**
 * @brief 向量索引接口
 *
 * 支持：
 *   - 添加/删除条目
 *   - 暴力搜索（适合数据量小）
 *   - 余弦相似度计算
 *   - 可选倒排索引加速
 */
class VectorIndex {
public:
    virtual ~VectorIndex() = default;

    /**
     * @brief 添加一个向量条目
     * @param item 向量条目
     * @return 是否成功
     */
    virtual bool Add(const VectorItem& item) = 0;

    /**
     * @brief 删除指定 ID 的条目
     * @param id 条目 ID
     * @return 是否成功
     */
    virtual bool Remove(int64_t id) = 0;

    /**
     * @brief 搜索最相似的 N 个条目
     * @param queryEmbedding 查询向量
     * @param maxResults 最大返回数量
     * @return 搜索结果（按相似度降序）
     */
    virtual std::vector<VectorItem> Search(
        const std::vector<float>& queryEmbedding,
        size_t maxResults) = 0;
};

/**
 * @brief 暴力搜索实现（适合小规模数据）
 *
 * 直接遍历所有条目计算余弦相似度。
 * 当条目数 < 1000 时性能足够好。
 */
class BruteForceIndex : public VectorIndex {
public:
    explicit BruteForceIndex(size_t dim);
    ~BruteForceIndex() override = default;

    bool Add(const VectorItem& item) override;
    bool Remove(int64_t id) override;
    std::vector<VectorItem> Search(
        const std::vector<float>& queryEmbedding,
        size_t maxResults) override;

    /**
     * @brief 获取当前条目数量
     */
    size_t Size() const { return m_items.size(); }

private:
    /**
     * @brief 计算余弦相似度
     */
    static float CosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b);

    size_t m_dim;
    std::vector<VectorItem> m_items;
};

} // namespace qi::knowledge