#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>

#include "catalog.h"
#include "download.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dwmapi.lib")

// Controls
#define IDC_SEARCH       1001
#define IDC_CATEGORY     1002
#define IDC_LIST         1003
#define IDC_INSTALL      1004
#define IDC_INSTALL_ALL  1005
#define IDC_PROGRESS     1006
#define IDC_STATUS       1007
#define IDC_DESC         1008

static HWND g_hwndMain = nullptr;
static HWND g_hwndSearch = nullptr;
static HWND g_hwndCategory = nullptr;
static HWND g_hwndList = nullptr;
static HWND g_hwndInstall = nullptr;
static HWND g_hwndInstallAll = nullptr;
static HWND g_hwndProgress = nullptr;
static HWND g_hwndStatus = nullptr;
static HWND g_hwndDesc = nullptr;

static HFONT g_fontUi = nullptr;
static HFONT g_fontBold = nullptr;
static bool g_busy = false;
static std::vector<int> g_visibleIndices;

static void SetBusy(bool busy) {
    g_busy = busy;
    EnableWindow(g_hwndInstall, !busy);
    EnableWindow(g_hwndInstallAll, !busy);
    EnableWindow(g_hwndList, !busy);
    EnableWindow(g_hwndSearch, !busy);
    EnableWindow(g_hwndCategory, !busy);
}

static std::wstring ToLower(const std::wstring& s) {
    std::wstring r = s;
    for (auto& c : r) c = (wchar_t)towlower(c);
    return r;
}

static bool MatchesFilter(int index, const std::wstring& search, int categorySel) {
    const AppEntry& app = g_catalog[index];
    if (categorySel > 0) {
        if (wcscmp(app.category, g_categories[categorySel]) != 0) return false;
    }
    if (search.empty()) return true;
    std::wstring hay = ToLower(std::wstring(app.name) + L" " + app.description + L" " + app.category);
    return hay.find(search) != std::wstring::npos;
}

static void RefreshList() {
    wchar_t searchBuf[256]{};
    GetWindowTextW(g_hwndSearch, searchBuf, 256);
    std::wstring search = ToLower(searchBuf);
    int categorySel = (int)SendMessageW(g_hwndCategory, CB_GETCURSEL, 0, 0);

    ListView_DeleteAllItems(g_hwndList);
    g_visibleIndices.clear();

    for (int i = 0; i < g_catalogCount; ++i) {
        if (!MatchesFilter(i, search, categorySel)) continue;
        g_visibleIndices.push_back(i);

        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = (int)g_visibleIndices.size() - 1;
        item.pszText = (LPWSTR)g_catalog[i].name;
        item.lParam = i;
        ListView_InsertItem(g_hwndList, &item);
        ListView_SetItemText(g_hwndList, item.iItem, 1, (LPWSTR)g_catalog[i].category);
        ListView_SetItemText(g_hwndList, item.iItem, 2, (LPWSTR)g_catalog[i].description);
    }

    wchar_t count[128];
    swprintf_s(count, L"Найдено программ: %d", (int)g_visibleIndices.size());
    SetWindowTextW(g_hwndStatus, count);
}

static void UpdateDescription() {
    int sel = ListView_GetNextItem(g_hwndList, -1, LVNI_SELECTED);
    if (sel < 0) {
        SetWindowTextW(g_hwndDesc, L"Выберите программу из списка, чтобы увидеть описание.");
        return;
    }
    LVITEMW item{};
    item.mask = LVIF_PARAM;
    item.iItem = sel;
    ListView_GetItem(g_hwndList, &item);
    const AppEntry& app = g_catalog[item.lParam];
    std::wstring text = std::wstring(L"📦 ") + app.name + L"\r\n\r\n" + app.description +
                        L"\r\n\r\nКатегория: " + app.category;
    SetWindowTextW(g_hwndDesc, text.c_str());
}

static void SetProgress(int pct, const wchar_t* status) {
    SendMessageW(g_hwndProgress, PBM_SETPOS, pct, 0);
    if (status) SetWindowTextW(g_hwndStatus, status);
}

