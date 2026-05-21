#include "quickinput/knowledge/knowledge_base.h"
#include "quickinput/knowledge/text_chunker.h"
#include "quickinput/knowledge/embedding.h"
#include "quickinput/knowledge/vector_index.h"
#include "quickinput/core/common.h"
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <cstring>

namespace qi::knowledge {

struct KnowledgeBase::Impl {
    sqlite3*            db = nullptr;
    std::unique_ptr<TfidfEmbedding> embeddingModel;
    std::unique_ptr<BruteForceIndex> vectorIndex;
    TextChunker chunker;

    Impl() : embeddingModel(std::make_unique<TfidfEmbedding>(EMBEDDING_DIM)),
             vectorIndex(std::make_unique<BruteForceIndex>(EMBEDDING_DIM)) {}

    ~Impl() {
        if (db) {
            sqlite3_close(db);
        }
    }
};

KnowledgeBase::KnowledgeBase()
    : m_impl(std::make_unique<Impl>())
{
}

KnowledgeBase::~KnowledgeBase()
{
}

bool KnowledgeBase::Initialize(const std::wstring& dbPath) {
    // 确保数据库目录存在
    std::wstring dir = dbPath.substr(0, dbPath.find_last_of(L"\\/"));
    CreateDirectoryW(dir.c_str(), nullptr);

    int rc = sqlite3_open16(dbPath.c_str(), &m_impl->db);
    if (rc != SQLITE_OK) {
        return false;
    }

    return InitDatabase(m_impl->db);
}

void KnowledgeBase::Shutdown() {
    m_impl.reset();
}

std::vector<std::wstring> KnowledgeBase::ChunkText(const std::wstring& text) {
    return m_impl->chunker.Chunk(text);
}

std::vector<float> KnowledgeBase::ComputeEmbedding(const std::wstring& text) {
    auto vec = m_impl->embeddingModel->Compute(text);
    // 添加文档到语料库（用于 IDF）
    m_impl->embeddingModel->AddDocument(text);
    return vec;
}

std::vector<KbResult> KnowledgeBase::Search(
    const std::wstring& query,
    int maxResults,
    KbCategory filter) {

    if (!m_impl->db) return {};

    // 计算查询向量
    auto queryVec = ComputeEmbedding(query);

    // 从向量索引检索
    auto candidates = m_impl->vectorIndex->Search(queryVec, static_cast<size_t>(maxResults * 5));

    std::vector<KbResult> results;

    // 批量查询条目详细信息
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, content, category, source_file, created_at, updated_at, use_count FROM entries WHERE id IN (";

    // 构建 IN 子句（最多 500 个）
    std::string inClause;
    size_t count = 0;
    for (const auto& item : candidates) {
        if (count >= 500) break;
        if (count > 0) inClause += ",";
        inClause += std::to_string(item.id);
        ++count;
    }
    inClause += ")";
    sql += inClause;

    int rc = sqlite3_prepare_v2(m_impl->db, sql.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        // 执行查询
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            KbEntry entry;
            entry.id = sqlite3_column_int64(stmt, 0);
            entry.content = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
            entry.category = static_cast<KbCategory>(sqlite3_column_int(stmt, 2));
            entry.sourceFile = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 3));
            entry.createdAt = sqlite3_column_int64(stmt, 4);
            entry.updatedAt = sqlite3_column_int64(stmt, 5);
            entry.useCount = sqlite3_column_int(stmt, 6);

            // 检查类别过滤
            if (filter != KbCategory::General &&
                static_cast<uint8_t>(filter) != static_cast<uint8_t>(entry.category)) {
                continue;
            }

            // 找到对应的候选项
            auto cit = std::find_if(candidates.begin(), candidates.end(),
                [&](const VectorItem& vi) { return vi.id == entry.id; });
            if (cit != candidates.end()) {
                KbResult res;
                res.entry = entry;
                res.similarity = cit->similarity;
                results.push_back(res);
            }
        }
        sqlite3_finalize(stmt);
    }

    // 更新使用计数
    for (auto& res : results) {
        IncrementUseCount(m_impl->db, res.entry.id);
    }

    // 按分数排序并截断
    std::sort(results.begin(), results.end(),
        [](const KbResult& a, const KbResult& b) {
            return a.similarity > b.similarity;
        });

    if (results.size() > static_cast<size_t>(maxResults)) {
        results.resize(maxResults);
    }

    return results;
}

