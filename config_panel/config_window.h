namespace qi::ui {

class ConfigWindow {
public:
    ConfigWindow(HINSTANCE hInstance);
    ~ConfigWindow();

    // ── 窗口创建与显示 ──
    bool Create(HWND hParent, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                int width = 600, int height = 480);
    void Show();
    void Hide();
    void Destroy();

    // ── 消息处理 ──
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // ── 配置同步 ──
    bool LoadConfiguration();      // 从配置文件加载
    bool SaveConfiguration();    // 保存到配置文件
    void UpdateUIFromConfig();   // UI控件更新

private:
    // 窗口属性
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;

    // 控件句柄
    HWND m_tabCtrl = nullptr;
    HWND m_btnOK = nullptr;
    HWND m_btnCancel = nullptr;
    HWND m_btnApply = nullptr;

    // 页面句柄
    HWND m_pageGeneral = nullptr;
    HWND m_pageHotkeys = nullptr;
    HWND m_pageKnowledge = nullptr;
    HWND m_pageAI = nullptr;

    // 配置数据
    HotkeyConfig m_config;

    // ── 辅助方法 ──
    // 创建标签页控件
    void CreateTabControl();

    // 创建各个页面
    void CreateGeneralPage(HWND parent);
    void CreateHotkeyPage(HWND parent);
    void CreateKnowledgePage(HWND parent);
    void CreateAIPage(HWND parent);

    // 创建通用控件
    HWND CreateLabel(HWND parent, const std::wstring& text, int x, int y, int width);
    HWND CreateCheckbox(HWND parent, const std::wstring& text, int x, int y);
    HWND CreateEdit(HWND parent, int x, int y, int width, int height = 20);
    HWND CreateComboBox(HWND parent, int x, int y, int width, int height = 20);
    HWND CreateButton(HWND parent, const std::wstring& text, int x, int y, int width, int height);

    // 事件处理回调
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 页面过程函数
    static LRESULT CALLBACK GeneralPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HotkeyPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KnowledgePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK AIPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 配置操作方法
    void OnLoadConfig();
    void OnSaveConfig();
    void UpdateConfigFromUI();

    // 对话框资源ID
    enum DialogIDs {
        ID_TAB_CONTROL = 1001,
        ID_BTN_OK = 1002,
        ID_BTN_CANCEL = 1003,
        ID_BTN_APPLY = 1004,
        ID_PAGE_GENERAL = 2001,
        ID_PAGE_HOTKEYS = 2002,
        ID_PAGE_KNOWLEDGE = 2003,
        ID_PAGE_AI = 2004
    };

    // 字符串资源
    static const wchar_t* GetStringResource(int id);

    // 初始化GDI+
    bool InitializeGDIplus();
    void ShutdownGDIplus();
};

} // namespace qi::ui