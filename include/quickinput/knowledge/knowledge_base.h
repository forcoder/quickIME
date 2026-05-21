# pragma once

#include "quickinput/core/common.h"
#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>

namespace qi::knowledge {

enum class KbCategory : uint8_t {
    General = 0,    // 通用
    Work,           // 工作
    Life,           // 生活
    Business,       // 商务
    Quotes,         // 语录
    CodeTalk,       // 代码话术
    // 客服场景分类
    FaqReply,       // FAQ 回复
    ComplaintHandle, // 投诉处理
    OrderInquiry,   // 订单咨询
    ProductInfo,    // 产品咨询
    ShippingInfo,   // 物流信息
    RefundProcess,  // 退款流程
    CategoryCount
};

struct KbEntry {
    int64_t         id;
    std::wstring    content;        // 原始文本内容
    std::string     embedding;      // 向量嵌入（二进制存储，float数组序列化）
    KbCategory      category;
    std::wstring    sourceFile;     // 来源文件
    int64_t         createdAt;
    int64_t         updatedAt;
    int             useCount;       // 使用次数（用于排序）

    KbEntry() : id(0), category(KbCategory::General), createdAt(0), updatedAt(0), useCount(0) {}
};

struct KbResult {
    KbEntry entry;
    float   similarity;  // 相似度分数
};

class KnowledgeBase {
public:
    KnowledgeBase();
    ~KnowledgeBase();

    // 初始化（打开/创建 SQLite 数据库）
    bool Initialize(const std::wstring& dbPath);
    void Shutdown();

    // ── 文档导入 ──
    // 支持 .txt, .md 格式
    bool ImportFile(const std::wstring& filePath, KbCategory category);
    bool ImportDirectory(const std::wstring& dirPath, KbCategory category, bool recursive = true);
    bool ImportText(const std::wstring& text, KbCategory category, const std::wstring& sourceName = L"");

    // ── 文本分块 ──
    // 智能分块：按段落、句子边界切分，保持语义完整
    std::vector<std::wstring> ChunkText(const std::wstring& text);

    // ── 向量化 ──
    // 使用简单的词袋模型 + TF-IDF 或调用嵌入模型
    std::vector<float> ComputeEmbedding(const std::wstring& text);

    // ── 检索 ──
    // 相似度检索（余弦相似度）
    std::vector<KbResult> Search(const std::wstring& query, int maxResults = 5, KbCategory filter = KbCategory::General);

    // 关键词精准匹配
    std::vector<KbResult> KeywordSearch(const std::wstring& keywords, int maxResults = 10);

    // 混合检索（语义+关键词组合）
    std::vector<KbResult> HybridSearch(
        const std::wstring& query,
        int maxResults = 5,
        KbCategory filter = KbCategory::General,
        float semanticWeight = 0.7f,
        float keywordWeight = 0.3f);

    // ── 管理 ──
    bool DeleteEntry(int64_t id);
    bool UpdateEntry(int64_t id, const std::wstring& newContent);
    bool SetCategory(int64_t id, KbCategory category);

    // 分类统计
    struct CategoryStats {
        KbCategory category;
        int64_t    entryCount;
        int64_t    totalSize;
    };
    std::vector<CategoryStats> GetStats();

    // 导出备份
    bool ExportBackup(const std::wstring& exportPath);
    bool ImportBackup(const std::wstring& backupPath);

    // 获取所有条目（分页）
    std::vector<KbEntry> GetEntries(int offset = 0, int limit = 100);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // 文本预处理
    std::wstring PreprocessText(const std::wstring& text);
    std::vector<std::wstring> Tokenize(const std::wstring& text);

    // 嵌入维度
    static constexpr int EMBEDDING_DIM = 384; // MiniLM 维度

    // SQLite 相关方法
    bool InitDatabase(sqlite3* db);
    bool CreateTables(sqlite3* db);
    bool InsertEntry(sqlite3* db, const KbEntry& entry);
    bool UpdateEntryInDb(sqlite3* db, int64_t id, const std::wstring& content);
    std::vector<KbEntry> QueryEntries(sqlite3* db, int offset = 0, int limit = 100);
    std::vector<KbEntry> QueryByKeyword(sqlite3* db, const std::wstring& keyword);
    std::vector<KbResult> QueryByEmbedding(
        sqlite3* db,
        const std::vector<float>& queryEmbedding,
        int maxResults,
        KbCategory filter);
    std::vector<KbEntry> QueryByCategory(sqlite3* db, KbCategory category);
    std::vector<CategoryStats> QueryStats(sqlite3* db);
    bool DeleteEntryFromDb(sqlite3* db, int64_t id);
    bool SetCategoryInDb(sqlite3* db, int64_t id, KbCategory category);
    bool IncrementUseCount(sqlite3* db, int64_t id);
};

} // namespace qi::knowledge