#pragma once

#include <windows.h>
#include <string>
#include <functional>

using ProgressCallback = std::function<void(int percent, const wchar_t* status)>;

bool DownloadFile(const wchar_t* url, const wchar_t* destPath, ProgressCallback onProgress);
bool RunInstaller(const wchar_t* filePath, const wchar_t* args);
std::wstring GetWinHubDownloadDir();
std::wstring UrlFileNameFromUrl(const wchar_t* url, const wchar_t* fallback);
