#include "config_window.h"
#include <windowsx.h>
#include <commdlg.h>
#include <shlobj.h>
#include <fstream>
#include <string>
#include <vector>
#include "../common/common.h"

namespace qi::ui {

// GDI+ 全局变量
static ULONG_PTR g_gdiplusToken = 0;

// 字符串资源表
const std::map<int, const wchar_t*> ConfigWindow::StringTable = {
    {IDS_APP_TITLE, L"QuickInput 配置面板"},
    {IDS_GENERAL, L"常规设置"},
    {IDS_HOTKEYS, L"快捷键设置"},
    {IDS_KNOWLEDGE, L"知识库设置"},
    {IDS_AI, L"AI 设置"}
};

ConfigWindow::ConfigWindow(HINSTANCE hInstance)
    : m_hInstance(hInstance)
{
    // 初始化GDI+
    GdiplusStartupInput startupInput;
    GdiplusStartup(&g_gdiplusToken, &startupInput, nullptr);
}

ConfigWindow::~ConfigWindow()
{
    if (g_gdiplusToken != 0)
    {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

bool ConfigWindow::Create(HWND hParent, int x, int y, int width, int height)
{
    // 注册窗口类
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = L"QuickInputConfigWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&wc))
    {
        return false;
    }

    // 创建窗口
    m_hwnd = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        L"QuickInputConfigWindow",
        GetStringResource(IDS_APP_TITLE),
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        x, y, width, height,
        hParent, nullptr, m_hInstance, this);

    if (!m_hwnd)
    {
        return false;
    }

    // 创建标签页控件
    CreateTabControl();

    // 创建按钮
    RECT rcClient;
    GetClientRect(m_hwnd, &rcClient);

    int btnWidth = 80;
    int btnHeight = 25;
    int spacing = 10;

    m_btnOK = CreateWindow(
        L"BUTTON", L"确定",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rcClient.right - btnWidth * 2 - spacing * 3,
        rcClient.bottom - btnHeight - spacing,
        btnWidth, btnHeight,
        m_hwnd, (HMENU)ID_BTN_OK, m_hInstance, nullptr);

    m_btnCancel = CreateWindow(
        L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rcClient.right - btnWidth - spacing,
        rcClient.bottom - btnHeight - spacing,
        btnWidth, btnHeight,
        m_hwnd, (HMENU)ID_BTN_CANCEL, m_hInstance, nullptr);

    m_btnApply = CreateWindow(
        L"BUTTON", L"应用",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rcClient.right - btnWidth * 3 - spacing * 4,
        rcClient.bottom - btnHeight - spacing,
        btnWidth, btnHeight,
        m_hwnd, (HMENU)ID_BTN_APPLY, m_hInstance, nullptr);

    // 加载配置
    LoadConfiguration();
    UpdateUIFromConfig();

    return true;
}

void ConfigWindow::Show()
{
    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    UpdateWindow(m_hwnd);
}

void ConfigWindow::Hide()
{
    ShowWindow(m_hwnd, SW_HIDE);
}

void ConfigWindow::Destroy()
{
    DestroyWindow(m_hwnd);
    m_hwnd = nullptr;
}

LRESULT ConfigWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        return OnCommand(wParam, lParam);
    case WM_NOTIFY:
        return OnNotify(wParam, lParam);
    case WM_CLOSE:
        OnClose();
        break;
    case WM_DESTROY:
        OnDestroy();
        break;
    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// ── 辅助方法实现 ──

void ConfigWindow::CreateTabControl()
{
    // 创建选项卡控件
    m_tabCtrl = CreateWindowEx(
        0,
        WC_TABCONTROL,
        L"",
        WS_CHILD | WS_VISIBLE | TCS_BUTTONS,
        0, 0, 600, 400,
        m_hwnd, (HMENU)ID_TAB_CONTROL, m_hInstance, nullptr);

    // 添加选项卡
    TCITEM tcItem = {};
    tcItem.mask = TCIF_TEXT;

    tcItem.pszText = const_cast<LPWSTR>(GetStringResource(IDS_GENERAL));
    TabCtrl_InsertItem(m_tabCtrl, 0, &tcItem);

    tcItem.pszText = const_cast<LPWSTR>(GetStringResource(IDS_HOTKEYS));
    TabCtrl_InsertItem(m_tabCtrl, 1, &tcItem);

    tcItem.pszText = const_cast<LPWSTR>(GetStringResource(IDS_KNOWLEDGE));
    TabCtrl_InsertItem(m_tabCtrl, 2, &tcItem);

    tcItem.pszText = const_cast<LPWSTR>(GetStringResource(IDS_AI));
    TabCtrl_InsertItem(m_tabCtrl, 3, &tcItem);
}

void ConfigWindow::CreateGeneralPage(HWND parent)
{
    m_pageGeneral = CreateWindowEx(
        0, L"SysLink", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 560, 350,
        parent, nullptr, m_hInstance, nullptr);

    // 输入法模式
    HWND cbInputMode = CreateComboBox(parent, 150, 20, 150);
    ComboBox_AddString(cbInputMode, L"五笔模式");
    ComboBox_AddString(cbInputMode, L"拼音模式");
    ComboBox_SetCurSel(cbInputMode, 0);

    // 默认语言
    HWND cbDefaultLang = CreateComboBox(parent, 150, 60, 150);
    ComboBox_AddString(cbDefaultLang, L"简体中文");
    ComboBox_AddString(cbDefaultLang, L"繁体中文");
    ComboBox_SetCurSel(cbDefaultLang, 0);

    // 自动启动
    HWND cbAutoStart = CreateCheckbox(parent, L"开机自启动", 20, 100);

    // 显示通知
    HWND cbShowNotify = CreateCheckbox(parent, L"显示系统通知", 20, 130);
}

void ConfigWindow::CreateHotkeyPage(HWND parent)
{
    m_pageHotkeys = CreateWindowEx(
        0, L"SysLink", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 560, 350,
        parent, nullptr, m_hInstance, nullptr);

    // 启用热键
    HWND cbEnableHotkeys = CreateCheckbox(parent, L"启用快捷键", 20, 20);

    // F1-F4 热键编辑
    CreateLabel(parent, L"F1 热键:", 20, 60, 80);
    HWND edtF1 = CreateEdit(parent, 120, 55, 100);
    SetWindowText(edtF1, L"Ctrl+Shift+F1");

    CreateLabel(parent, L"F2 热键:", 20, 90, 80);
    HWND edtF2 = CreateEdit(parent, 120, 85, 100);
    SetWindowText(edtF2, L"Ctrl+Shift+F2");

    CreateLabel(parent, L"F3 热键:", 20, 120, 80);
    HWND edtF3 = CreateEdit(parent, 120, 115, 100);
    SetWindowText(edtF3, L"Ctrl+Shift+F3");

    CreateLabel(parent, L"F4 热键:", 20, 150, 80);
    HWND edtF4 = CreateEdit(parent, 120, 145, 100);
    SetWindowText(edtF4, L"Ctrl+Shift+F4");
}

void ConfigWindow::CreateKnowledgePage(HWND parent)
{
    m_pageKnowledge = CreateWindowEx(
        0, L"SysLink", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 560, 350,
        parent, nullptr, m_hInstance, nullptr);

    // 知识库路径
    CreateLabel(parent, L"知识库路径:", 20, 20, 100);
    HWND edtKBPath = CreateEdit(parent, 130, 15, 300, 25);
    SetWindowText(edtKBPath, L"C:\\QuickInput\\Knowledge");

    HWND btnBrowse = CreateButton(parent, L"浏览...", 440, 15, 80, 25);

    // 分类列表
    CreateLabel(parent, L"分类管理:", 20, 60, 100);
    HWND listCategories = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"LISTBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | LBS_STANDARD,
        130, 60, 200, 150,
        parent, nullptr, m_hInstance, nullptr);

    // 添加示例分类
    ListBox_AddString(listCategories, L"专业词汇");
    ListBox_AddString(listCategories, L"人名地名");
    ListBox_AddString(listCategories, L"自定义词组");

    // 操作按钮
    HWND btnAddCategory = CreateButton(parent, L"添加", 350, 60, 60, 25);
    HWND btnDelCategory = CreateButton(parent, L"删除", 350, 90, 60, 25);
}

void ConfigWindow::CreateAIPage(HWND parent)
{
    m_pageAI = CreateWindowEx(
        0, L"SysLink", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 560, 350,
        parent, nullptr, m_hInstance, nullptr);

    // AI模型路径
    CreateLabel(parent, L"模型路径:", 20, 20, 100);
    HWND edtModelPath = CreateEdit(parent, 130, 15, 300, 25);
    SetWindowText(edtModelPath, L"./models/gpt-3.5-turbo");

    // 建议条数
    CreateLabel(parent, L"建议条数:", 20, 60, 100);
    HWND spnSuggestCount = CreateWindow(
        UPDOWN_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
        130, 55, 0, 0,
        parent, (HMENU)1402, m_hInstance, nullptr);
    SendMessage(spnSuggestCount, UDM_SETRANGE, 0, MAKELONG(10, 1));
    SendMessage(spnSuggestCount, UDM_SETPOS, 0, 3);

    // 温度参数
    CreateLabel(parent, L"温度参数:", 20, 100, 100);
    HWND sliderTemp = CreateWindow(
        TRACKBAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, 95, 200, 30,
        parent, (HMENU)1403, m_hInstance, nullptr);
    SendMessage(sliderTemp, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
    SendMessage(sliderTemp, TBM_SETPOS, TRUE, 50);

    // API密钥
    CreateLabel(parent, L"API密钥:", 20, 150, 100);
    HWND edtApiKey = CreateEdit(parent, 130, 145, 300, 25);

    // 启用AI
    HWND cbEnableAI = CreateCheckbox(parent, L"启用AI功能", 20, 190);
}

HWND ConfigWindow::CreateLabel(HWND parent, const std::wstring& text, int x, int y, int width)
{
    return CreateWindowEx(
        0, L"STATIC", text.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, width, 20,
        parent, nullptr, m_hInstance, nullptr);
}

HWND ConfigWindow::CreateCheckbox(HWND parent, const std::wstring& text, int x, int y)
{
    return CreateWindowEx(
        0, L"BUTTON", text.c_str(),
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
        x, y, 150, 20,
        parent, nullptr, m_hInstance, nullptr);
}

HWND ConfigWindow::CreateEdit(HWND parent, int x, int y, int width, int height)
{
    return CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, width, height,
        parent, nullptr, m_hInstance, nullptr);
}

HWND ConfigWindow::CreateComboBox(HWND parent, int x, int y, int width, int height)
{
    HWND combo = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        x, y, width, height + 20,
        parent, nullptr, m_hInstance, nullptr);
    return combo;
}

