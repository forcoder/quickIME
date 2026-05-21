#include "quickinput/ui/candidate_window.h"
#include <windowsx.h>

namespace qi::ui {

// 窗口类名
static const wchar_t* kCandidateWindowClass = L"QuickInput_CandidateWindow";

CandidateWindow::CandidateWindow()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_hParent(nullptr) {
}

CandidateWindow::~CandidateWindow() {
    Destroy();
}

bool CandidateWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // 注册窗口类
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kCandidateWindowClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    return true;
}

HWND CandidateWindow::Create(HWND hParent, int x, int y, int width, int height) {
    m_hParent = hParent;

    // 创建无边框透明窗口
    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        kCandidateWindowClass,
        L"Candidate Window",
        WS_POPUP,
        x, y, width, height,
        hParent,
        nullptr,
        m_hInstance,
        this  // 传递this指针给窗口过程
    );

    if (m_hwnd) {
        // 设置透明背景
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
        ShowWindow(m_hwnd, SW_SHOWNA);
        UpdateWindow(m_hwnd);
    }

    return m_hwnd;
}

void CandidateWindow::Destroy() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void CandidateWindow::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOWNA);
    }
}

void CandidateWindow::Hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void CandidateWindow::UpdatePosition(int x, int y) {
    if (m_hwnd) {
        SetWindowPos(m_hwnd, nullptr, x, y, 0, 0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void CandidateWindow::Invalidate() {
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void CandidateWindow::Render(HDC hdc) {
    if (m_hwnd) {
        PAINTSTRUCT ps;
        HDC hdcPaint = BeginPaint(m_hwnd, &ps);

        // 填充背景为透明
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        FillRect(hdcPaint, &rcClient, (HBRUSH)GetStockObject(NULL_BRUSH));

        // TODO: 这里可以调用候选词管理器的绘制方法
        // RenderCandidateManager(hdcPaint);

        EndPaint(m_hwnd, &ps);
    }
}

LRESULT CALLBACK CandidateWindow::WindowProc(HWND hwnd, UINT uMsg,
                                           WPARAM wParam, LPARAM lParam) {
    // 获取this指针
    CandidateWindow* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<CandidateWindow*>(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<CandidateWindow*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CandidateWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT:
            Render(nullptr);  // 实际应用中应该传入有效的hdc
            break;

        case WM_MOUSEMOVE:
            // 处理鼠标移动事件
            PostMessage(m_hParent, WM_MOUSEMOVE, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            // 处理鼠标左键按下事件
            PostMessage(m_hParent, WM_LBUTTONDOWN, wParam, lParam);
            break;

        case WM_LBUTTONUP:
            // 处理鼠标左键释放事件
            PostMessage(m_hParent, WM_LBUTTONUP, wParam, lParam);
            break;

        case WM_KEYDOWN:
            // 处理键盘按下事件
            PostMessage(m_hParent, WM_KEYDOWN, wParam, lParam);
            break;

        case WM_CHAR:
            // 处理字符输入事件
            PostMessage(m_hParent, WM_CHAR, wParam, lParam);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

} // namespace qi::ui