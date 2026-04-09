@echo off
setlocal

set "VER=unknown"
if exist VERSION set /p VER=<VERSION

echo Building BetterAngle v%VER% Pro Edition...

where cl >nul 2>nul
if errorlevel 1 goto :msvc_missing

if not exist bin mkdir bin

set "FLAGS=/EHsc /O2 /DUNICODE /D_UNICODE /DAPP_VERSION=%VER% /Iinclude /Isrc"
set "LIBS=user32.lib gdi32.lib gdiplus.lib dwmapi.lib winhttp.lib shell32.lib d2d1.lib dwrite.lib windowscodecs.lib /SUBSYSTEM:WINDOWS"

echo Building BetterAngle.exe (Main)...
cl.exe %FLAGS% src/main_app/BetterAngle.cpp src/shared/*.cpp /Fe:bin/BetterAngle.exe /link %LIBS%
if errorlevel 1 goto :build_failed

echo Building BetterAngleConfig.exe (Wizard)...
cl.exe %FLAGS% src/config_tool/BetterAngleConfig.cpp src/shared/*.cpp /Fe:bin/BetterAngleConfig.exe /link %LIBS%
if errorlevel 1 goto :build_failed

echo [SUCCESS] BetterAngle v%VER% binaries created in bin/
goto :end

:msvc_missing
echo [ERROR] MSVC (cl.exe) not found in PATH.
goto :end

:build_failed
echo [ERROR] Build failed.

:end
pause
endlocal
