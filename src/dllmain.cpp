#include "quickinput/core/common.h"
#include "quickinput/ime/text_service.h"
#include "quickinput/ime/register.h"

#include <windows.h>
#include <ole2.h>
#include <olectl.h>
#include <msctf.h>
#include <tchar.h>
#include <objbase.h>

#include <atomic>

// ═══════════════════════════════════════════════════════════════
// Module-level globals
// ═══════════════════════════════════════════════════════════════

namespace {

// DLL module handle
HINSTANCE g_hInstance = nullptr;

// COM 对象引用计数（用于 DllCanUnloadNow）
std::atomic<LONG> g_cServerLocks{0};

// 已注册的 TextService 类工厂计数
std::atomic<LONG> g_cClassObjects{0};

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════
// DLL Entry Point
// ═══════════════════════════════════════════════════════════════

/**
 * @brief DLL 入口函数
 * @param hInstance DLL 实例句柄
 * @param dwReason 加载/卸载原因
 * @param lpReserved 保留参数
 */
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // 保存模块句柄
        g_hInstance = hInstance;
        // 禁用线程通知回调以提高性能
        DisableThreadLibraryCalls(hInstance);
        break;

    case DLL_PROCESS_DETACH:
        g_hInstance = nullptr;
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

// ═══════════════════════════════════════════════════════════════
// Class Factory
// ═══════════════════════════════════════════════════════════════

/**
 * @class QuickInputClassFactory
 * @brief QuickInput 的 COM 类工厂
 *
 * 负责创建 TextService 实例。
 * TSF 管理器通过 DllGetClassObject 获取此工厂，然后调用 CreateInstance
 * 创建输入法实例。
 */
class QuickInputClassFactory : public IClassFactory
{
public:
    QuickInputClassFactory();
    ~QuickInputClassFactory();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG   STDMETHODCALLTYPE AddRef() override;
    ULONG   STDMETHODCALLTYPE Release() override;

    // IClassFactory
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override;
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;

private:
    LONG m_refCount;
};

QuickInputClassFactory::QuickInputClassFactory()
    : m_refCount(1)
{
}

QuickInputClassFactory::~QuickInputClassFactory()
{
}

HRESULT STDMETHODCALLTYPE QuickInputClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject) return E_INVALIDARG;
    *ppvObject = nullptr;

    if (IsEqualIID(riid, __uuidof(IUnknown)) ||
        IsEqualIID(riid, __uuidof(IClassFactory)))
    {
        *ppvObject = static_cast<IClassFactory*>(this);
    }

    if (*ppvObject)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE QuickInputClassFactory::AddRef()
{
    return ::InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE QuickInputClassFactory::Release()
{
    LONG ref = ::InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        delete this;
    }
    return static_cast<ULONG>(ref);
}

HRESULT STDMETHODCALLTYPE QuickInputClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    // 不支持聚合
    if (pUnkOuter != nullptr) return CLASS_E_NOAGGREGATION;
    if (!ppvObject) return E_INVALIDARG;

    *ppvObject = nullptr;

    // 创建 TextService 实例
    qi::ime::TextService* pTextService = new (std::nothrow) qi::ime::TextService();
    if (!pTextService) return E_OUTOFMEMORY;

    // 增加服务器锁计数
    ::InterlockedIncrement(&g_cServerLocks);
    ::InterlockedIncrement(&g_cClassObjects);

    HRESULT hr = pTextService->QueryInterface(riid, ppvObject);
    pTextService->Release();  // QI 已经 AddRef

    if (FAILED(hr))
    {
        ::InterlockedDecrement(&g_cServerLocks);
        ::InterlockedDecrement(&g_cClassObjects);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE QuickInputClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        ::InterlockedIncrement(&g_cServerLocks);
    }
    else
    {
        ::InterlockedDecrement(&g_cServerLocks);
    }
    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// Exported COM functions
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 获取 QuickInput 的类工厂
 * @param rclsid 必须为 QI_GUID_TEXT_SERVICE
 * @param riid 必须为 IID_IClassFactory
 * @param ppv 输出类工厂指针
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;

    // 只处理 QuickInput 的 CLSID
    if (!IsEqualCLSID(rclsid, QI_GUID_TEXT_SERVICE))
        return CLASS_E_CLASSNOTAVAILABLE;

    // 创建类工厂
    QuickInputClassFactory* pFactory = new (std::nothrow) QuickInputClassFactory();
    if (!pFactory) return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();

    return hr;
}

/**
 * @brief 检查 DLL 是否可以卸载
 * @return S_OK 可以卸载，S_FALSE 不可卸载
 */
STDAPI DllCanUnloadNow()
{
    if (g_cServerLocks > 0 || g_cClassObjects > 0)
        return S_FALSE;
    return S_OK;
}

/**
 * @brief 注册输入法到系统
 * 写入注册表 + 注册 TSF 配置文件
 */
STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;

    // 1. 注册 COM 服务器（CLSID 写入注册表）
    hr = qi::ime::RegisterServer(g_hInstance);
    if (FAILED(hr)) return hr;

    // 2. 注册 TSF 配置文件
    hr = qi::ime::RegisterProfiles(g_hInstance);
    if (FAILED(hr)) return hr;

    // 3. 注册类别
    hr = qi::ime::RegisterCategories();

    return hr;
}

/**
 * @brief 从系统卸载输入法
 * 清理注册表 + 注销 TSF 配置文件
 */
STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;

    // 1. 注销类别
    qi::ime::UnregisterCategories();

    // 2. 注销 TSF 配置文件
    qi::ime::UnregisterProfiles();

    // 3. 注销 COM 服务器
    hr = qi::ime::UnregisterServer();

    return hr;
}
