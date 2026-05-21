# 知识库模块快速参考

## 文件结构

```
include/quickinput/knowledge/
├── embedding.h           # 嵌入模型接口与 TF-IDF 实现
├── knowledge_base.h      # 知识库主接口
├── text_chunker.h        # 智能文本分块器
└── vector_index.h        # 向量索引接口

src/knowledge/
├── embedding.cpp         # 嵌入实现
├── knowledge_base.cpp    # 知识库核心实现
├── test_kb.cpp          # 测试程序
├── text_chunker.cpp     # 分块器实现
└── vector_index.cpp     # 索引实现

KNOWLEDGE_MODULE.md     # 详细设计文档
KNOWLEDGE_QUICKREF.md   # 本快速参考
```

## 枚举类型

### KbCategory
```cpp
enum class KbCategory : uint8_t {
    General = 0,    // 通用
    Work,           // 工作
    Life,           // 生活
    Business,       // 商务
    Quotes,         // 语录
    CodeTalk,       // 代码话术
    CategoryCount
};
```

## 数据结构

### KbEntry
```cpp
struct KbEntry {
    int64_t         id;             // 唯一标识符
    std::wstring    content;        // 原始文本内容
    std::string     embedding;      // 二进制向量数据
    KbCategory      category;       // 类别
    std::wstring    sourceFile;     // 来源文件路径
    int64_t         createdAt;      // Unix 时间戳（秒）
    int64_t         updatedAt;      // Unix 时间戳（秒）
    int             useCount;       // 使用次数
};
```

### KbResult
```cpp
struct KbResult {
    KbEntry entry;          // 条目信息
    float   similarity;     // 相似度分数 (0.0 - 1.0)
};
```

## 核心类：KnowledgeBase

### 构造/析构
- `KnowledgeBase()`: 构造函数
- `~KnowledgeBase()`: 析构函数（自动关闭数据库）
- `Initialize(const std::wstring& dbPath)`: 初始化数据库
- `Shutdown()`: 关闭数据库连接

### 文档导入
- `ImportFile(const std::wstring& filePath, KbCategory category)`
  - 支持 .txt, .md 格式
  - 自动分块和向量化
- `ImportDirectory(const std::wstring& dirPath, KbCategory category, bool recursive = true)`
  - 递归导入子目录
- `ImportText(const std::wstring& text, KbCategory category, const std::wstring& sourceName = L"")`
  - 直接导入文本

### 检索方法
- `Search(const std::wstring& query, int maxResults = 5, KbCategory filter = KbCategory::General)`
  - 语义相似度搜索
  - 可选类别过滤
- `KeywordSearch(const std::wstring& keywords, int maxResults = 10)`
  - 基于 FTS5 的全文搜索

### 管理操作
- `DeleteEntry(int64_t id)`: 删除指定条目
- `UpdateEntry(int64_t id, const std::wstring& newContent)`: 更新内容
- `SetCategory(int64_t id, KbCategory category)`: 更改类别
- `GetStats()`: 获取分类统计
- `ExportBackup(const std::wstring& exportPath)`: JSON 导出
- `ImportBackup(const std::wstring& backupPath)`: JSON 导入
- `GetEntries(int offset = 0, int limit = 100)`: 分页查询

## 工具类

### TextChunker
```cpp
struct ChunkConfig {
    size_t maxSize = 200;       // 最大块大小
    size_t overlap = 40;        // 重叠大小
    bool respectParagraph = true;
    bool respectSentence = true;
};

std::vector<std::wstring> Chunk(const std::wstring& text);
```

### TfidfEmbedding
```cpp
void AddDocument(const std::wstring& doc);           // 添加到语料库
std::vector<float> Compute(const std::wstring& text); // 计算向量
```

### BruteForceIndex
```cpp
bool Add(const VectorItem& item);                     // 添加条目
bool Remove(int64_t id);                              // 删除条目
std::vector<VectorItem> Search(                       // 相似度搜索
    const std::vector<float>& query,
    size_t maxResults);
```

## 编译选项

在 CMakeLists.txt 中设置：
```cmake
option(QI_BUILD_TESTS "Build unit tests" ON)
```

测试程序会自动编译为 `test_kb` 可执行文件。

## 典型使用流程

```cpp
#include "quickinput/knowledge/knowledge_base.h"

// 1. 创建实例
qi::knowledge::KnowledgeBase kb;

// 2. 初始化数据库
if (!kb.Initialize(L"C:\\QuickInput\\knowledge\\my_kb.db")) {
    return -1;
}

// 3. 导入知识
kb.ImportFile(L"C:\\docs\\work_manual.txt", qi::knowledge::KbCategory::Work);
kb.ImportDirectory(L"C:\\notes\\personal", qi::knowledge::KbCategory::Life);

// 4. 执行搜索
auto results = kb.Search(L"如何备份数据", 10);

// 5. 显示结果
for (const auto& res : results) {
    wprintf(L"匹配: %s\n", res.entry.content.c_str());
}

// 6. 清理资源
kb.Shutdown();
```

## 注意事项

1. **数据库路径**: 建议使用绝对路径，确保有写入权限
2. **文件大小**: 单个文本块建议 < 1KB，过大会降低性能
3. **编码**: 所有文本按 UTF-16 处理，确保源文件为 UTF-8 编码
4. **线程安全**: 当前实现非线程安全，多线程访问需加锁
5. **内存占用**: 向量索引会占用约 (条目数 × 维度 × 4 bytes) 内存

## 故障排除

### 编译错误：未找到 sqlite3
- 确保系统中安装了 SQLite3 开发库
- Windows 可使用 vcpkg 安装：`vcpkg install sqlite3:x64-windows`

### 导入文件失败
- 检查文件扩展名是否为 .txt 或 .md
- 验证文件是否可读且非空
- 检查磁盘空间是否充足

### 搜索返回空结果
- 确认已先导入文档
- 检查查询关键词是否匹配内容
- 尝试 `KeywordSearch` 进行精确匹配

## API 变更历史

- v1.0: 初始版本
  - 支持 .txt/.md 导入
  - TF-IDF 向量化
  - 暴力搜索索引
  - SQLite FTS5 关键词搜索