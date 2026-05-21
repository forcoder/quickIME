@echo off
:: QuickInput Portable Version Creation Script
:: Creates a standalone portable version of QuickInput IME

setlocal enabledelayedexpansion

:: Configuration variables
set PORTABLE_NAME=QuickInput_Portable_v1.0.0
set SOURCE_DIR=..
set OUTPUT_DIR=..\portable_output
set BUILD_DIR=..\build\release

:: Colors for output
set "GREEN=[INFO]"
set "RED=[ERROR]"
set "YELLOW=[WARN]"

echo %GREEN% Starting QuickInput Portable Version Creation...
echo.

:: Check if source directory exists
if not exist "%SOURCE_DIR%" (
    echo %RED% Source directory not found: %SOURCE_DIR%
    goto :error
)

:: Create output directory
if exist "%OUTPUT_DIR%" (
    echo %YELLOW% Output directory exists, removing existing contents...
    rmdir /s /q "%OUTPUT_DIR%"
)
mkdir "%OUTPUT_DIR%" 2>nul || (
    echo %RED% Failed to create output directory: %OUTPUT_DIR%
    goto :error
)

:: Create portable directory structure
set PORTABLE_ROOT=%OUTPUT_DIR%\%PORTABLE_NAME%
mkdir "%PORTABLE_ROOT%" 2>nul
mkdir "%PORTABLE_ROOT%\bin" 2>nul
mkdir "%PORTABLE_ROOT%\models" 2>nul
mkdir "%PORTABLE_ROOT%\resources" 2>nul
mkdir "%PORTABLE_ROOT%\config" 2>nul
mkdir "%PORTABLE_ROOT%\logs" 2>nul

echo %GREEN% Created portable directory structure:
echo   %PORTABLE_ROOT%
echo   +-- bin/
echo   +-- models/
echo   +-- resources/
echo   +-- config/
echo   +-- logs/

:: Copy executable files
echo %GREEN% Copying executable files...
if exist "%BUILD_DIR%\bin\" (
    xcopy /E /I /Q /Y "%BUILD_DIR%\bin\*.*" "%PORTABLE_ROOT%\bin\" >nul
    echo %GREEN% Copied executables to bin/
) else (
    echo %YELLOW% No build directory found, copying from source...
    xcopy /E /I /Q /Y "%SOURCE_DIR%\src\*.exe" "%PORTABLE_ROOT%\bin\" >nul
)

:: Copy model files
echo %GREEN% Copying model files...
if exist "%SOURCE_DIR%\models\" (
    xcopy /E /I /Q /Y "%SOURCE_DIR%\models\*.*" "%PORTABLE_ROOT%\models\" >nul
    echo %GREEN% Copied models to models/
) else (
    echo %YELLOW% No models directory found in source
)

:: Copy resource files
echo %GREEN% Copying resource files...
if exist "%SOURCE_DIR%\resources\" (
    xcopy /E /I /Q /Y "%SOURCE_DIR%\resources\*.*" "%PORTABLE_ROOT%\resources\" >nul
    echo %GREEN% Copied resources to resources/
) else (
    echo %YELLOW% No resources directory found in source
)

:: Copy configuration files
echo %GREEN% Copying configuration files...
xcopy /E /I /Q /Y "%SOURCE_DIR%\config\*.*" "%PORTABLE_ROOT%\config\" >nul
echo %GREEN% Copied configurations to config/

:: Copy documentation
echo %GREEN% Copying documentation...
xcopy /E /I /Q /Y "%SOURCE_DIR%\README.md" "%PORTABLE_ROOT%\" >nul
xcopy /E /I /Q /Y "%SOURCE_DIR%\LICENSE" "%PORTABLE_ROOT%\" >nul
xcopy /E /I /Q /Y "%SOURCE_DIR%\docs\*" "%PORTABLE_ROOT%\docs\" >nul
echo %GREEN% Copied documentation to root and docs/

:: Create shortcuts
echo %GREEN% Creating desktop shortcuts...
call :create_shortcut "QuickInput.exe" "启动 QuickInput 输入法"
call :create_shortcut "config_panel.exe" "QuickInput 配置面板"
call :create_shortcut "portable_version.bat" "创建便携版"

:: Create registry-free registration script
call :create_portable_registration

:: Create uninstall script
call :create_uninstall_script

:: Compress the portable version
call :compress_portable

goto :success

:: Function to create shortcuts
:create_shortcut
set SHORTCUT_NAME=%~1
set SHORTCUT_DESC=%~2

