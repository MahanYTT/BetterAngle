#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <atomic>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>

#include "shared/State.h"
#include "shared/Profile.h"
#include "shared/Logic.h"
#include "shared/Input.h"
#include "shared/Overlay.h"
#include "shared/Tray.h"
#include "shared/Updater.h"
#include "shared/ControlPanel.h"
#include "shared/BetterAngleBackend.h"

// Global Definitions
HINSTANCE g_hInstance = NULL;
HWND      g_hHUD      = NULL;
ULONG_PTR g_gdiplusToken;

// External Globals
extern BetterAngleBackend* g_backend;
extern AngleLogic          g_logic;

// Prototypes
LRESULT CALLBACK HUDWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // 0. Single Instance Check (v4.27.0 - Hardened Mutex) - ABSOLUTE TOP
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"BetterAnglePro_MainInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0; 
    }

    // 1. ENGINE-FIRST BOOT (v4.27.13)
    static int   argc = 1;
    static char* arg0 = const_cast<char*>("BetterAngle");
    static char* argv_arr[] = { arg0, nullptr };
    char**       argv = argv_arr;
    
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    
    Q_INIT_RESOURCE(qml);
    g_hInstance = hInstance;

    // 2. SHOW SPLASH IMMEDIATELY (Starts painting as soon as app.exec() hits)
    EnsureEngineInitialized();
    ShowSplashScreen();

    // 3. DEFERRED SYSTEM INIT (Runs inside the event loop)
    QTimer::singleShot(0, [hInstance]() {
        qDebug() << "[BOOT] Starting Deferred Win32/HW Initialization...";
        
        SetProcessDPIAware();
        
        GdiplusStartupInput gsi;
        GdiplusStartup(&g_gdiplusToken, &gsi, NULL);

        // Register Win32 classes
        WNDCLASS wc = {};
        wc.lpfnWndProc = HUDWndProc; wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.lpszClassName = L"BetterAngleHUD";
        RegisterClass(&wc);

        WNDCLASS wcMsg = {};
        wcMsg.lpfnWndProc = MsgWndProc; wcMsg.hInstance = hInstance;
        wcMsg.lpszClassName = L"BetterAngleMsgWnd";
        RegisterClass(&wcMsg);

        // SYNC STARTUP (v4.27.8 - Background Thread)
        std::thread([hInstance]() {
            qDebug() << "[BOOT] Background Thread: Loading Data...";
            g_loadingProgress = 10;
            LoadSettings();
            g_loadingProgress = 30;
            CleanupUpdateJunk();
            g_loadingProgress = 40;
            g_allProfiles = GetProfiles(GetProfilesPath());
            g_loadingProgress = 70;
            
            if (g_allProfiles.empty() || g_needsSetup) {
                g_needsSetup = true;
                g_allProfiles.clear();
                Profile def; def.name = L"Default"; def.sensitivityX = 0.05; def.sensitivityY = 0.05;
                def.showCrosshair = true; def.crossThickness = 2.0f;
                def.crossColor = RGB(0, 255, 204); def.tolerance = 2;
                g_allProfiles.push_back(def);
                g_selectedProfileIdx = 0;
            }

            if (g_selectedProfileIdx < 0 || g_selectedProfileIdx >= (int)g_allProfiles.size()) {
                g_selectedProfileIdx = 0;
            }

            g_currentProfile = g_allProfiles[g_selectedProfileIdx];
            g_crossThickness = g_currentProfile.crossThickness;
            g_crossColor     = g_currentProfile.crossColor;
            g_crossOffsetX   = g_currentProfile.crossOffsetX;
            g_crossOffsetY   = g_currentProfile.crossOffsetY;
            g_crossPulse     = g_currentProfile.crossPulse;
            g_logic.LoadProfile(g_currentProfile.sensitivityX);
            
            g_loadingProgress = 100;
            qDebug() << "[BOOT] Data Ready. Triggering UI transition...";
            if (g_backend) {
                QMetaObject::invokeMethod(g_backend, "requestShowControlPanel", Qt::QueuedConnection);
            }
        }).detach();

        // Register Input
        HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
        RegisterRawMouse(hMsgWnd);

        // HUD Setup
        int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        g_hHUD = CreateWindowEx(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,
                                 L"BetterAngleHUD", L"BetterAngle HUD", WS_POPUP,
                                 GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), sw, sh,
                                 NULL, NULL, hInstance, NULL);
        SetLayeredWindowAttributes(g_hHUD, 0, 255, LWA_ALPHA);

        // Wait for stability then add UI components
        QTimer::singleShot(6000, []() {
            AddSystrayIcon(g_hHUD);
            SetTimer(g_hHUD, 1, 32, NULL);    
            SetTimer(g_hHUD, 2, 30000, NULL);
            RefreshHotkeys(g_hHUD);
        });

        // Dashboard Warm-up
        CreateControlPanel(hInstance);
    });

    // 4. MASTER FAIL-SAFE (v4.27.9)
    QTimer::singleShot(5000, []() {
        if (g_backend) g_backend->requestShowControlPanel();
    });

    // 5. MASTER LOGGING RE-ENABLE
    QTimer::singleShot(15000, []() {
        // qInstallMessageHandler(QtLogHandler);
    });

    return app.exec();
}
