#include "quickinput/core/hotkey_manager.h"
// 日志系统占位符 - 实际项目中应实现完整的日志系统
#include <iostream>
#include <thread>

// 简单的测试窗口类
class TestWindow {
public:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_HOTKEY: {
                qi::HotkeyManager& hotkeyMgr = qi::HotkeyManager::GetInstance();
                return hotkeyMgr.HandleMessage(uMsg, wParam, lParam);
            }
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    bool Create() {
        WNDCLASS wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"TestWindowClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClass(&wc)) {
            LOG_ERROR(L"注册窗口类失败");
            return false;
        }

        m_hwnd = CreateWindowEx(
            0,
            L"TestWindowClass",
            L"QuickInput 热键测试窗口",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr
        );

        if (!m_hwnd) {
            LOG_ERROR(L"创建窗口失败");
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    void RunMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    HWND GetHandle() const { return m_hwnd; }

private:
    HWND m_hwnd = nullptr;
};

int main() {
    // 日志系统占位符 - 实际项目中应实现完整的日志系统

    std::cout << "=== QuickInput 热键管理器测试程序 ===" << std::endl;

    // 创建测试窗口
    TestWindow window;
    if (!window.Create()) {
        std::cerr << "创建测试窗口失败" << std::endl;
        return 1;
    }

    // 获取热键管理器实例
    qi::HotkeyManager& hotkeyMgr = qi::HotkeyManager::GetInstance();

    // 初始化热键管理器
    if (!hotkeyMgr.Initialize(window.GetHandle())) {
        std::cerr << "初始化热键管理器失败" << std::endl;
        return 1;
    }

    // 设置配置变更回调
    hotkeyMgr.SetConfigChangedCallback([]() {
        std::cout << "配置已变更，请检查热键状态" << std::endl;
    });

    // 显示当前配置
    auto config = hotkeyMgr.GetConfig();
    std::wcout << L"当前配置:" << std::endl;
    std::wcout << L"  AI联想: " << (config.aiEnabled ? L"启用" : L"禁用") << std::endl;
    std::wcout << L"  知识库: " << (config.knowledgeEnabled ? L"启用" : L"禁用") << std::endl;
    std::wcout << L"  AI建议数: " << config.aiSuggestionCount << std::endl;
    std::wcout << L"  知识库权重: " << config.kbWeight << std::endl;
    std::wcout << L"  触发字数: " << config.triggerCharCount << std::endl;
    std::wcout << L"  联想延迟: " << config.suggestionDelayMs << "ms" << std::endl;
    std::wcout << L"  输入模式: " << static_cast<int>(config.inputMode) << std::endl;

    std::cout << "\n=== 热键说明 ===" << std::endl;
    std::cout << "Ctrl+Alt+A: 切换AI联想开关" << std::endl;
    std::cout << "Ctrl+Alt+K: 切换知识库联想开关" << std::endl;
    std::cout << "Ctrl+Alt+.: 保存到知识库" << std::endl;
    std::cout << "Ctrl+Alt+,: 打开配置面板" << std::endl;
    std::cout << "Ctrl+Alt+M: 切换输入模式" << std::endl;

    std::cout << "\n按任意键退出测试..." << std::endl;
    std::cin.get();

    // 清理
    hotkeyMgr.Shutdown();

    std::cout << "测试程序结束" << std::endl;

    return 0;
}