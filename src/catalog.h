#pragma once

#include <windows.h>

struct AppEntry {
    const wchar_t* id;
    const wchar_t* name;
    const wchar_t* category;
    const wchar_t* description;
    const wchar_t* downloadUrl;
    const wchar_t* fileName;
    const wchar_t* installArgs;
};

inline const AppEntry g_catalog[] = {
    {L"firefox", L"Mozilla Firefox", L"Браузеры",
     L"Быстрый приватный браузер с поддержкой расширений",
     L"https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=ru",
     L"FirefoxSetup.exe", L"/S"},

    {L"chrome", L"Google Chrome", L"Браузеры",
     L"Популярный браузер от Google",
     L"https://dl.google.com/chrome/install/latest/chrome_installer.exe",
     L"ChromeSetup.exe", L""},

    {L"7zip", L"7-Zip", L"Архиваторы",
     L"Бесплатный архиватор с высокой степенью сжатия",
     L"https://7zip.org/a/7z2409-x64.exe",
     L"7zip-setup.exe", L"/S"},

    {L"vlc", L"VLC Media Player", L"Медиа",
     L"Универсальный видео- и аудиоплеер",
     L"https://get.videolan.org/vlc/last/win64/vlc-3.0.21-win64.exe",
     L"VLCSetup.exe", L"/S"},

    {L"notepadpp", L"Notepad++", L"Редакторы",
     L"Мощный редактор кода и текста",
     L"https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.7.7/npp.8.7.7.Installer.x64.exe",
     L"npp-setup.exe", L"/S"},

    {L"vscode", L"Visual Studio Code", L"Разработка",
     L"Редактор кода от Microsoft с расширениями",
     L"https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user",
     L"VSCodeSetup.exe", L"/VERYSILENT /NORESTART /MERGETASKS=!runcode"},

    {L"git", L"Git for Windows", L"Разработка",
     L"Система контроля версий для разработчиков",
     L"https://github.com/git-for-windows/git/releases/download/v2.49.0.windows.1/Git-2.49.0-64-bit.exe",
     L"GitSetup.exe", L"/VERYSILENT /NORESTART"},

    {L"python", L"Python 3.12", L"Разработка",
     L"Язык программирования Python с pip",
     L"https://www.python.org/ftp/python/3.12.10/python-3.12.10-amd64.exe",
     L"PythonSetup.exe", L"/quiet InstallAllUsers=0 PrependPath=1"},

    {L"nodejs", L"Node.js LTS", L"Разработка",
     L"JavaScript runtime для серверов и инструментов",
     L"https://nodejs.org/dist/v22.15.0/node-v22.15.0-x64.msi",
     L"NodeSetup.msi", L""},

    {L"discord", L"Discord", L"Соцсети",
     L"Голосовой и текстовый чат для геймеров",
     L"https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x86",
     L"DiscordSetup.exe", L""},

    {L"steam", L"Steam", L"Игры",
     L"Платформа цифровой дистрибуции игр",
     L"https://cdn.cloudflare.steamstatic.com/client/installer/steamsetup.exe",
     L"SteamSetup.exe", L"/S"},

    {L"obs", L"OBS Studio", L"Медиа",
     L"Запись и стриминг экрана",
     L"https://cdn-fastly.obsproject.com/downloads/OBS-Studio-31.0.2-Windows-Installer.exe",
     L"OBSSetup.exe", L"/S"},

    {L"gimp", L"GIMP", L"Графика",
     L"Бесплатный редактор изображений",
     L"https://download.gimp.org/gimp/v3.0/windows/gimp-3.0.0-setup-1.exe",
     L"GIMPSetup.exe", L"/VERYSILENT /NORESTART"},

    {L"libreoffice", L"LibreOffice", L"Офис",
     L"Бесплатный офисный пакет (Word, Excel, PowerPoint)",
     L"https://download.documentfoundation.org/libreoffice/stable/25.2.3/win/x86_64/LibreOffice_25.2.3_Win_x86-64.msi",
     L"LibreOfficeSetup.msi", L"/quiet /norestart"},

    {L"putty", L"PuTTY", L"Утилиты",
     L"SSH и Telnet клиент",
     L"https://the.earth.li/~sgtatham/putty/latest/w64/putty-64bit-0.83-installer.msi",
     L"PuTTYSetup.msi", L"/quiet /norestart"},

    {L"winscp", L"WinSCP", L"Утилиты",
     L"SFTP, FTP и SCP клиент",
     L"https://winscp.net/download/WinSCP-6.5-Setup.exe",
     L"WinSCPSetup.exe", L"/SILENT"},

    {L"everything", L"Everything Search", L"Утилиты",
     L"Мгновенный поиск файлов на диске",
     L"https://www.voidtools.com/Everything-1.4.1.1026.x64-Setup.exe",
     L"EverythingSetup.exe", L"/S"},

    {L"sharex", L"ShareX", L"Утилиты",
     L"Скриншоты, запись экрана и загрузка файлов",
     L"https://github.com/ShareX/ShareX/releases/download/v17.0.0/ShareX-17.0.0-setup.exe",
     L"ShareXSetup.exe", L"/VERYSILENT /NORESTART"},

    {L"cpu-z", L"CPU-Z", L"Система",
     L"Информация о процессоре, памяти и материнской плате",
     L"https://download.cpuid.com/cpu-z/cpu-z_2.15-en.exe",
     L"CPUZSetup.exe", L"/VERYSILENT /NORESTART"},

    {L"hwinfo", L"HWiNFO64", L"Система",
     L"Детальный мониторинг железа и сенсоров",
     L"https://www.hwinfo.com/files/hw64_828.exe",
     L"HWiNFOSetup.exe", L""},
};

inline const int g_catalogCount = sizeof(g_catalog) / sizeof(g_catalog[0]);

inline const wchar_t* g_categories[] = {
    L"Все",
    L"Браузеры",
    L"Архиваторы",
    L"Медиа",
    L"Редакторы",
    L"Разработка",
    L"Соцсети",
    L"Игры",
    L"Графика",
    L"Офис",
    L"Утилиты",
    L"Система",
};

inline const int g_categoryCount = sizeof(g_categories) / sizeof(g_categories[0]);
