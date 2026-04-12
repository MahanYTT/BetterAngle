#include <algorithm>
#include <atomic>
#include <cmath>
#include <dwmapi.h>
#include <filesystem>
#include <fstream>
#include <gdiplus.h>
#include <iostream>
#include <shlobj.h>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

#include "shared/BetterAngleBackend.h"
#include "shared/ControlPanel.h"
#include "shared/Detector.h"
#include "shared/FirstTimeSetup.h"
#include "shared/Input.h"
#include "shared/Logic.h"
#include "shared/Overlay.h"
#include "shared/Profile.h"
#include "shared/Tray.h"
#include "shared/Updater.h"
#include "shared/State.h"

#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Global State
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
FovDetector g_detector;
extern BetterAngleBackend *g_backend;

// ─── Logging ────────────────────────────────────────────────────────────────
static void WriteLog(const std::wstring &msg) {
    std::wstring logPath = GetAppRootPath() + L"debug.log";
    std::wofstream out(logPath, std::ios::app);
    if (out.is_open()) {
        out << msg << std::endl;
    }
}

void QtLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::wstring logPath = GetAppRootPath() + L"debug.log";
    std::wofstream out(logPath, std::ios::app);
    if (out.is_open()) {
        out << L"[QT] " << msg.toStdWString() << std::endl;
    }
}

// ─── MAIN APP ENTRY ─────────────────────────────────────────────────────────
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WriteLog(L"[BOOT] --- BetterAngle Pro Starting ---");
    
    // 1. Recovery Mode (SHIFT)
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        if (MessageBoxW(NULL, L"BetterAngle Recovery Mode: Reset settings to defaults?", L"BetterAngle Recovery", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            std::filesystem::remove_all(GetAppRootPath());
        }
    }

    // 2. Qt Startup
    int argc = 0;
    char** argv = nullptr;
    qInstallMessageHandler(QtLogHandler);
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // 3. System Components (GDI+, Win32 Class)
    GdiplusStartupInput gsi;
    GdiplusStartup(&g_gdiplusToken, &gsi, NULL);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = HUDWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleHUD";
    RegisterClass(&wc);

    // 4. Load Data
    LoadSettings();
    CleanupUpdateJunk();
    g_allProfiles = GetProfiles(GetProfilesPath());
    if (g_allProfiles.empty() || g_needsSetup) {
        g_needsSetup = true;
        g_allProfiles.clear();
        Profile def; def.name = L"Default"; def.sensitivityX = 0.05; def.sensitivityY = 0.05;
        g_allProfiles.push_back(def);
        g_selectedProfileIdx = 0;
    }
    g_currentProfile = g_allProfiles[g_selectedProfileIdx];
    g_logic.LoadProfile(g_currentProfile.sensitivityX);

    // 5. Win32 Windows
    WNDCLASS wcMsg = { 0 };
    wcMsg.lpfnWndProc = MsgWndProc;
    wcMsg.hInstance = hInstance;
    wcMsg.lpszClassName = L"BetterAngleMsgWnd";
    RegisterClass(&wcMsg);
    HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    RegisterRawMouse(hMsgWnd);

    g_virtScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    g_virtScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    g_hHUD = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, L"BetterAngleHUD", L"HUD", WS_POPUP, g_virtScreenX, g_virtScreenY, sw, sh, NULL, NULL, hInstance, NULL);

    AddSystrayIcon(g_hHUD);
    SetTimer(g_hHUD, 1, 16, NULL);

    // 6. QML ENGINE BOOT (Phased)
    qDebug() << "[BOOT] Phase 1: Engine Init";
    EnsureEngineInitialized(); 
    
    qDebug() << "[BOOT] Phase 2: Show Splash";
    ShowSplashScreen();

    // DELAY Dashboard to prevent hang
    QTimer::singleShot(1500, [hInstance]() {
        qDebug() << "[BOOT] Phase 3: Loading Main UI";
        CreateControlPanel(hInstance);
    });

    // FAIL-SAFE: Force show if nothing happened in 5s
    QTimer::singleShot(5000, []() {
        if (g_backend) {
            qDebug() << "[BOOT] Fail-Safe active.";
            g_backend->requestShowControlPanel();
        }
    });

    // 7. Background Jobs
    std::thread(CheckForUpdates).detach();
    std::thread(DetectorThread).detach();

    WriteLog(L"[BOOT] Event loop entering...");
    return app.exec();
}
