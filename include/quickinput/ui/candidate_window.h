#ifndef QUICKINPUT_UI_CANDIDATE_WINDOW_H_
#define QUICKINPUT_UI_CANDIDATE_WINDOW_H_

#include <windows.h>

namespace qi::ui {

// 候选栏窗口类（独立窗口）
class CandidateWindow {
public:
    CandidateWindow();
    ~CandidateWindow();

    // 初始化窗口
    bool Initialize(HINSTANCE hInstance);

    // 创建候选栏窗口
    HWND Create(HWND hParent, int x, int y, int width, int height);

    // 销毁窗口
    void Destroy();

    // 显示/隐藏窗口
    void Show();
    void Hide();
    void UpdatePosition(int x, int y);

    // 重绘窗口
    void Invalidate();
    void Render(HDC hdc);

private:
    // 窗口过程
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam);

    // 实际窗口过程
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 窗口句柄
    HWND m_hwnd;

    // 实例句柄
    HINSTANCE m_hInstance;

    // 父窗口句柄
    HWND m_hParent;
};

} // namespace qi::ui

#endif // QUICKINPUT_UI_CANDIDATE_WINDOW_H_