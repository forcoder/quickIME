#pragma once

#include "quickinput/core/common.h"

#include <windows.h>
#include <msctf.h>

namespace qi {
namespace ime {

// ═══════════════════════════════════════════════════════════════
// COM Server Registration
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 注册 COM 服务器到注册表
 * @param hInstance DLL 实例句柄
 * @return HRESULT
 *
 * 写入以下注册表项：
 * HKCR\CLSID\{QI_GUID_TEXT_SERVICE} = QuickInput Text Service
 * HKCR\CLSID\{QI_GUID_TEXT_SERVICE}\InprocServer32 = DLL 路径
 * HKCR\CLSID\{QI_GUID_TEXT_SERVICE}\InprocServer32\ThreadingModel = Apartment
 */
HRESULT RegisterServer(HINSTANCE hInstance);

/**
 * @brief 从注册表注销 COM 服务器
 * @return HRESULT
 */
HRESULT UnregisterServer();

// ═══════════════════════════════════════════════════════════════
// TSF Profile Registration
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 注册输入法配置文件到 TSF
 * @param hInstance DLL 实例句柄
 * @return HRESULT
 *
 * 使用 ITfInputProcessorProfileMgr 注册：
 * - 语言 ID: 0x0804 (中文-简体)
 * - 配置文件 GUID: QI_GUID_PROFILE
 * - 显示名称: QuickInput
 */
HRESULT RegisterProfiles(HINSTANCE hInstance);

/**
 * @brief 从 TSF 注销输入法配置文件
 * @return HRESULT
 */
HRESULT UnregisterProfiles();

// ═══════════════════════════════════════════════════════════════
// Category Registration
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 注册输入法类别
 * @return HRESULT
 *
 * 向 TSF 注册以下类别：
 * - GUID_TFCAT_TIP_KEYBOARD: 键盘输入法
 * - GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER: 显示属性提供者
 * - GUID_TFCAT_TIPCAP_UIELEMENTENABLED: UI 元素支持
 * - GUID_TFCAT_TIPCAP_COMLESS: 无 COM 依赖支持
 * - GUID_TFCAT_TIPCAP_INPUTMODECOMPOSITION: 输入模式组合支持
 */
HRESULT RegisterCategories();

/**
 * @brief 注销输入法类别
 * @return HRESULT
 */
HRESULT UnregisterCategories();

// ═══════════════════════════════════════════════════════════════
// Language Profile
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 添加语言配置文件
 * @param pThreadMgr 线程管理器
 * @param clientId 客户端 ID
 * @param langId 语言 ID (如 0x0804)
 * @param profileGuid 配置文件 GUID
 * @param pchDesc 显示名称
 * @param cchDesc 名称长度
 * @return HRESULT
 */
HRESULT AddLanguageProfile(
    ITfThreadMgr* pThreadMgr,
    TfClientId clientId,
    LANGID langId,
    REFGUID profileGuid,
    const WCHAR* pchDesc,
    ULONG cchDesc
);

// ═══════════════════════════════════════════════════════════════
// Utility functions
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 获取 DLL 文件路径
 * @param hInstance DLL 实例句柄
 * @param pszPath 输出路径缓冲区
 * @param cchPath 缓冲区大小（字符数）
 * @return 实际写入的字符数
 */
UINT GetDllPath(HINSTANCE hInstance, WCHAR* pszPath, UINT cchPath);

/**
 * @brief 将 GUID 格式化为注册表字符串
 * @param guid GUID
 * @param pszBuf 输出缓冲区（至少 64 字符）
 * @return 写入的字符数
 */
int FormatGuidString(const GUID& guid, WCHAR* pszBuf);

} // namespace ime
} // namespace qi
