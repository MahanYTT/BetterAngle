@echo off
echo Building BetterAngle v4.9.12 Pro Edition...

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
    echo [SUCCESS] BetterAngle v4.9.12 Pro binaries created in bin/
) else (
    echo [ERROR] Build failed.
)

pause
