#include "quickinput/core/common.h"
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace qi {

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), size);
    return result;
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

bool StartsWith(const std::wstring& str, const std::wstring& prefix) {
    if (prefix.size() > str.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::wstring GetAppDataPath() {
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::wstring result(path);
        result += L"\\QuickInput";
        CreateDirectoryW(result.c_str(), nullptr);
        return result;
    }
    return L"C:\\QuickInput";
}

std::wstring GetInstallPath() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring fullPath(path);
    auto pos = fullPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return fullPath.substr(0, pos);
    }
    return L"C:\\QuickInput";
}

std::wstring GetKbPath() {
    std::wstring path = GetAppDataPath() + L"\\knowledge";
    CreateDirectoryW(path.c_str(), nullptr);
    return path;
}

std::wstring GetModelPath() {
    return GetInstallPath() + L"\\models";
}

std::wstring GuidToString(const GUID& guid) {
    wchar_t buf[64] = {};
    StringFromGUID2(guid, buf, 64);
    return std::wstring(buf);
}

} // namespace qi
