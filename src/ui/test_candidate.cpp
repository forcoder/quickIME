#include "quickinput/ui/candidate_manager.h"
#include <windows.h>
#include <iostream>
#include <vector>

// 测试程序：演示候选栏显示和交互

class TestCandidateApp {
public:
    TestCandidateApp() : m_hInstance(nullptr), m_hwndMain(nullptr) {
        Initialize();
    }

    ~TestCandidateApp() {
        Shutdown();
    }

    bool Initialize() {
        // 获取实例句柄
        m_hInstance = GetModuleHandle(nullptr);

        // 注册窗口类
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = StaticWndProc;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = L"TestCandidateClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

        if (!RegisterClassEx(&wc)) {
            return false;
        }

        // 创建主窗口
        m_hwndMain = CreateWindowEx(
            0,
            L"TestCandidateClass",
            L"QuickInput 候选栏测试",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
            nullptr,
            nullptr,
            m_hInstance,
            this
        );

        if (!m_hwndMain) {
            return false;
        }

        // 初始化候选管理器
        if (!m_candidateManager.Initialize(m_hwndMain)) {
            return false;
        }

        ShowWindow(m_hwndMain, SW_SHOW);
        UpdateWindow(m_hwndMain);

        return true;
    }

    void Shutdown() {
        if (m_hwndMain) {
            DestroyWindow(m_hwndMain);
            m_hwndMain = nullptr;
        }
    }

    // 模拟添加候选词
    void AddTestCandidates() {
        std::vector<CandidateItem> candidates;

        // 普通词汇
        candidates.push_back({L"你好", CandidateCategory::Word, 100});
        candidates.push_back({L"世界", CandidateCategory::Word, 95});
        candidates.push_back({L"编程", CandidateCategory::Word, 90});

        // AI生成词汇
        candidates.push_back({L"人工智能", CandidateCategory::AI, 85});
        candidates.push_back({L"机器学习", CandidateCategory::AI, 80});
        candidates.push_back({L"深度学习", CandidateCategory::AI, 75});

        // 知识库词汇
        candidates.push_back({L"科学", CandidateCategory::Knowledge, 70});
        candidates.push_back({L"技术", CandidateCategory::Knowledge, 65});
        candidates.push_back({L"创新", CandidateCategory::Knowledge, 60});

        // 高频词汇
        candidates.push_back({L"快速", CandidateCategory::Frequent, 55});
        candidates.push_back({L"高效", CandidateCategory::Frequent, 50});
        candidates.push_back({L"便捷", CandidateCategory::Frequent, 45});

        // 最近使用词汇
        candidates.push_back({L"测试", CandidateCategory::Recent, 40});
        candidates.push_back({L"开发", CandidateCategory::Recent, 35});
        candidates.push_back({L"设计", CandidateCategory::Recent, 30});

        m_candidateManager.AddCandidates(candidates);
    }

