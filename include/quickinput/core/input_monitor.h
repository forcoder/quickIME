#pragma once

#include "quickinput/core/common.h"
#include <windows.h>
#include <functional>

namespace qi {

// 输入事件类型
enum class InputEventType : uint8_t {
    KeyDown,        // 按键按下
    KeyUp,          // 按键释放
    CharInput,      // 字符输入
    Composition,    // 编码输入
    Commit,         // 提交文本
    Delete,         // 删除操作
    Other           // 其他操作
};

// 输入事件结构
struct InputEvent {
    InputEventType type;        // 事件类型
    WPARAM wParam;              // 窗口消息参数
    LPARAM lParam;              // 窗口消息参数
    DWORD time;                 // 事件时间戳
    std::wstring text;          // 输入的文本（如果有）
    int scanCode;               // 扫描码
    bool isExtended;            // 是否扩展键
    bool isAltDown;             // Alt键状态
    bool isCtrlDown;            // Ctrl键状态
    bool isShiftDown;           // Shift键状态

    InputEvent() : type(InputEventType::Other), wParam(0), lParam(0), time(0),
                   scanCode(0), isExtended(false), isAltDown(false),
                   isCtrlDown(false), isShiftDown(false) {}
};

// 输入监控器回调函数类型
using InputEventCallback = std::function<void(const InputEvent& event)>;

// 输入监控器
class InputMonitor {
public:
    static InputMonitor& GetInstance();

    // 初始化监控
    bool Initialize();
    void Shutdown();

    // 设置事件回调
    void SetEventCallback(const InputEventCallback& callback);

    // 开始/停止监控
    bool StartMonitoring();
    void StopMonitoring();

    // 检查是否正在监控
    bool IsMonitoring() const { return m_isMonitoring; }

    // 获取最后的事件
    const InputEvent& GetLastEvent() const { return m_lastEvent; }

    // 判断是否为五笔编码输入模式
    bool IsWubiCompositionMode() const;

    // 判断是否为组合输入事件
    bool IsCompositionEvent(const InputEvent& event) const;

    // 判断是否为删除操作
    bool IsDeleteOperation(const InputEvent& event) const;

    // 判断是否为回车/换行
    bool IsEnterKey(const InputEvent& event) const;

    // 判断是否为退格键
    bool IsBackspaceKey(const InputEvent& event) const;

    // 获取当前输入上下文
    const InputContext& GetCurrentContext() const;

private:
    InputMonitor();
    ~InputMonitor();

    // 全局键盘钩子回调
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    // 处理键盘事件
    void HandleKeyboardEvent(const KBDLLHOOKSTRUCT* kbStruct, WPARAM wParam);

    // 解析修饰键状态
    void UpdateModifierKeys(const KBDLLHOOKSTRUCT* kbStruct, WPARAM wParam);

    // 创建输入事件
    InputEvent CreateInputEvent(WPARAM wParam, LPARAM lParam) const;

    // 线程安全
    mutable std::mutex m_mutex;

    // 事件回调
    InputEventCallback m_eventCallback;

    // 最后的事件
    InputEvent m_lastEvent;

    // 修饰键状态
    bool m_isAltDown = false;
    bool m_isCtrlDown = false;
    bool m_isShiftDown = false;

    // 监控状态
    bool m_isMonitoring = false;

    // 键盘钩子
    HHOOK m_keyboardHook = nullptr;

    // 上下文管理器引用
    ContextManager* m_contextManager = nullptr;

    // 单例
    static InputMonitor* s_instance;
};

} // namespace qi