@echo off
:: QuickInput IME Registration Script
:: Registers the Text Service Framework (TSF) profiles for QuickInput IME

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

echo [INFO] Registering QuickInput IME...

:: Register TSF profiles using reg add commands
if "!SILENT_MODE!"=="1" (
    goto :silent_register
)

:interactive_register
echo.
echo 正在注册 QuickInput IME...
echo.

:: Create registry entries for TSF
reg add "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /v "ThreadMgrEx" /t REG_DWORD /d 1 /f
reg add "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /v "TextService" /t REG_SZ /d "{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f
reg add "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /ve /d "QuickInput Text Service" /f
reg add "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}\LocalServer32" /ve /d "%%SystemRoot%%\system32\quickinput_ime.dll" /f
reg add "HKCU\SOFTWARE\QuickInput" /ve /d "" /f
reg add "HKCU\SOFTWARE\QuickInput\Config" /ve /d "" /f

echo.
echo 注册完成！
echo QuickInput IME 已成功注册到系统中。
goto :end

:silent_register
:: Silent registration - only essential operations
reg add "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /v "ThreadMgrEx" /t REG_DWORD /d 1 /f >nul 2>&1
reg add "HKLM\SOFTWARE\Microsoft\CTF\SystemShared\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /v "TextService" /t REG_SZ /d "{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /f >nul 2>&1
reg add "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}" /ve /d "QuickInput Text Service" /f >nul 2>&1
reg add "HKCR\CLSID\{B4F3A834-5C91-4E8D-8A7F-3D2C1E6A9F0B}\LocalServer32" /ve /d "%%SystemRoot%%\system32\quickinput_ime.dll" /f >nul 2>&1
reg add "HKCU\SOFTWARE\QuickInput" /ve /d "" /f >nul 2>&1
reg add "HKCU\SOFTWARE\QuickInput\Config" /ve /d "" /f >nul 2>&1

echo [SUCCESS] QuickInput IME registered successfully (silent mode)
goto :end

:end
if "!SILENT_MODE!"=="1" (
    exit /b 0
) else (
    echo.
    pause
)
endlocal