std::vector<KbResult> KnowledgeBase::KeywordSearch(
    const std::wstring& keywords,
    int maxResults) {

    if (!m_impl->db) return {};

    // 构建 FTS5 查询（支持中文分词）
    std::wstring ftsQuery = Utf8ToWide(keywords) + L"*";

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT rowid, content FROM entries_fts WHERE content MATCH ? ORDER BY rank LIMIT ?";

    int rc = sqlite3_prepare_v2(m_impl->db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return {};

    sqlite3_bind_text16(stmt, 1, ftsQuery.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, maxResults);

    std::vector<KbResult> results;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        KbEntry entry;
        entry.id = sqlite3_column_int64(stmt, 0);
        entry.content = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));

        KbResult res;
        res.entry = entry;
        res.similarity = 1.0f; // FTS5 匹配分数
        results.push_back(res);
    }

    sqlite3_finalize(stmt);

    // 更新使用计数
    for (auto& res : results) {
        IncrementUseCount(m_impl->db, res.entry.id);
    }

    return results;
}

bool KnowledgeBase::DeleteEntry(int64_t id) {
    if (!m_impl->db) return false;
    return DeleteEntryFromDb(m_impl->db, id);
}

bool KnowledgeBase::UpdateEntry(int64_t id, const std::wstring& newContent) {
    if (!m_impl->db) return false;
    return UpdateEntryInDb(m_impl->db, id, newContent);
}

bool KnowledgeBase::SetCategory(int64_t id, KbCategory category) {
    if (!m_impl->db) return false;
    return SetCategoryInDb(m_impl->db, id, category);
}

std::vector<KnowledgeBase::CategoryStats> KnowledgeBase::GetStats() {
    if (!m_impl->db) return {};
    return QueryStats(m_impl->db);
}

