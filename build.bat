@echo off
echo ===============================================================================
echo WARNING: Local builds are deprecated
echo ===============================================================================
echo.
echo All releases must be handled by GitHub Actions.
echo This script requires local MSVC compilation which is not recommended.
echo.
echo For production builds, use GitHub Actions workflow:
echo   - Push to main branch triggers automated build and release
echo   - Or create a tag starting with 'v' (e.g., v4.27.216)
echo.
echo To build locally for development only, you need:
echo   1. Visual Studio 2022 with MSVC
echo   2. Qt 6.5.3 installed
echo   3. Windows SDK
echo.
echo Continue with local build? (Y/N)
set /p choice=
if /i "%choice%" neq "Y" (
    echo Build cancelled. Use GitHub Actions for production releases.
    pause
    exit /b 0
)

echo Building BetterAngle v5.0.33 Pro Edition...

:: Check for MSVC
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] MSVC (cl.exe) not found in PATH.
    pause
    exit /b 1
)

:: Create bin directory
if not exist bin mkdir bin

:: Shared Flags
set FLAGS=/EHsc /O2 /DUNICODE /D_UNICODE /I include /I src
set LIBS=user32.lib gdi32.lib gdiplus.lib dwmapi.lib winhttp.lib shell32.lib d2d1.lib dwrite.lib windowscodecs.lib /SUBSYSTEM:WINDOWS

:: 1. Build Main Application
echo Building BetterAngle.exe (Main)...
cl.exe %FLAGS% src/main_app/BetterAngle.cpp src/shared/*.cpp /Fe:bin/BetterAngle.exe /link %LIBS%

:: 2. Build Calibration Tool
echo Building BetterAngleConfig.exe (Wizard)...
cl.exe %FLAGS% src/config_tool/BetterAngleConfig.cpp src/shared/*.cpp /Fe:bin/BetterAngleConfig.exe /link %LIBS%

if %errorlevel% equ 0 (
    echo [SUCCESS] BetterAngle v5.0.33 Pro binaries created in bin/
    echo NOTE: This is a development build only. For production releases, use GitHub Actions.
) else (
    echo [ERROR] Build failed.
)

pause
