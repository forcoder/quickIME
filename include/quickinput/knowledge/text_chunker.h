#pragma once

#include <string>
#include <vector>

namespace qi::knowledge {

/**
 * @brief 文本分块配置
 */
struct ChunkConfig {
    size_t maxSize       = 200;  ///< 最大块大小（字符数）
    size_t overlap       = 40;   ///< 重叠大小（字符数）
    bool   respectParagraph = true;  ///< 优先按段落边界切分
    bool   respectSentence  = true;  ///< 其次按句子边界切分
};

/**
 * @brief 智能文本分块器
 *
 * 三级切分策略：
 *   1. 按段落边界（空行）切分
 *   2. 按句子边界（。！？；）切分
 *   3. 固定长度兜底切分
 *
 * 相邻块之间保持指定重叠，避免语义在边界处断裂。
 */
class TextChunker {
public:
    explicit TextChunker(const ChunkConfig& config = ChunkConfig{});
    ~TextChunker() = default;

    /**
     * @brief 对输入文本进行智能分块
     * @param text  原始文本（已预处理）
     * @return 分块后的文本列表
     */
    std::vector<std::wstring> Chunk(const std::wstring& text) const;

    /**
     * @brief 更新分块配置
     */
    void SetConfig(const ChunkConfig& config);

private:
    /**
     * @brief 按段落边界切分（以连续空行为分隔）
     */
    std::vector<std::wstring> SplitByParagraph(const std::wstring& text) const;

    /**
     * @brief 按句子边界切分（。！？；…）
     */
    std::vector<std::wstring> SplitBySentence(const std::wstring& text) const;

    /**
     * @brief 固定长度兜底切分（带重叠）
     */
    std::vector<std::wstring> SplitByFixedLength(const std::wstring& text) const;

    /**
     * @brief 为相邻块添加重叠区域
     */
    void AddOverlap(std::vector<std::wstring>& chunks) const;

    ChunkConfig m_config;
};

} // namespace qi::knowledge
