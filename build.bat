@echo off
echo Building BetterAngle (C++)...

:: Check for MSVC
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] MSVC (cl.exe) not found in PATH.
    echo Please run this from a 'Developer Command Prompt for VS' or install Visual Studio Build Tools.
    pause
    exit /b 1
)

:: Create bin directory
if not exist bin mkdir bin

:: Compile
cl.exe /EHsc /O2 /I include src\*.cpp /Fe:bin\BetterAngle.exe /link user32.lib gdi32.lib gdiplus.lib dwmapi.lib winhttp.lib /SUBSYSTEM:WINDOWS

if %errorlevel% equ 0 (
    echo [SUCCESS] BetterAngle.exe created in bin/
) else (
    echo [ERROR] Build failed.
)

pause
