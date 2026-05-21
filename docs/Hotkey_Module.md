# QuickInput 快捷键逻辑模块 (模块7)

## 概述
模块7实现了QuickInput五笔输入法的快捷键管理系统，负责处理用户自定义的热键配置、热键注册与触发、以及与其他模块的通信。

## 文件结构

### 头文件
- `include/quickinput/core/hotkey_manager.h` - 快捷键管理器主接口
- `include/quickinput/core/config_types.h` - 配置类型定义
- `include/quickinput/core/utils.h` - 工具函数声明

### 源文件
- `src/core/hotkey_manager.cpp` - 快捷键管理器实现
- `src/core/config_types.cpp` - 配置类型实现
- `src/core/utils.cpp` - 工具函数实现
- `src/core/test_hotkey.cpp` - 测试程序

## 核心功能

### 1. 热键管理
- **单例模式**：线程安全的单例实现
- **热键注册**：使用Windows API RegisterHotKey/UnregisterHotKey
- **消息处理**：在窗口消息循环中处理WM_HOTKEY消息

### 2. 默认热键配置
| 热键组合 | 功能 | ID |
|---------|------|----|
| Ctrl+Alt+A | 切换AI联想开关 | 1001 |
| Ctrl+Alt+K | 切换知识库联想开关 | 1002 |
| Ctrl+Alt+. | 保存到知识库 | 1003 |
| Ctrl+Alt+, | 打开配置面板 | 1004 |
| Ctrl+Alt+M | 切换输入模式 | 1005 |

### 3. 配置管理
- **JSON格式**：配置文件使用JSON格式存储
- **配置验证**：自动验证配置参数的有效性
- **持久化**：支持配置的加载和保存
- **动态更新**：运行时可更新热键配置

### 4. 状态通知
- AI联想状态变化通知
- 知识库联想状态变化通知
- 输入模式变更通知
- 配置变更回调机制

## 技术特点

### 线程安全
- 所有公共方法都使用std::mutex进行保护
- 支持多线程环境下的安全访问

### 错误处理
- 详细的错误日志输出（通过OutputDebugStringW）
- 配置验证和异常捕获
- Windows API错误码记录

### 扩展性
- 配置变更回调机制
- 易于添加新的热键功能
- 模块化设计，便于集成到其他系统

## 使用方法

### 初始化
```cpp
qi::HotkeyManager& hotkeyMgr = qi::HotkeyManager::GetInstance();
if (!hotkeyMgr.Initialize(hMainWindow)) {
    // 初始化失败处理
}
```

### 消息处理
在主窗口的消息循环中调用：
```cpp
LRESULT result = hotkeyMgr.HandleMessage(uMsg, wParam, lParam);
```

### 配置管理
```cpp
// 获取当前配置
auto config = hotkeyMgr.GetConfig();

// 更新配置
qi::HotkeyConfig newConfig = /* ... */;
hotkeyMgr.UpdateConfig(newConfig);

// 设置配置变更回调
hotkeyMgr.SetConfigChangedCallback([]() {
    // 配置已变更的处理
});
```

### 状态查询
```cpp
bool aiEnabled = hotkeyMgr.IsAIAssociationEnabled();
bool kbEnabled = hotkeyMgr.IsKnowledgeBaseEnabled();
```

## 配置文件格式
```json
{
    "aiEnabled": true,
    "knowledgeEnabled": true,
    "aiSuggestionCount": 5,
    "kbWeight": 30,
    "triggerCharCount": 2,
    "suggestionDelayMs": 200,
    "hotkeyAICtrlAlt": 188,
    "hotkeyKBCtrlAlt": 190,
    "hotkeySaveCtrlAlt": 190,
    "hotkeyConfigCtrlAlt": 188,
    "hotkeyModeCtrlAlt": 186,
    "modifierCtrlAlt": 3,
    "inputMode": 1
}
```

## 依赖关系
- Windows API (RegisterHotKey, UnregisterHotKey)
- nlohmann/json for JSON处理
- C++17标准库
- 项目核心类型定义 (common.h)

## TODO
- [ ] 实现完整的日志系统
- [ ] 添加热键冲突检测
- [ ] 支持用户自定义热键映射
- [ ] 集成到主IME框架中