static void InstallApps(const std::vector<int>& indices) {
    if (indices.empty() || g_busy) return;
    SetBusy(true);
    SendMessageW(g_hwndProgress, PBM_SETPOS, 0, 0);

    std::thread([indices]() {
        std::wstring dir = GetWinHubDownloadDir();
        int done = 0;
        int total = (int)indices.size();

        for (int idx : indices) {
            const AppEntry& app = g_catalog[idx];
            done++;
            wchar_t status[512];
            swprintf_s(status, L"[%d/%d] Загрузка: %s", done, total, app.name);
            PostMessageW(g_hwndMain, WM_APP + 1, 0, (LPARAM)_wcsdup(status));

            std::wstring fileName = UrlFileNameFromUrl(app.downloadUrl, app.fileName);
            std::wstring dest = dir + fileName;

            bool ok = DownloadFile(app.downloadUrl, dest.c_str(),
                [](int pct, const wchar_t* st) {
                    PostMessageW(g_hwndMain, WM_APP + 2, pct, 0);
                });

            if (!ok) {
                swprintf_s(status, L"Ошибка загрузки: %s", app.name);
                PostMessageW(g_hwndMain, WM_APP + 1, 0, (LPARAM)_wcsdup(status));
                continue;
            }

            swprintf_s(status, L"[%d/%d] Установка: %s", done, total, app.name);
            PostMessageW(g_hwndMain, WM_APP + 1, 0, (LPARAM)_wcsdup(status));
            RunInstaller(dest.c_str(), app.installArgs);
            Sleep(1500);
        }

        PostMessageW(g_hwndMain, WM_APP + 3, 0, 0);
    }).detach();
}

static void LayoutControls(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = rc.right;
    int h = rc.bottom;
    int pad = 12;
    int topH = 36;
    int bottomH = 120;
    int btnW = 160;
    int btnH = 34;

    MoveWindow(g_hwndSearch, pad, pad, w - pad * 2 - 200, topH, TRUE);
    MoveWindow(g_hwndCategory, w - pad - 190, pad, 190, 400, TRUE);

    int listTop = pad + topH + pad;
    int listH = h - listTop - bottomH - pad * 2;
    MoveWindow(g_hwndList, pad, listTop, w - pad * 2, listH, TRUE);

    int bottomTop = listTop + listH + pad;
    MoveWindow(g_hwndDesc, pad, bottomTop, w - pad * 2 - btnW - pad, 70, TRUE);
    MoveWindow(g_hwndInstall, w - pad - btnW, bottomTop, btnW, btnH, TRUE);
    MoveWindow(g_hwndInstallAll, w - pad - btnW, bottomTop + btnH + 6, btnW, btnH, TRUE);
    MoveWindow(g_hwndProgress, pad, bottomTop + 78, w - pad * 2, 22, TRUE);
    MoveWindow(g_hwndStatus, pad, h - pad - 20, w - pad * 2, 20, TRUE);
}