HWND ConfigWindow::CreateButton(HWND parent, const std::wstring& text, int x, int y, int width, int height)
{
    return CreateWindowEx(
        0, L"BUTTON", text.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, width, height,
        parent, nullptr, m_hInstance, nullptr);
}

// ── 事件处理函数 ──

LRESULT CALLBACK ConfigWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ConfigWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<ConfigWindow*>(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<ConfigWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ConfigWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (LOWORD(wParam))
    {
    case ID_BTN_OK:
        SaveConfiguration();
        Destroy();
        break;

    case ID_BTN_CANCEL:
        Destroy();
        break;

    case ID_BTN_APPLY:
        SaveConfiguration();
        break;
    }

    return 0;
}

LRESULT ConfigWindow::OnNotify(WPARAM wParam, LPARAM lParam)
{
    auto* pnmh = reinterpret_cast<NMHDR*>(lParam);

    if (pnmh->idFrom == ID_TAB_CONTROL && pnmh->code == TCN_SELCHANGE)
    {
        // 隐藏所有页面
        ShowWindow(m_pageGeneral, SW_HIDE);
        ShowWindow(m_pageHotkeys, SW_HIDE);
        ShowWindow(m_pageKnowledge, SW_HIDE);
        ShowWindow(m_pageAI, SW_HIDE);

        // 显示选中的页面
        int sel = TabCtrl_GetCurSel(m_tabCtrl);
        switch (sel)
        {
        case 0: ShowWindow(m_pageGeneral, SW_SHOW); break;
        case 1: ShowWindow(m_pageHotkeys, SW_SHOW); break;
        case 2: ShowWindow(m_pageKnowledge, SW_SHOW); break;
        case 3: ShowWindow(m_pageAI, SW_SHOW); break;
        }
    }

    return 0;
}

