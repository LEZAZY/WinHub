@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"
set "OUT=WinHub.exe"
set "SRC=src\main.cpp src\download.cpp"
if not exist build mkdir build

where cl >nul 2>&1
if not errorlevel 1 goto build_msvc

set "GPP="
where g++ >nul 2>&1 && set "GPP=g++"
if not defined GPP if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe" (
    set "GPP=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"
)
if not defined GPP if exist "C:\mingw64\bin\g++.exe" set "GPP=C:\mingw64\bin\g++.exe"
if defined GPP goto build_mingw

echo.
echo ERROR: C++ compiler not found.
echo Install: winget install BrechtSanders.WinLibs.POSIX.UCRT
echo Or: Visual Studio Build Tools with C++
exit /b 1

:build_msvc
echo [WinHub] Building with MSVC...
cl /nologo /EHsc /O2 /MT /DUNICODE /D_UNICODE /Fe:build\%OUT% %SRC% ^
    user32.lib gdi32.lib comctl32.lib winhttp.lib urlmon.lib shell32.lib dwmapi.lib ole32.lib ^
    /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:WinHub.manifest
goto build_done

:build_mingw
echo [WinHub] Building with MinGW...
"%GPP%" -O2 -std=c++17 -municode -mwindows -static -static-libgcc -static-libstdc++ ^
    -o build\%OUT% %SRC% ^
    -luser32 -lgdi32 -lcomctl32 -lwinhttp -lurlmon -lshell32 -ldwmapi -lole32

:build_done
if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)
echo.
echo OK: build\%OUT%
echo Run: build\%OUT%
exit /b 0
