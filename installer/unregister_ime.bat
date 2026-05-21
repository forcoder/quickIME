@echo off
:: QuickInput IME Unregistration Script
:: Unregisters the Text Service Framework (TSF) profiles for QuickInput IME

setlocal enabledelayedexpansion

:: Check for silent mode
if "%1"=="/silent" (
    set SILENT_MODE=1
) else (
    set SILENT_MODE=0
)

:: Get the script directory
set SCRIPT_DIR=%~dp0
cd /d %SCRIPT_DIR%

echo [INFO] Unregistering QuickInput IME...

if "!SILENT_MODE!"=="1" (
    goto :silent_unregister
)

:interactive_unregister
echo.
echo 正在注销 QuickInput IME...
echo.

:: Remove TSF registry entries
reg delete "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f >nul 2>&1
reg delete "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f >nul 2>&1
reg delete "HKCU\SOFTWARE\QuickInput" /f >nul 2>&1
reg delete "HKCU\SOFTWARE\QuickInput\Config" /f >nul 2>&1

echo.
echo 注销完成！
echo QuickInput IME 已从系统中移除。
goto :end

:silent_unregister
:: Silent unregistration - remove registry entries silently
reg delete "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f >nul 2>&1
reg delete "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f >nul 2>&1
reg delete "HKCU\SOFTWARE\QuickInput" /f >nul 2>&1
reg delete "HKCU\SOFTWARE\QuickInput\Config" /f >nul 2>&1

echo [SUCCESS] QuickInput IME unregistered successfully (silent mode)
goto :end

:end
if "!SILENT_MODE!"=="1" (
    exit /b 0
) else (
    echo.
    pause
)
endlocal