    // 性能测试
    void RunPerformanceTest() {
        std::cout << "开始性能测试..." << std::endl;

        // 测试大量候选词的性能
        std::vector<CandidateItem> largeCandidates;
        for (int i = 0; i < 1000; ++i) {
            largeCandidates.push_back({
                L"候选词" + std::to_wstring(i),
                static_cast<CandidateCategory>(i % 5),
                100 - i
            });
        }

        auto start = GetTickCount();
        m_candidateManager.AddCandidates(largeCandidates);
        auto end = GetTickCount();

        std::cout << "处理1000个候选词耗时: " << (end - start) << "ms" << std::endl;
        std::cout << "当前页数: " << m_candidateManager.GetTotalPages() << std::endl;
        std::cout << "完成性能测试" << std::endl;
    }

private:
    HINSTANCE m_hInstance;
    HWND m_hwndMain;
    qi::ui::CandidateManager m_candidateManager;

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam) {
        TestCandidateApp* pThis = nullptr;

        if (msg == WM_NCCREATE) {
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            pThis = static_cast<TestCandidateApp*>(lpcs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        } else {
            pThis = reinterpret_cast<TestCandidateApp*>(
                GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (pThis) {
            return pThis->WndProc(hwnd, msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_CREATE:
                // 创建测试按钮
                CreateTestButtons();
                break;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                // 绘制背景
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                FillRect(hdc, &rcClient, (HBRUSH)(COLOR_WINDOW + 1));

                // 绘制说明文字
                DrawInstructions(hdc);

                EndPaint(hwnd, &ps);
                break;
            }

            case WM_COMMAND:
                HandleCommand(wParam);
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                // 转发消息给候选管理器
                if (wParam == VK_UP || wParam == VK_DOWN ||
                    wParam == VK_LEFT || wParam == VK_RIGHT ||
                    wParam == VK_RETURN || wParam == VK_ESCAPE) {
                    m_candidateManager.HandleKeyDown(static_cast<WORD>(wParam));
                } else if (wParam >= '1' && wParam <= '9') {
                    m_candidateManager.HandleChar(static_cast<WCHAR>(wParam));
                } else if (msg == WM_MOUSEMOVE) {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);
                    m_candidateManager.HandleMouseMove(x, y);
                } else if (msg == WM_LBUTTONDOWN) {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);
                    m_candidateManager.HandleLButtonDown(x, y);
                } else if (msg == WM_LBUTTONUP) {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);
                    m_candidateManager.HandleLButtonUp(x, y);
                } else if (msg == WM_KEYDOWN) {
                    m_candidateManager.HandleKeyDown(static_cast<WORD>(wParam));
                } else if (msg == WM_CHAR) {
                    m_candidateManager.HandleChar(static_cast<WCHAR>(wParam));
                }

                return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        return 0;
    }

    void CreateTestButtons() {
        // 创建测试按钮（简化实现）
        // 实际应用中可以使用更复杂的UI框架
    }

    void DrawInstructions(HDC hdc) {
        RECT rcClient;
        GetClientRect(m_hwndMain, &rcClient);

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        TextOut(hdc, 10, 10, L"候选栏测试程序", 8);
        TextOut(hdc, 10, 30, L"按数字键1-9选择候选词", 12);
        TextOut(hdc, 10, 50, L"方向键导航，Enter确认", 11);
        TextOut(hdc, 10, 70, L"Esc隐藏候选栏", 7);

        // 显示当前状态
        std::wstring status = L"状态: ";
        switch (m_candidateManager.GetState()) {
            case qi::ui::CandidateState::Hidden:
                status += L"隐藏";
                break;
            case qi::ui::CandidateState::Empty:
                status += L"空";
                break;
            case qi::ui::CandidateState::Normal:
                status += L"正常";
                break;
            case qi::ui::CandidateState::Error:
                status += L"错误";
                break;
        }
        TextOut(hdc, 10, 90, status.c_str(), status.length());
    }

    void HandleCommand(WPARAM wParam) {
        // 处理按钮点击事件
        switch (LOWORD(wParam)) {
            case 1: // 添加测试候选词
                AddTestCandidates();
                InvalidateRect(m_hwndMain, nullptr, TRUE);
                break;

            case 2: // 清空候选词
                m_candidateManager.ClearCandidates();
                InvalidateRect(m_hwndMain, nullptr, TRUE);
                break;

            case 3: // 显示/隐藏候选栏
                if (m_candidateManager.GetState() == qi::ui::CandidateState::Hidden) {
                    m_candidateManager.Show();
                } else {
                    m_candidateManager.Hide();
                }
                InvalidateRect(m_hwndMain, nullptr, TRUE);
                break;

            case 4: // 运行性能测试
                RunPerformanceTest();
                break;
        }
    }
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR lpCmdLine,
                     _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    try {
        TestCandidateApp app;

        // 消息循环
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        GdiplusShutdown(gdiplusToken);
        return (int)msg.wParam;
    } catch (...) {
        GdiplusShutdown(gdiplusToken);
        return -1;
    }
}