bool KnowledgeBase::ExportBackup(const std::wstring& exportPath) {
    if (!m_impl->db) return false;

    auto entries = GetEntries(0, -1); // 获取全部
    std::ofstream file(WideToUtf8(exportPath), std::ios::binary);
    if (!file) return false;

    // JSON 格式备份
    file << "{\n  \"version\": \"1.0\",\n  \"entries\": [\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        file << "    {\n";
        file << "      \"id\": " << e.id << ",\n";
        file << "      \"content\": \"" << WideToUtf8(e.content) << "\",\n";
        file << "      \"category\": " << static_cast<int>(e.category) << ",\n";
        file << "      \"sourceFile\": \"" << WideToUtf8(e.sourceFile) << "\"\n";
        file << "    }";
        if (i < entries.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n}\n";

    return true;
}

bool KnowledgeBase::ImportBackup(const std::wstring& backupPath) {
    // TODO: 实现 JSON 导入
    return false;
}

std::vector<KbEntry> KnowledgeBase::GetEntries(int offset, int limit) {
    if (!m_impl->db) return {};
    return QueryEntries(m_impl->db, offset, limit);
}

std::wstring KnowledgeBase::PreprocessText(const std::wstring& text) {
    std::wstring cleaned = text;

    // 移除多余空白
    std::wstring result;
    bool inSpace = false;
    for (wchar_t c : cleaned) {
        if (std::iswspace(c)) {
            if (!inSpace && !result.empty()) {
                result += L' ';
                inSpace = true;
            }
        } else {
            result += c;
            inSpace = false;
        }
    }

    // 去除首尾空白
    size_t start = result.find_first_not_of(L" \t\r\n");
    size_t end = result.find_last_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return L"";

    return result.substr(start, end - start + 1);
}

std::vector<std::wstring> KnowledgeBase::Tokenize(const std::wstring& text) {
    return m_impl->embeddingModel->Tokenize(text);
}

// SQLite 相关方法实现

bool KnowledgeBase::InitDatabase(sqlite3* db) {
    return CreateTables(db);
}

bool KnowledgeBase::CreateTables(sqlite3* db) {
    // 创建条目表
    const char* sql_entries =
        "CREATE TABLE IF NOT EXISTS entries ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "content TEXT NOT NULL,"
        "embedding BLOB,"
        "category INTEGER NOT NULL DEFAULT 0,"
        "source_file TEXT,"
        "created_at INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL,"
        "use_count INTEGER DEFAULT 0"
        ");";

    // 创建 FTS5 虚拟表用于中文全文搜索
    const char* sql_fts =
        "CREATE VIRTUAL TABLE IF NOT EXISTS entries_fts USING fts5("
        "content,"
        "tokenize=porter"
        ");";

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql_entries, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        return false;
    }

    rc = sqlite3_exec(db, sql_fts, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        return false;
    }

    // 创建触发器：插入时同步到 FTS
    const char* trigger_insert =
        "CREATE TRIGGER IF NOT EXISTS entries_ai AFTER INSERT ON entries "
        "BEGIN "
        "INSERT INTO entries_fts(rowid, content) VALUES (new.rowid, new.content); "
        "END;";

    const char* trigger_update =
        "CREATE TRIGGER IF NOT EXISTS entries_au AFTER UPDATE ON entries "
        "BEGIN "
        "INSERT OR REPLACE INTO entries_fts(rowid, content) VALUES (old.rowid, new.content); "
        "END;";

    rc = sqlite3_exec(db, trigger_insert, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) return false;

    rc = sqlite3_exec(db, trigger_update, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) return false;

    return true;
}

