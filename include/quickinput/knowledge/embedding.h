# pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace qi::knowledge {

/**
 * @brief 嵌入模型接口（预留）
 */
class EmbeddingModel {
public:
    virtual ~EmbeddingModel() = default;

    /**
     * @brief 计算文本的向量嵌入
     * @param text 输入文本
     * @return float 数组，维度由模型决定
     */
    virtual std::vector<float> Compute(const std::wstring& text) = 0;
};

/**
 * @brief TF-IDF 嵌入实现（轻量替代方案）
 *
 * 使用词频(TF)和逆文档频率(IDF)构建稀疏向量。
 * 适用于知识库规模不大且不需要高质量语义的场景。
 */
class TfidfEmbedding : public EmbeddingModel {
public:
    explicit TfidfEmbedding(size_t dim = 384);
    ~TfidfEmbedding() override = default;

    /**
     * @brief 添加文档到语料库（用于 IDF 计算）
     */
    void AddDocument(const std::wstring& doc);

    /**
     * @brief 计算单个文本的 TF-IDF 向量
     */
    std::vector<float> Compute(const std::wstring& text) override;

    /**
     * @brief 获取当前向量维度
     */
    size_t GetDimension() const { return m_dim; }

private:
    size_t m_dim;
    std::unordered_map<std::wstring, double> m_idf; // term -> idf score
    size_t m_totalDocs = 0;

    /**
     * @brief 分词
     */
    std::vector<std::wstring> Tokenize(const std::wstring& text) const;

    /**
     * @brief 计算词频向量（归一化）
     */
    std::unordered_map<std::wstring, double> ComputeTf(const std::wstring& text) const;

    /**
     * @brief 将稀疏向量转换为固定长度稠密向量（补零或截断）
     */
    std::vector<float> SparseToDense(
        const std::unordered_map<std::wstring, double>& tf,
        const std::unordered_map<std::wstring, double>& idf) const;
};

/**
 * @brief MiniLM ONNX 模型加载器（预留接口）
 *
 * 未来可扩展支持加载本地 ONNX 模型文件进行语义嵌入。
 */
class MiniLMEmbedding : public EmbeddingModel {
public:
    explicit MiniLMEmbedding(const std::wstring& modelPath);
    ~MiniLMEmbedding() override;

    std::vector<float> Compute(const std::wstring& text) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace qi::knowledge