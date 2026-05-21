#include "quickinput/ime/register.h"

#include <windows.h>
#include <msctf.h>
#include <ctffunc.h>
#include <tchar.h>
#include <objbase.h>

namespace qi {
namespace ime {

// ═══════════════════════════════════════════════════════════════
// Registry key path constants
// ═══════════════════════════════════════════════════════════════

namespace {

// COM 注册表路径前缀
const WCHAR kClsidPath[]      = L"CLSID";
const WCHAR kInprocServer[]   = L"InprocServer32";
const WCHAR kThreadingModel[] = L"ThreadingModel";
const WCHAR kApartment[]      = L"Apartment";

// TSF 配置文件注册表路径
// HKLM\SOFTWARE\Microsoft\CTF\TIP\{CLSID}\LanguageProfile\0x00000804\{ProfileGUID}
const WCHAR kTipPath[] = L"SOFTWARE\\Microsoft\\CTF\\TIP";
const WCHAR kLangProfile[] = L"LanguageProfile";
const WCHAR kLangChineseSimplified[] = L"0x00000804";

// 输入法显示名称
const WCHAR kDisplayName[] = L"QuickInput 五笖输入法";  // QuickInput 五笔输入法
const ULONG kDisplayNameLen = static_cast<ULONG>(wcslen(kDisplayName));

// 描述字符串
const WCHAR kDescription[] = L"QuickInput Wubi Input Method";

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════
// Utility functions
// ═══════════════════════════════════════════════════════════════

UINT GetDllPath(HINSTANCE hInstance, WCHAR* pszPath, UINT cchPath)
{
    if (!pszPath || cchPath == 0) return 0;
    return GetModuleFileNameW(hInstance, pszPath, cchPath);
}

int FormatGuidString(const GUID& guid, WCHAR* pszBuf)
{
    if (!pszBuf) return 0;
    return StringFromGUID2(guid, pszBuf, 64);
}

// ═══════════════════════════════════════════════════════════════
// COM Server Registration
// ═══════════════════════════════════════════════════════════════

HRESULT RegisterServer(HINSTANCE hInstance)
{
    HRESULT hr = S_OK;
    WCHAR szDllPath[MAX_PATH] = {};
    WCHAR szClsidString[64] = {};
    WCHAR szSubKey[MAX_PATH] = {};

    // 获取 DLL 路径
    UINT pathLen = GetDllPath(hInstance, szDllPath, MAX_PATH);
    if (pathLen == 0) return HRESULT_FROM_WIN32(GetLastError());

    // 格式化 CLSID 字符串
    FormatGuidString(QI_GUID_TEXT_SERVICE, szClsidString);

    // ── 1. HKCR\CLSID\{QI_GUID_TEXT_SERVICE} ──
    hr = StringCchPrintfW(szSubKey, MAX_PATH, L"%s\\%s", kClsidPath, szClsidString);
    if (FAILED(hr)) return hr;

    HKEY hKey = nullptr;
    LONG lResult = RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubKey, 0, nullptr,
                                    REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                                    &hKey, nullptr);
    if (lResult != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lResult);

    // 设置默认值为描述字符串
    RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(kDescription),
                   static_cast<DWORD>((wcslen(kDescription) + 1) * sizeof(WCHAR)));
    RegCloseKey(hKey);

    // ── 2. HKCR\CLSID\{QI_GUID_TEXT_SERVICE}\InprocServer32 ──
    hr = StringCchPrintfW(szSubKey, MAX_PATH, L"%s\\%s\\%s",
                          kClsidPath, szClsidString, kInprocServer);
    if (FAILED(hr)) return hr;

    lResult = RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubKey, 0, nullptr,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                               &hKey, nullptr);
    if (lResult != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lResult);

    // 设置 DLL 路径
    RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(szDllPath),
                   static_cast<DWORD>((pathLen + 1) * sizeof(WCHAR)));

    // 设置线程模型为 Apartment
    RegSetValueExW(hKey, kThreadingModel, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(kApartment),
                   static_cast<DWORD>((wcslen(kApartment) + 1) * sizeof(WCHAR)));
    RegCloseKey(hKey);

    return S_OK;
}

HRESULT UnregisterServer()
{
    WCHAR szClsidString[64] = {};
    FormatGuidString(QI_GUID_TEXT_SERVICE, szClsidString);

    // 删除 HKCR\CLSID\{QI_GUID_TEXT_SERVICE} 及其所有子项
    WCHAR szSubKey[MAX_PATH] = {};
    StringCchPrintfW(szSubKey, MAX_PATH, L"%s\\%s", kClsidPath, szClsidString);

    // 递归删除注册表项
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szSubKey);

    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// TSF Profile Registration
// ═══════════════════════════════════════════════════════════════

