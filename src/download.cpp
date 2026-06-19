#include "download.h"

#include <winhttp.h>
#include <shlobj.h>
#include <urlmon.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

std::wstring GetWinHubDownloadDir() {
    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    std::wstring dir = std::wstring(temp) + L"WinHub\\downloads\\";
    CreateDirectoryW((std::wstring(temp) + L"WinHub").c_str(), nullptr);
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring UrlFileNameFromUrl(const wchar_t* url, const wchar_t* fallback) {
    if (fallback && fallback[0]) return fallback;
    const wchar_t* slash = wcsrchr(url, L'/');
    if (!slash || !slash[1]) return L"download.bin";
    std::wstring name = slash + 1;
    const size_t q = name.find(L'?');
    if (q != std::wstring::npos) name = name.substr(0, q);
    return name.empty() ? L"download.bin" : name;
}

static bool ParseUrl(const wchar_t* url, std::wstring& host, std::wstring& path, INTERNET_PORT& port, bool& secure) {
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t hostBuf[256]{};
    wchar_t pathBuf[2048]{};
    parts.lpszHostName = hostBuf;
    parts.dwHostNameLength = 256;
    parts.lpszUrlPath = pathBuf;
    parts.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(url, 0, 0, &parts)) return false;
    host = hostBuf;
    path = pathBuf;
    if (parts.lpszExtraInfo && parts.lpszExtraInfo[0]) path += parts.lpszExtraInfo;
    port = parts.nPort;
    secure = (parts.nScheme == INTERNET_SCHEME_HTTPS);
    return true;
}

bool DownloadFile(const wchar_t* url, const wchar_t* destPath, ProgressCallback onProgress) {
    std::wstring host, path;
    INTERNET_PORT port = 0;
    bool secure = false;
    if (!ParseUrl(url, host, path, port, secure)) return false;

    HINTERNET session = WinHttpOpen(L"WinHub/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;

    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    HINTERNET connect = WinHttpConnect(session, host.c_str(), port, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(), nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    if (statusCode >= 300 && statusCode < 400) {
        wchar_t location[2048]{};
        DWORD locSize = sizeof(location);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                                location, &locSize, WINHTTP_NO_HEADER_INDEX)) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return DownloadFile(location, destPath, onProgress);
        }
    }

    if (statusCode != 200) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD contentLength = 0;
    DWORD clSize = sizeof(contentLength);
    bool hasLength = WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                                         WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &clSize,
                                         WINHTTP_NO_HEADER_INDEX);

    HANDLE file = CreateFileW(destPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD totalRead = 0;
    std::vector<char> buffer(64 * 1024);
    bool ok = true;

    while (ok) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            ok = false;
            break;
        }
        if (available == 0) break;

        DWORD toRead = available > buffer.size() ? (DWORD)buffer.size() : available;
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), toRead, &read) || read == 0) {
            ok = false;
            break;
        }

        DWORD written = 0;
        if (!WriteFile(file, buffer.data(), read, &written, nullptr) || written != read) {
            ok = false;
            break;
        }

        totalRead += read;
        if (onProgress && hasLength && contentLength > 0) {
            int pct = (int)((totalRead * 100ull) / contentLength);
            onProgress(pct, L"Загрузка...");
        }
    }

    CloseHandle(file);
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    if (!ok) DeleteFileW(destPath);
    return ok;
}

bool RunInstaller(const wchar_t* filePath, const wchar_t* args) {
    HINSTANCE result = ShellExecuteW(nullptr, L"open", filePath,
                                     (args && args[0]) ? args : nullptr, nullptr, SW_SHOWNORMAL);
    return (INT_PTR)result > 32;
}
