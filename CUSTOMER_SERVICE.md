# QuickIME 客服场景功能文档

## 概述

QuickIME 现已支持客服场景的智能回复建议功能，类似于搜狗输入法的"知识库+AI生成建议回复"模式。

## 功能特性

### 1. 客服场景分类

新增以下知识库分类：

| 分类 | 说明 |
|------|------|
| `FaqReply` | FAQ 回复 |
| `ComplaintHandle` | 投诉处理 |
| `OrderInquiry` | 订单咨询 |
| `ProductInfo` | 产品咨询 |
| `ShippingInfo` | 物流信息 |
| `RefundProcess` | 退款流程 |

### 2. AI 建议类型

新增以下 AI 建议类型：

| 类型 | 说明 | 模板特点 |
|------|------|----------|
| `CustomerService` | 客服回复 | 简洁、礼貌、专业 |
| `FaqReply` | FAQ 回复 | 基于知识库匹配 |
| `ComplaintHandle` | 投诉处理 | 安抚+解决方案 |
| `OrderInquiry` | 订单咨询 | 提供查询指引 |
| `RefundProcess` | 退款流程 | 流程说明+时间 |

### 3. 混合检索

支持语义+关键词混合检索：

```cpp
// 语义权重 0.7 + 关键词权重 0.3
auto results = kb.HybridSearch(query, 5, KbCategory::FaqReply, 0.7f, 0.3f);
```

### 4. 客服场景管理器

`CustomerServiceManager` 是核心组件，协调知识库和 AI：

```cpp
// 初始化
CustomerServiceManager& csMgr = CustomerServiceManager::GetInstance();
csMgr.Initialize(modelPath, kbPath);

// 生成建议
auto suggestions = csMgr.GenerateSuggestions(customerMessage, 5);
```

### 5. 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Alt+1~9` | 快速选择建议 |
| `Alt+S` | 发送当前建议 |
| `Alt+R` | 刷新建议 |

## 使用方法

### 初始化知识库

```cpp
// 导入 FAQ 文件
csMgr.ImportKbFile(L"faq.md", KbCategory::FaqReply);

// 导入投诉处理话术
csMgr.ImportKbFile(L"complaints.md", KbCategory::ComplaintHandle);

// 导入订单咨询
csMgr.ImportKbText(L"订单正在处理中，预计3-5个工作日内发货。",
                   KbCategory::OrderInquiry);
```

### 生成客服建议

```cpp
// 客户发送消息
std::wstring customerMsg = L"我想查一下我的订单";

auto suggestions = csMgr.GenerateSuggestions(customerMsg, 5);
for (const auto& s : suggestions) {
    wcout << s.text << L" [" << s.confidence << L"]" << endl;
}
```

### 集成到输入法

在 `KeyHandler` 中集成：

```cpp
// Ctrl+Alt+C: 触发客服建议
if (m_state.isCtrlDown && m_state.isAltDown && wParam == 'C') {
    auto& ctxMgr = ContextManager::GetInstance();
    std::wstring context = ctxMgr.ExtractConversationContext();
    auto suggestions = csMgr.GenerateSuggestions(context);
    // 显示到候选栏
}
```

## 配置

```json
{
    "customer_service": {
        "ai_enabled": true,
        "kb_enabled": true,
        "ai_weight": 0.5,
        "kb_weight": 0.5,
        "max_suggestions": 5,
        "min_confidence": 0.3
    }
}
```

## 模块结构

```
include/quickinput/
├── cs/
│   └── cs_manager.h      # 客服场景管理器
├── ai/
│   └── inference_engine.h # AI 推理引擎（已增强客服场景）
├── knowledge/
│   └── knowledge_base.h  # 知识库（已增加客服分类）
└── core/
    └── context_manager.h # 上下文管理（已增加对话提取）

src/
├── cs/
│   └── cs_manager.cpp    # 客服场景管理器实现
├── ai/
│   └── inference_engine.cpp  # AI 推理实现
├── knowledge/
│   └── knowledge_base.cpp   # 知识库实现
└── core/
    └── context_manager.cpp  # 上下文管理实现
```

## 注意事项

1. **模型加载**：AI 功能需要加载模型文件（.gguf 或 .onnx）
2. **知识库导入**：首次使用需要导入客服话术知识库
3. **权限要求**：需要管理员权限注册 TSF 输入法
