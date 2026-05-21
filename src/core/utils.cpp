#include "quickinput/core/utils.h"
#include "quickinput/core/common.h"
#include <shlobj.h>
#include <codecvt>
#include <locale>

namespace qi {
namespace utils {

// 检查文件是否存在
bool FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

// 创建目录（如果不存在）
bool CreateDirectory(const std::wstring& path) {
    if (FileExists(path)) {
        return true;
    }

    // 确保父目录存在
    std::wstring parentDir = GetParentDirectory(path);
    if (!parentDir.empty() && !CreateDirectory(parentDir)) {
        return false;
    }

    return ::CreateDirectoryW(path.c_str(), nullptr) == ERROR_SUCCESS;
}

// 获取父目录
std::wstring GetParentDirectory(const std::wstring& path) {
    size_t lastSlash = path.find_last_of(L"/\\");
    if (lastSlash == std::wstring::npos || lastSlash == 0) {
        return L"";
    }
    return path.substr(0, lastSlash);
}

// 获取配置目录路径
std::wstring GetConfigDirectory() {
    std::wstring appDataPath = GetAppDataPath();
    return appDataPath + L"\\QuickInput";
}

// 获取应用程序数据路径
std::wstring GetAppDataPath() {
    PWSTR pszPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pszPath);

    if (SUCCEEDED(hr)) {
        std::wstring path(pszPath);
        CoTaskMemFree(pszPath);
        return path;
    }

    // 回退到环境变量
    wchar_t* envPath = nullptr;
    _dupenv_s(&envPath, nullptr, L"APPDATA");
    if (envPath) {
        std::wstring path(envPath);
        free(envPath);
        return path;
    }

    // 最终回退到当前目录
    return L"./config";
}

// 字符串转换工具函数
std::wstring Narrow(const std::string& utf8) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(utf8);
    } catch (...) {
        return L"[转换错误]";
    }
}

std::string WideToUtf8(const std::wstring& wide) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wide);
    } catch (...) {
        return "[转换错误]";
    }
}

} // namespace utils
} // namespace qi