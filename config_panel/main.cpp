#include <windows.h>
#include "config_window.h"
#include "../common/common.h"

// 全局变量
static qi::ui::ConfigWindow* g_pConfigWindow = nullptr;

// WinMain 入口点
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 创建配置窗口实例
    g_pConfigWindow = new qi::ui::ConfigWindow(hInstance);

    // 尝试创建主窗口
    if (!g_pConfigWindow->Create(nullptr))
    {
        MessageBox(nullptr, L"无法创建配置窗口", L"错误", MB_OK | MB_ICONERROR);
        delete g_pConfigWindow;
        g_pConfigWindow = nullptr;
        return FALSE;
    }

    // 显示窗口
    g_pConfigWindow->Show();

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理 GDI+
    GdiplusShutdown(gdiplusToken);

    // 删除窗口对象
    delete g_pConfigWindow;
    g_pConfigWindow = nullptr;

    return (int) msg.wParam;
}

// 窗口过程函数（如果需要）
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}