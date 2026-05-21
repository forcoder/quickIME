#ifndef QUICKINPUT_CORE_UTILS_H_
#define QUICKINPUT_CORE_UTILS_H_

#include <string>
#include <windows.h>

namespace qi {
namespace utils {

// 文件操作工具函数
bool FileExists(const std::wstring& path);
bool CreateDirectory(const std::wstring& path);
std::wstring GetParentDirectory(const std::wstring& path);
std::wstring GetConfigDirectory();
std::wstring GetAppDataPath();

// 字符串工具函数
std::wstring Narrow(const std::string& utf8);
std::string WideToUtf8(const std::wstring& wide);

} // namespace utils
} // namespace qi

#endif // QUICKINPUT_CORE_UTILS_H_