@echo off
echo Building BetterAngle v3.5 Pro Edition...

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
set LIBS=user32.lib gdi32.lib gdiplus.lib dwmapi.lib winhttp.lib shell32.lib /SUBSYSTEM:WINDOWS

:: 1. Build Main Application
echo Building BetterAngle.exe (Main)...
cl.exe /EHsc /O2 /DUNICODE /D_UNICODE /Iinclude /Isrc src/main_app/BetterAngle.cpp src/shared/*.cpp /Fe:bin/BetterAngle.exe /link user32.lib gdi32.lib gdiplus.lib dwmapi.lib winhttp.lib /SUBSYSTEM:WINDOWS

:: 2. Build Calibration Tool
echo Building BetterAngleConfig.exe (Wizard)...
cl.exe /EHsc /O2 /DUNICODE /D_UNICODE /Iinclude /Isrc src/config_tool/BetterAngleConfig.cpp src/shared/*.cpp /Fe:bin/BetterAngleConfig.exe /link user32.lib gdi32.lib gdiplus.lib dwmapi.lib winhttp.lib /SUBSYSTEM:WINDOWS

if %errorlevel% equ 0 (
    echo [SUCCESS] BetterAngle v3.5 Pro binaries created in bin/
) else (
    echo [ERROR] Build failed.
)

pause
