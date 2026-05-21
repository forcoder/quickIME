# QuickInput — 智能五笔输入法

基于 Windows TSF (Text Services Framework) 架构的定制五笔输入法，集成本地知识库 RAG 与 AI 实时联想。

## 核心特性

- **五笔引擎**：完整兼容 86 版 / 98 版五笔编码，支持简码、词组、固顶词、手动造词
- **本地知识库**：导入 txt / md / 笔记自动切片，向量化存储，输入时实时检索
- **AI 联想**：接入本地离线大模型，智能补全、短句建议、话术回复、文案续写
- **候选栏分类**：普通五笔词库 / 自定义词库 / 知识库内容 / AI 智能建议 四类分区
- **完全离线**：无网络可完整运行，联网可调用云端大模型增强效果
- **快捷键控制**：开关 AI 联想、开关知识库联想、自定义联想延迟

## 架构总览

```
┌─────────────────────────────────────────────────────┐
│                    QuickInput IME                     │
├──────────┬──────────┬───────────┬───────────────────┤
│  IME     │ 五笔编码  │ 候选栏 UI  │  配置管理          │
│ 框架层   │ 引擎层    │           │                   │
├──────────┴──────────┴───────────┴───────────────────┤
│              上下文管理器 (Context Manager)            │
├──────────────────┬──────────────────────────────────┤
│  知识库 RAG 模块  │  AI 推理模块                      │
│  (SQLite+向量)   │  (llama.cpp / ONNX)              │
├──────────────────┴──────────────────────────────────┤
│              存储层 (码表/知识库/配置)                 │
└─────────────────────────────────────────────────────┘
```

## 构建要求

- Windows 10/11 (x64)
- Visual Studio 2022 (MSVC v143) 或 Clang-CL
- CMake >= 3.20
- Windows SDK >= 10.0.22621
- WiX Toolset v3.x (安装包)

## 构建步骤

```powershell
# 克隆项目
git clone --recursive https://github.com/your-repo/quickInput.git
cd quickInput

# 生成构建系统
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build build --config Release

# 安装（注册输入法）
cmake --install build --config Release

# 运行测试
ctest --test-dir build -C Release
```

## 模块说明

| 模块 | 目录 | 说明 |
|------|------|------|
| core/ | src/core/ | 核心数据结构、事件总线、上下文管理 |
| ime/ | src/ime/ | TSF 输入法框架实现 |
| ui/ | src/ui/ | 候选栏 UI、输入面板 |
| knowledge/ | src/knowledge/ | 知识库导入、切片、向量检索 |
| ai/ | src/ai/ | 本地 LLM 推理、AI 联想 |
| config/ | src/config/ | 配置管理、用户设置 |
| utils/ | src/utils/ | 工具函数、日志、编码转换 |

## 配置

配置文件位于 `%APPDATA%\QuickInput\config.json`：

```json
{
    "wubi_version": "86",
    "ai_enabled": true,
    "knowledge_enabled": true,
    "suggestion_delay_ms": 300,
    "candidate_page_size": 5,
    "knowledge_paths": [
        "C:/Users/you/Documents/notes"
    ],
    "ai_model_path": "C:/QuickInput/models/llama-3-8b-q4.gguf"
}
```

## 快捷键

| 快捷键 | 功能 |
|--------|------|
| Ctrl+Shift+A | 开关 AI 联想 |
| Ctrl+Shift+K | 开关知识库联想 |
| Ctrl+Shift+S | 保存当前输入到知识库 |
| Ctrl+Shift+, | 打开配置面板 |

## 许可证

MIT License
