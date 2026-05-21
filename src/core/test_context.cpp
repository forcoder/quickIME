#include "quickinput/core/context_manager.h"
#include "quickinput/core/input_monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

// 简单的测试程序，演示上下文抓取功能

void PrintContext(const qi::InputContext& context, const std::wstring& title) {
    std::wcout << L"\n=== " << title << L" ===" << std::endl;
    std::wcout << L"五笔编码: " << context.composition << std::endl;
    std::wcout << L"已上屏文本: " << context.committedText << std::endl;
    std::wcout << L"周围文本: " << (context.surroundingText.length() > 100 ?
                                   context.surroundingText.substr(0, 100) + L"..." :
                                   context.surroundingText) << std::endl;
    std::wcout << L"光标位置: " << context.cursorPos << std::endl;
    std::wcout << L"中文模式: " << (context.isChineseMode ? L"是" : L"否") << std::endl;
    std::wcout << L"全角模式: " << (context.isFullWidth ? L"是" : L"否") << std::endl;
    std::wcout << L"输入模式: " << static_cast<int>(context.mode) << std::endl;
}

void TestContextManager() {
    std::wcout << L"\n🧪 测试 ContextManager 模块" << std::endl;

    // 获取单例实例
    auto& contextMgr = qi::ContextManager::GetInstance();

    // 初始化
    if (!contextMgr.Initialize()) {
        std::wcerr << L"❌ 上下文管理器初始化失败" << std::endl;
        return;
    }

    std::wcout << L"✅ 上下文管理器初始化成功" << std::endl;

    // 创建模拟窗口句柄（在实际应用中会从系统获取）
    HWND testWindow = GetDesktopWindow();

    // 更新上下文
    if (contextMgr.UpdateContext(testWindow)) {
        const auto& context = contextMgr.GetContext();
        PrintContext(context, L"初始上下文");
    }

    // 设置一些测试数据
    contextMgr.SetComposition(L"asdf");
    contextMgr.SetCommittedText(L"你好");
    contextMgr.SetCursorPos(5);
    contextMgr.SetMode(qi::InputMode::Hybrid);
    contextMgr.SetChineseMode(true);
    contextMgr.SetFullWidthMode(false);

    // 再次获取上下文
    const auto& updatedContext = contextMgr.GetContext();
    PrintContext(updatedContext, L"更新后的上下文");

    // 测试窗口信息获取
    std::wcout << L"\n📋 窗口信息:" << std::endl;
    std::wcout << L"标题: " << contextMgr.GetWindowTitle() << std::endl;
    std::wcout << L"类名: " << contextMgr.GetClassName() << std::endl;

    // 测试文本提取
    std::wcout << L"\n🔍 文本提取测试:" << std::endl;
    std::wstring aroundCursor = contextMgr.GetTextAroundCursor(10, 20);
    std::wcout << L"光标前后文本: " << aroundCursor << std::endl;

    // 测试段落提取
    std::wcout << L"\n📝 段落提取测试:" << std::endl;
    std::wstring sampleText = L"第一段文字。\n这是第二段。\n\n第三段单独成行。";
    auto paragraphs = contextMgr.ExtractParagraphs(sampleText);
    for (size_t i = 0; i < paragraphs.size(); ++i) {
        std::wcout << L"段落 " << (i + 1) << L": " << paragraphs[i] << std::endl;
    }

    // 清理
    contextMgr.Shutdown();
}

void TestInputMonitor() {
    std::wcout << L"\n🎯 测试 InputMonitor 模块" << std::endl;

    // 获取单例实例
    auto& inputMonitor = qi::InputMonitor::GetInstance();

    // 初始化
    if (!inputMonitor.Initialize()) {
        std::wcerr << L"❌ 输入监控器初始化失败" << std::endl;
        return;
    }

    std::wcout << L"✅ 输入监控器初始化成功" << std::endl;

    // 设置事件回调
    inputMonitor.SetEventCallback([](const qi::InputEvent& event) {
        std::wcout << L"📥 收到输入事件:" << std::endl;
        std::wcout << L"  类型: " << static_cast<int>(event.type) << std::endl;
        std::wcout << L"  虚拟键码: " << event.wParam << std::endl;
        std::wcout << L"  扫描码: " << event.scanCode << std::endl;
        std::wcout << L"  时间: " << event.time << std::endl;
        std::wcout << L"  文本: " << event.text << std::endl;
        std::wcout << L"  Alt按下: " << event.isAltDown << std::endl;
        std::wcout << L"  Ctrl按下: " << event.isCtrlDown << std::endl;
        std::wcout << L"  Shift按下: " << event.isShiftDown << std::endl;
        std::wcout << L"  扩展键: " << event.isExtended << std::endl;
    });

    // 开始监控
    if (inputMonitor.StartMonitoring()) {
        std::wcout << L"✅ 输入监控已启动" << std::endl;

        // 模拟一些键盘事件（在实际应用中会由系统触发）
        std::wcout << L"💡 提示: 请在运行此程序时实际按键，观察输出" << std::endl;

        // 等待一小段时间让用户测试
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // 停止监控
        inputMonitor.StopMonitoring();
        std::wcout << L"✅ 输入监控已停止" << std::endl;
    } else {
        std::wcerr << L"❌ 无法启动输入监控" << std::endl;
    }

    // 清理
    inputMonitor.Shutdown();
}

int main() {
    // 设置控制台支持Unicode输出
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::wcout << L"🚀 QuickInput 上下文抓取模块测试程序" << std::endl;
    std::wcout << L"===========================================" << std::endl;

    try {
        // 测试上下文管理器
        TestContextManager();

        // 测试输入监控器
        TestInputMonitor();

        std::wcout << L"\n🎉 所有测试完成!" << std::endl;

    } catch (const std::exception& e) {
        std::wcerr << L"❌ 测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::wcerr << L"❌ 测试过程中发生未知异常" << std::endl;
        return 1;
    }

    return 0;
}