HRESULT RegisterProfiles(HINSTANCE hInstance)
{
    HRESULT hr = S_OK;

    // 获取 ITfInputProcessorProfileMgr 接口
    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfileMgr,
                          reinterpret_cast<void**>(&pProfileMgr));
    if (FAILED(hr)) return hr;

    // 获取 DLL 路径
    WCHAR szDllPath[MAX_PATH] = {};
    UINT pathLen = GetDllPath(hInstance, szDllPath, MAX_PATH);

    // 注册配置文件
    // 参数：CLSID, langid, guidProfile, 描述, 描述长度, 图标文件, 图标索引, 快捷键...
    hr = pProfileMgr->Register(
        QI_GUID_TEXT_SERVICE,       // CLSID
        MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),  // 0x0804
        QI_GUID_PROFILE,            // Profile GUID
        kDisplayName,               // 显示名称
        kDisplayNameLen,            // 名称长度
        szDllPath,                  // 图标文件路径（使用 DLL 自带图标）
        pathLen,                    // 路径长度
        0,                          // 图标索引
        nullptr,                    // 快捷键（默认）
        0,                          // 快捷键数量
        0,                          // 标志
        0,                          // 是否为默认输入法
        0                           // 保留
    );

    pProfileMgr->Release();
    return hr;
}

HRESULT UnregisterProfiles()
{
    HRESULT hr = S_OK;

    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfileMgr,
                          reinterpret_cast<void**>(&pProfileMgr));
    if (FAILED(hr)) return hr;

    // 注销配置文件
    hr = pProfileMgr->Unregister(
        QI_GUID_TEXT_SERVICE,
        MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
        QI_GUID_PROFILE,
        0
    );

    pProfileMgr->Release();
    return hr;
}

// ═══════════════════════════════════════════════════════════════
// Category Registration
// ═══════════════════════════════════════════════════════════════

HRESULT RegisterCategories()
{
    HRESULT hr = S_OK;

    // 获取 ITfCategoryMgr 接口
    ITfCategoryMgr* pCategoryMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr,
                          reinterpret_cast<void**>(&pCategoryMgr));
    if (FAILED(hr)) return hr;

    // 注册键盘输入法类别
    hr = pCategoryMgr->RegisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_TIP_KEYBOARD,
        QI_GUID_TEXT_SERVICE
    );
    if (FAILED(hr))
    {
        pCategoryMgr->Release();
        return hr;
    }

    // 注册显示属性提供者类别
    hr = pCategoryMgr->RegisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
        QI_GUID_TEXT_SERVICE
    );
    if (FAILED(hr))
    {
        pCategoryMgr->Release();
        return hr;
    }

    // 注册 UI 元素支持类别
    hr = pCategoryMgr->RegisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
        QI_GUID_TEXT_SERVICE
    );
    if (FAILED(hr))
    {
        pCategoryMgr->Release();
        return hr;
    }

    // 注册输入模式组合支持类别
    hr = pCategoryMgr->RegisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_TIPCAP_INPUTMODECOMPOSITION,
        QI_GUID_TEXT_SERVICE
    );

    pCategoryMgr->Release();
    return hr;
}

HRESULT UnregisterCategories()
{
    HRESULT hr = S_OK;

    ITfCategoryMgr* pCategoryMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr,
                          reinterpret_cast<void**>(&pCategoryMgr));
    if (FAILED(hr)) return hr;

    // 注销所有已注册的类别
    pCategoryMgr->UnregisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_TIP_KEYBOARD,
        QI_GUID_TEXT_SERVICE
    );

    pCategoryMgr->UnregisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
        QI_GUID_TEXT_SERVICE
    );

    pCategoryMgr->UnregisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
        QI_GUID_TEXT_SERVICE
    );

    pCategoryMgr->UnregisterCategory(
        QI_GUID_TEXT_SERVICE,
        GUID_TFCAT_TIPCAP_INPUTMODECOMPOSITION,
        QI_GUID_TEXT_SERVICE
    );

    pCategoryMgr->Release();
    return S_OK;
}

// ═══════════════════════════════════════════════════════════════
// Language Profile
// ═══════════════════════════════════════════════════════════════

HRESULT AddLanguageProfile(
    ITfThreadMgr* pThreadMgr,
    TfClientId clientId,
    LANGID langId,
    REFGUID profileGuid,
    const WCHAR* pchDesc,
    ULONG cchDesc)
{
    if (!pThreadMgr || !pchDesc) return E_INVALIDARG;

    // 获取 ITfInputProcessorProfiles 接口
    ITfInputProcessorProfiles* pProfiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfiles,
                                   reinterpret_cast<void**>(&pProfiles));
    if (FAILED(hr)) return hr;

    // 添加语言配置文件
    hr = pProfiles->AddLanguageProfile(
        QI_GUID_TEXT_SERVICE,
        langId,
        profileGuid,
        pchDesc,
        cchDesc,
        nullptr,  // 图标文件
        0,        // 图标路径长度
        0         // 图标索引
    );

    pProfiles->Release();
    return hr;
}

} // namespace ime
} // namespace qi