static void ApplyDarkTheme(HWND hwnd) {
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    HBRUSH bg = CreateSolidBrush(RGB(24, 24, 28));
    SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bg);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwndMain = hwnd;
        INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&icc);

        g_fontUi = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_fontBold = CreateFontW(-18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        g_hwndSearch = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 100, 30, hwnd, (HMENU)IDC_SEARCH, nullptr, nullptr);
        SendMessageW(g_hwndSearch, EM_SETCUEBANNER, TRUE, (LPARAM)L"🔍  Поиск программ...");

        g_hwndCategory = CreateWindowExW(0, L"COMBOBOX", nullptr,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 100, 300,
            hwnd, (HMENU)IDC_CATEGORY, nullptr, nullptr);
        for (int i = 0; i < g_categoryCount; ++i)
            SendMessageW(g_hwndCategory, CB_ADDSTRING, 0, (LPARAM)g_categories[i]);
        SendMessageW(g_hwndCategory, CB_SETCURSEL, 0, 0);

        g_hwndList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
            0, 0, 100, 100, hwnd, (HMENU)IDC_LIST, nullptr, nullptr);
        ListView_SetExtendedListViewStyle(g_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = (LPWSTR)L"Название"; col.cx = 200; ListView_InsertColumn(g_hwndList, 0, &col);
        col.pszText = (LPWSTR)L"Категория"; col.cx = 120; ListView_InsertColumn(g_hwndList, 1, &col);
        col.pszText = (LPWSTR)L"Описание"; col.cx = 400; ListView_InsertColumn(g_hwndList, 2, &col);

        g_hwndDesc = CreateWindowExW(0, L"STATIC",
            L"Выберите программу из списка, чтобы увидеть описание.",
            WS_CHILD | WS_VISIBLE, 0, 0, 100, 60, hwnd, (HMENU)IDC_DESC, nullptr, nullptr);

        g_hwndInstall = CreateWindowExW(0, L"BUTTON", L"⬇ Установить",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, hwnd, (HMENU)IDC_INSTALL, nullptr, nullptr);
        g_hwndInstallAll = CreateWindowExW(0, L"BUTTON", L"⬇ Установить всё",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 30, hwnd, (HMENU)IDC_INSTALL_ALL, nullptr, nullptr);

        g_hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
            WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, hwnd, (HMENU)IDC_PROGRESS, nullptr, nullptr);
        SendMessageW(g_hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

        g_hwndStatus = CreateWindowExW(0, L"STATIC", L"Готово",
            WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, hwnd, (HMENU)IDC_STATUS, nullptr, nullptr);

        HWND ctrls[] = { g_hwndSearch, g_hwndCategory, g_hwndList, g_hwndDesc,
                         g_hwndInstall, g_hwndInstallAll, g_hwndStatus };
        for (HWND c : ctrls) SendMessageW(c, WM_SETFONT, (WPARAM)g_fontUi, TRUE);

        ApplyDarkTheme(hwnd);
        RefreshList();
        return 0;
    }
    case WM_SIZE:
        LayoutControls(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SEARCH:
            if (HIWORD(wParam) == EN_CHANGE) RefreshList();
            break;
        case IDC_CATEGORY:
            if (HIWORD(wParam) == CBN_SELCHANGE) RefreshList();
            break;
        case IDC_INSTALL: {
            int sel = ListView_GetNextItem(g_hwndList, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBoxW(hwnd, L"Выберите программу для установки.", L"WinHub", MB_OK | MB_ICONINFORMATION);
                break;
            }
            LVITEMW item{};
            item.mask = LVIF_PARAM;
            item.iItem = sel;
            ListView_GetItem(g_hwndList, &item);
            InstallApps({ (int)item.lParam });
            break;
        }
        case IDC_INSTALL_ALL: {
            if (g_visibleIndices.empty()) break;
            int r = MessageBoxW(hwnd,
                L"Скачать и запустить установку всех программ из текущего списка?\n\n"
                L"Это может занять много времени и места на диске.",
                L"WinHub", MB_YESNO | MB_ICONQUESTION);
            if (r == IDYES) InstallApps(g_visibleIndices);
            break;
        }
        }
        return 0;
    case WM_NOTIFY: {
        auto* nm = (LPNMHDR)lParam;
        if (nm->idFrom == IDC_LIST && nm->code == LVN_ITEMCHANGED)
            UpdateDescription();
        return 0;
    }
    case WM_APP + 1: {
        wchar_t* text = (wchar_t*)lParam;
        if (text) {
            SetWindowTextW(g_hwndStatus, text);
            free(text);
        }
        return 0;
    }
    case WM_APP + 2:
        SendMessageW(g_hwndProgress, PBM_SETPOS, wParam, 0);
        return 0;
    case WM_APP + 3:
        SetProgress(100, L"Готово! Установщики запущены.");
        SetBusy(false);
        MessageBoxW(hwnd,
            L"Загрузка завершена.\nСледуйте инструкциям установщиков на экране.",
            L"WinHub", MB_OK | MB_ICONINFORMATION);
        return 0;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(220, 220, 225));
        SetBkColor(hdc, RGB(24, 24, 28));
        static HBRUSH brush = CreateSolidBrush(RGB(24, 24, 28));
        return (LRESULT)brush;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(230, 230, 235));
        SetBkColor(hdc, RGB(35, 35, 42));
        static HBRUSH brushEdit = CreateSolidBrush(RGB(35, 35, 42));
        return (LRESULT)brushEdit;
    }
    case WM_DESTROY:
        DeleteObject(g_fontUi);
        DeleteObject(g_fontBold);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int show) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.lpszClassName = L"WinHubMain";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"WinHubMain", L"WinHub — Центр загрузки ПО для Windows",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 980, 640,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