bool KnowledgeBase::InsertEntry(sqlite3* db, const KbEntry& entry) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO entries (content, embedding, category, source_file, created_at, updated_at, use_count)"
        "VALUES (?, ?, ?, ?, ?, ?, ?)";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    // 绑定参数
    sqlite3_bind_text16(stmt, 1, entry.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, entry.embedding.data(), static_cast<int>(entry.embedding.size()), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(entry.category));
    sqlite3_bind_text16(stmt, 4, entry.sourceFile.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, entry.createdAt);
    sqlite3_bind_int64(stmt, 6, entry.updatedAt);
    sqlite3_bind_int(stmt, 7, entry.useCount);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool KnowledgeBase::UpdateEntryInDb(sqlite3* db, int64_t id, const std::wstring& content) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE entries SET content = ?, updated_at = ?, use_count = use_count + 1 WHERE id = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    sqlite3_bind_text16(stmt, 1, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int64(stmt, 3, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<KbEntry> KnowledgeBase::QueryEntries(sqlite3* db, int offset, int limit) {
    std::vector<KbEntry> entries;
    sqlite3_stmt* stmt = nullptr;

    std::string sql = "SELECT id, content, embedding, category, source_file, created_at, updated_at, use_count FROM entries ORDER BY updated_at DESC";
    if (offset >= 0 && limit > 0) {
        sql += " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);
    }

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return entries;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        KbEntry entry;
        entry.id = sqlite3_column_int64(stmt, 0);

        const void* blob = sqlite3_column_blob(stmt, 2);
        int blobSize = sqlite3_column_bytes(stmt, 2);
        entry.embedding.assign(static_cast<const char*>(blob), blobSize);

        entry.content = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
        entry.category = static_cast<KbCategory>(sqlite3_column_int(stmt, 3));
        entry.sourceFile = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 4));
        entry.createdAt = sqlite3_column_int64(stmt, 5);
        entry.updatedAt = sqlite3_column_int64(stmt, 6);
        entry.useCount = sqlite3_column_int(stmt, 7);

        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

bool KnowledgeBase::DeleteEntryFromDb(sqlite3* db, int64_t id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM entries WHERE id = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool KnowledgeBase::SetCategoryInDb(sqlite3* db, int64_t id, KbCategory category) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE entries SET category = ? WHERE id = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, static_cast<int>(category));
    sqlite3_bind_int64(stmt, 2, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<KnowledgeBase::CategoryStats> KnowledgeBase::QueryStats(sqlite3* db) {
    std::vector<CategoryStats> stats;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT category, COUNT(*), SUM(LENGTH(content)) "
        "FROM entries GROUP BY category";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return stats;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        CategoryStats s;
        s.category = static_cast<KbCategory>(sqlite3_column_int(stmt, 0));
        s.entryCount = sqlite3_column_int64(stmt, 1);
        s.totalSize = sqlite3_column_int64(stmt, 2);
        stats.push_back(s);
    }

    sqlite3_finalize(stmt);
    return stats;
}

bool KnowledgeBase::IncrementUseCount(sqlite3* db, int64_t id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE entries SET use_count = use_count + 1 WHERE id = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

// 文档导入方法实现

bool KnowledgeBase::ImportFile(const std::wstring& filePath, KbCategory category) {
    // 检查文件扩展名
    std::wstring ext = filePath.substr(filePath.find_last_of(L".\\") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != L"txt" && ext != L"md") {
        return false;
    }

    // 读取文件内容
    std::ifstream file(WideToUtf8(filePath));
    if (!file) return false;

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    std::wstring wtext = Utf8ToWide(content);
    if (wtext.empty()) return false;

    // 预处理
    wtext = PreprocessText(wtext);

    // 分块
    auto chunks = ChunkText(wtext);

    // 向量化并插入
    for (const auto& chunk : chunks) {
        if (chunk.size() < 10) continue; // 过滤过短块

        auto vec = ComputeEmbedding(chunk);
        std::string binVec(reinterpret_cast<const char*>(vec.data()),
                           vec.size() * sizeof(float));

        KbEntry entry;
        entry.content = chunk;
        entry.embedding = binVec;
        entry.category = category;
        entry.sourceFile = filePath;
        entry.createdAt = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        entry.updatedAt = entry.createdAt;
        entry.useCount = 0;

        if (InsertEntry(m_impl->db, entry)) {
            // 添加到向量索引
            VectorItem vi(entry.id);
            vi.embedding.assign(vec.begin(), vec.end());
            m_impl->vectorIndex->Add(vi);
        }
    }

    return true;
}

bool KnowledgeBase::ImportDirectory(const std::wstring& dirPath, KbCategory category, bool recursive) {
    namespace fs = std::filesystem;
    bool success = true;

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::wstring path = entry.path().wstring();
            std::wstring ext = path.substr(path.find_last_of(L".\\") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == L"txt" || ext == L"md") {
                if (!ImportFile(path, category)) {
                    success = false;
                }
            }
        } else if (recursive && entry.is_directory()) {
            ImportDirectory(entry.path().wstring(), category, true);
        }
    }

    return success;
}

bool KnowledgeBase::ImportText(const std::wstring& text, KbCategory category, const std::wstring& sourceName) {
    auto chunks = ChunkText(text);
    bool success = true;

    for (const auto& chunk : chunks) {
        if (chunk.size() < 10) continue;

        auto vec = ComputeEmbedding(chunk);
        std::string binVec(reinterpret_cast<const char*>(vec.data()),
                           vec.size() * sizeof(float));

        KbEntry entry;
        entry.content = chunk;
        entry.embedding = binVec;
        entry.category = category;
        entry.sourceFile = sourceName;
        entry.createdAt = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        entry.updatedAt = entry.createdAt;
        entry.useCount = 0;

        if (InsertEntry(m_impl->db, entry)) {
            VectorItem vi(entry.id);
            vi.embedding.assign(vec.begin(), vec.end());
            m_impl->vectorIndex->Add(vi);
        } else {
            success = false;
        }
    }

    return success;
}

} // namespace qi::knowledge