if exist "%PORTABLE_ROOT%\%SHORTCUT_NAME%" (
    echo Creating shortcut for %SHORTCUT_NAME%...

    :: Create VBScript to generate shortcut
    echo Set oWS = WScript.CreateObject("WScript.Shell") > "%TEMP%\create_shortcut.vbs"
    echo sLinkFile = "%USERPROFILE%\Desktop\%SHORTCUT_NAME%.lnk" >> "%TEMP%\create_shortcut.vbs"
    echo Set oLink = oWS.CreateShortcut(sLinkFile) >> "%TEMP%\create_shortcut.vbs"
    echo oLink.TargetPath = "%CD%\%PORTABLE_ROOT%\%SHORTCUT_NAME%" >> "%TEMP%\create_shortcut.vbs"
    echo oLink.WorkingDirectory = "%CD%\%PORTABLE_ROOT%" >> "%TEMP%\create_shortcut.vbs"
    echo oLink.Description = "%SHORTCUT_DESC%" >> "%TEMP%\create_shortcut.vbs"
    echo oLink.IconLocation = "%CD%\%PORTABLE_ROOT%\bin\quickinput.ico" >> "%TEMP%\create_shortcut.vbs"
    echo oLink.Save >> "%TEMP%\create_shortcut.vbs"

    cscript //nologo "%TEMP%\create_shortcut.vbs"
    del "%TEMP%\create_shortcut.vbs"

    echo %GREEN% Created desktop shortcut: %SHORTCUT_NAME%.lnk
)

goto :eof

:: Function to create portable registration
:create_portable_registration
echo @echo off > "%PORTABLE_ROOT%\register_portable.bat"
echo :: Portable Registration Script >> "%PORTABLE_ROOT%\register_portable.bat"
echo setlocal >> "%PORTABLE_ROOT%\register_portable.bat"
echo echo Registering QuickInput as portable application... >> "%PORTABLE_ROOT%\register_portable.bat"
echo echo This will create registry entries for this portable installation only. >> "%PORTABLE_ROOT%\register_portable.bat"
echo reg add "HKCU\SOFTWARE\QuickInput" /v "PortableMode" /t REG_DWORD /d 1 /f >> "%PORTABLE_ROOT%\register_portable.bat"
echo reg add "HKCU\SOFTWARE\QuickInput" /v "InstallPath" /t REG_SZ /d "%CD%\%PORTABLE_ROOT%" /f >> "%PORTABLE_ROOT%\register_portable.bat"
echo echo Registration complete! >> "%PORTABLE_ROOT%\register_portable.bat"
echo pause >> "%PORTABLE_ROOT%\register_portable.bat"
echo endlocal >> "%PORTABLE_ROOT%\register_portable.bat"
echo.
echo %GREEN% Created portable registration script
goto :eof

:: Function to create uninstall script
:create_uninstall_script
echo @echo off > "%PORTABLE_ROOT%\uninstall_portable.bat"
echo :: Portable Uninstall Script >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo setlocal >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo echo Uninstalling QuickInput portable version... >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo reg delete "HKCU\SOFTWARE\QuickInput" /v "PortableMode" /f >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo reg delete "HKCU\SOFTWARE\QuickInput" /v "InstallPath" /f >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo echo Portable version uninstalled. >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo echo Desktop shortcuts were not removed. Please delete them manually if needed. >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo pause >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo endlocal >> "%PORTABLE_ROOT%\uninstall_portable.bat"
echo.
echo %GREEN% Created portable uninstall script
goto :eof

:: Function to compress portable version
:compress_portable
set ZIP_NAME=%PORTABLE_NAME%.zip
echo %GREEN% Creating compressed archive...
if exist "%ZIP_NAME%" (
    del "%ZIP_NAME%"
)
powershell -command "Compress-Archive -Path '%PORTABLE_ROOT%\*' -DestinationPath '%ZIP_NAME%' -Force"
if exist "%ZIP_NAME%" (
    echo %GREEN% Created compressed archive: %ZIP_NAME%
    echo %GREEN% Size: %~z0 bytes
) else (
    echo %YELLOW% Warning: Could not create compressed archive
)

goto :eof

:success
echo.
echo %GREEN% ========================================
echo %GREEN% QuickInput Portable Version Created!
echo %GREEN% ========================================
echo %GREEN% Location: %PORTABLE_ROOT%
echo %GREEN% Archive: %PORTABLE_NAME%.zip
echo.
echo %GREEN% Features created:
echo %GREEN%   - Complete portable package
echo %GREEN%   - Desktop shortcuts
echo %GREEN%   - Registration scripts
echo %GREEN%   - Compressed archive
echo.
echo %GREEN% To use:
echo %GREEN%   1. Extract the zip file to any location
echo %GREEN%   2. Run register_portable.bat (optional)
echo %GREEN%   3. Run QuickInput.exe
echo.
pause
exit /b 0

:error
echo.
echo %RED% ========================================
echo %RED% Error creating portable version!
echo %RED% ========================================
echo.
pause
exit /b 1