void ConfigWindow::OnClose()
{
    SaveConfiguration();
    Destroy();
}

void ConfigWindow::OnDestroy()
{
    PostQuitMessage(0);
}

// ── 配置操作方法 ──

bool ConfigWindow::LoadConfiguration()
{
    // 从配置文件加载设置
    std::wifstream file(L"config.json");
    if (!file.is_open())
    {
        // 使用默认配置
        return true;
    }

    // TODO: 解析JSON配置
    file.close();
    return true;
}

bool ConfigWindow::SaveConfiguration()
{
    // 更新配置数据
    UpdateConfigFromUI();

    // 保存到配置文件
    std::wofstream file(L"config.json");
    if (!file.is_open())
    {
        return false;
    }

    // TODO: 序列化JSON配置
    file.close();
    return true;
}

void ConfigWindow::UpdateUIFromConfig()
{
    // TODO: 从m_config更新UI控件状态
}

void ConfigWindow::UpdateConfigFromUI()
{
    // TODO: 从UI控件更新m_config
}

void ConfigWindow::OnLoadConfig()
{
    LoadConfiguration();
    UpdateUIFromConfig();
}

void ConfigWindow::OnSaveConfig()
{
    SaveConfiguration();
}

// ── 字符串资源获取 ──

const wchar_t* ConfigWindow::GetStringResource(int id)
{
    auto it = StringTable.find(id);
    if (it != StringTable.end())
    {
        return it->second;
    }
    return L"";
}

} // namespace qi::ui