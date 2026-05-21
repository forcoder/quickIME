# 知识库管理模块 (Module 3)

## 概述

知识库管理模块为 QuickInput 输入法提供本地知识存储和检索能力。用户可以将文档、笔记等导入到本地知识库中，并在输入时快速检索相关内容作为候选词。

## 功能特性

### 1. 文档导入
- **支持格式**: .txt, .md 文件
- **递归导入**: 可导入目录及其子目录
- **智能预处理**: 自动清理空白、格式化文本
- **分类管理**: 按类别组织知识条目（通用、工作、生活、商务、语录、代码话术）

### 2. 文本处理
- **智能分块**: 三级切分策略：
  - 段落边界切分（空行分隔）
  - 句子边界切分（。！？；…）
  - 固定长度兜底切分（默认 200 字符，重叠 40 字符）
- **文本预处理**: 去除多余空白、标准化格式

### 3. 向量化与检索
- **TF-IDF 嵌入**: 轻量级替代方案，无需外部模型
- **余弦相似度**: 计算文本相似度
- **关键词搜索**: 基于 SQLite FTS5 的全文搜索
- **分类过滤**: 可按知识类别筛选结果

### 4. 数据存储
- **SQLite 数据库**: 本地存储，无外部依赖
- **向量索引**: 暴力搜索实现，适合小规模数据
- **使用统计**: 记录条目访问频率，用于排序
- **备份/恢复**: JSON 格式导出/导入

## 核心类

### KnowledgeBase
主接口类，负责知识库的全生命周期管理。

#### 主要方法
- `Initialize(const std::wstring& dbPath)`: 初始化数据库
- `ImportFile()`: 导入单个文件
- `ImportDirectory()`: 导入整个目录
- `Search()`: 语义检索
- `KeywordSearch()`: 关键词搜索
- `GetStats()`: 获取分类统计

### TextChunker
智能文本分块器，保持语义完整性。

### TfidfEmbedding
TF-IDF 向量化实现。

### BruteForceIndex
向量索引，支持余弦相似度计算。

## 技术架构

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   文档导入      │    │   文本分块       │    │   向量化        │
│ (ImportFile)    │───>│ (ChunkText)      │───>│ (ComputeEmb)    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │                         │
                              ▼                         ▼
                    ┌──────────────────┐    ┌─────────────────┐
                    │   文本预处理     │    │   向量索引      │
                    │ (Preprocess)     │    │ (BruteForce)    │
                    └──────────────────┘    └─────────────────┘
                                                      │
                                                      ▼
                                            ┌─────────────────────┐
                                            │   检索              │
                                            │ (Search/Keyword)    │
                                            └─────────────────────┘
```

## 数据库设计

### entries 表
```sql
CREATE TABLE entries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    content TEXT NOT NULL,           -- 原始文本内容
    embedding BLOB,                  -- 向量嵌入（二进制）
    category INTEGER NOT NULL DEFAULT 0,  -- 类别
    source_file TEXT,                -- 来源文件
    created_at INTEGER NOT NULL,     -- 创建时间戳
    updated_at INTEGER NOT NULL,     -- 更新时间戳
    use_count INTEGER DEFAULT 0      -- 使用次数
);
```

### entries_fts 虚拟表（FTS5）
用于中文全文搜索。

## 使用示例

```cpp
#include "quickinput/knowledge/knowledge_base.h"

int main() {
    qi::knowledge::KnowledgeBase kb;

    // 初始化
    if (!kb.Initialize(L"C:\\QuickInput\\knowledge\\my_kb.db")) {
        return -1;
    }

    // 导入文档
    kb.ImportFile(L"C:\\docs\\manual.txt", qi::knowledge::KbCategory::Work);

    // 搜索
    auto results = kb.Search(L"如何配置输入法", 5);

    for (const auto& res : results) {
        wprintf(L"匹配: %s\n", res.entry.content.c_str());
        wprintf(L"相似度: %.3f\n", res.similarity);
    }

    kb.Shutdown();
    return 0;
}
```

## 编译要求

- C++20 标准
- SQLite3 开发库
- Windows SDK / TSF

CMake 会自动查找 sqlite3 库。如果系统中未安装，需要手动安装 SQLite3 开发包。

## 性能考虑

- 向量索引使用暴力搜索，适合条目数 < 1000 的场景
- 对于大规模知识库，未来可扩展倒排索引或近似最近邻算法
- TF-IDF 嵌入计算在导入时完成，搜索时只计算查询向量的相似度

## 扩展性

- 预留 MiniLM ONNX 模型接口，未来可替换为高质量语义嵌入
- 模块化设计，易于替换向量化和索引组件
- 支持多种知识源格式扩展