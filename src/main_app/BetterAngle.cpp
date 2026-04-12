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
#include "shared/State.h"
#include "shared/Tray.h"
#include "shared/Updater.h"

#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

// ─── Forward Declarations ──────────────────────────────────────────────────
LRESULT CALLBACK HUDWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MsgWndProc(HWND, UINT, WPARAM, LPARAM);
void DetectorThread();
void HandleRawInput(LPARAM lParam);

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Globals defined here
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
FovDetector g_detector;
extern BetterAngleBackend* g_backend; // defined in ControlPanel.cpp
HINSTANCE g_hInstance = NULL;

// ── Logging ──────────────────────────────────────────────────────────────────
void QtLogHandler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    std::wstring logPath = GetAppRootPath() + L"debug.log";
    std::wofstream out(logPath, std::ios::app);
    if (out.is_open()) {
        const wchar_t* lvl = L"[D] ";
        if (type == QtWarningMsg)  lvl = L"[W] ";
        if (type == QtCriticalMsg) lvl = L"[C] ";
        if (type == QtFatalMsg)    lvl = L"[F] ";
        out << lvl << msg.toStdWString() << std::endl;
    }
}

// ── Detector thread ───────────────────────────────────────────────────────────
void DetectorThread() {
    while (g_running) {
        if (!g_allProfiles.empty() && g_currentSelection == NONE) {
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            g_logic.LoadProfile(p.sensitivityX);

            bool focused = IsFortniteFocused();
            g_fortniteFocusedCache = focused;

            if (focused) {
                RoiConfig cfg = { p.roi_x, p.roi_y, p.roi_w, p.roi_h,
                                  p.target_color, p.tolerance };
                g_detectionRatio = g_detector.Scan(cfg);
                if (g_forceDetection) g_detectionRatio = 1.0f;

                if (g_forceDiving || g_detectionRatio >= g_freefallThreshold) {
                    g_isDiving = true;
                    g_logic.SetDivingState(true);
                } else if (g_detectionRatio <= g_glideThreshold) {
                    g_isDiving = false;
                    g_logic.SetDivingState(false);
                }
            } else {
                g_isDiving = false;
                g_logic.SetDivingState(false);
                g_detectionRatio = 0.0f;
                g_fortniteFocusedCache = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ── Screen snapshot for ROI selection ────────────────────────────────────────
static void CaptureScreenSnapshot() {
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int ox = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int oy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    HDC hScr = GetDC(NULL);
    HDC hMem = CreateCompatibleDC(hScr);
    if (g_screenSnapshot) { DeleteObject(g_screenSnapshot); g_screenSnapshot = NULL; }
    g_screenSnapshot = CreateCompatibleBitmap(hScr, sw, sh);
    SelectObject(hMem, g_screenSnapshot);
    BitBlt(hMem, 0, 0, sw, sh, hScr, ox, oy, SRCCOPY);
    DeleteDC(hMem);
    ReleaseDC(NULL, hScr);
}

// ── HUD window procedure ──────────────────────────────────────────────────────
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_TIMER:
        if (wParam == 1) {
            // Repaint
            RECT rc; GetClientRect(hWnd, &rc);
            InvalidateRect(hWnd, &rc, FALSE);
        } else if (wParam == 2) {
            // Auto-save
            if (!g_allProfiles.empty()) {
                Profile& p = g_allProfiles[g_selectedProfileIdx];
                p.Save(GetProfilesPath() + p.name + L".json");
            }
        }
        break;

    case WM_PAINT: {
        // Use the existing DrawOverlay function from Overlay.cpp
        double angle = g_allProfiles.empty() ? 0.0
                     : g_logic.GetAngle();
        bool showCross = g_showCrosshair && !g_allProfiles.empty()
                       && g_allProfiles[g_selectedProfileIdx].showCrosshair;
        DrawOverlay(hWnd, angle, g_detectionRatio, showCross);
        break;
    }

    case WM_HOTKEY:
        if ((int)wParam == 1) { // Toggle panel
            ShowControlPanel();
        } else if ((int)wParam == 2) { // Toggle ROI selection
            if (g_currentSelection == NONE) {
                CaptureScreenSnapshot();
                g_currentSelection = SELECTING_ROI;
                // Remove transparency and ensure top-most
                SetWindowLong(hWnd, GWL_EXSTYLE,
                    GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                SetCapture(hWnd);
            } else {
                g_currentSelection = NONE;
                ReleaseCapture();
                SetWindowLong(hWnd, GWL_EXSTYLE,
                    GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            }
            InvalidateRect(hWnd, NULL, FALSE);
        } else if ((int)wParam == 3) { // Toggle crosshair
            g_showCrosshair = !g_showCrosshair;
            InvalidateRect(hWnd, NULL, FALSE);
        } else if ((int)wParam == 4) { // Zero angle
            g_logic.SetZero();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if ((int)wParam == 5) { // Toggle debug
            g_debugMode = !g_debugMode;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

    case WM_LBUTTONDOWN:
        if (g_currentSelection == SELECTING_ROI) {
            g_startPoint = { LOWORD(lParam), HIWORD(lParam) };
            g_selectionRect = { g_startPoint.x + g_virtScreenX,
                                g_startPoint.y + g_virtScreenY,
                                g_startPoint.x + g_virtScreenX,
                                g_startPoint.y + g_virtScreenY };
        } else if (g_currentSelection == NONE) {
            // HUD drag
            g_isDraggingHUD = true;
            g_dragStartHUD   = { g_hudX, g_hudY };
            g_dragStartMouse = { LOWORD(lParam), HIWORD(lParam) };
        }
        break;

    case WM_MOUSEMOVE:
        if (g_currentSelection == SELECTING_ROI && (wParam & MK_LBUTTON)) {
            POINT cur = { LOWORD(lParam), HIWORD(lParam) };
            g_selectionRect = {
                std::min(g_startPoint.x, cur.x) + g_virtScreenX,
                std::min(g_startPoint.y, cur.y) + g_virtScreenY,
                std::max(g_startPoint.x, cur.x) + g_virtScreenX,
                std::max(g_startPoint.y, cur.y) + g_virtScreenY
            };
            RECT rc; GetClientRect(hWnd, &rc);
            InvalidateRect(hWnd, &rc, FALSE);
        } else if (g_isDraggingHUD && g_currentSelection == NONE) {
            int dx = LOWORD(lParam) - g_dragStartMouse.x;
            int dy = HIWORD(lParam) - g_dragStartMouse.y;
            g_hudX = g_dragStartHUD.x + dx;
            g_hudY = g_dragStartHUD.y + dy;
        }
        break;

    case WM_LBUTTONUP:
        if (g_currentSelection == SELECTING_ROI) {
            // Commit ROI and move to colour selection
            if (!g_allProfiles.empty()) {
                RECT r = g_selectionRect;
                Profile& p = g_allProfiles[g_selectedProfileIdx];
                p.roi_x = r.left; p.roi_y = r.top;
                p.roi_w = r.right  - r.left;
                p.roi_h = r.bottom - r.top;
            }
            g_currentSelection = SELECTING_COLOR;
        } else {
            g_isDraggingHUD = false;
        }
        break;

    case WM_RBUTTONUP:
        if (g_currentSelection == SELECTING_COLOR) {
            // Commit picked colour, exit selection
            if (!g_allProfiles.empty()) {
                g_allProfiles[g_selectedProfileIdx].target_color = g_pickedColor;
                g_targetColor = g_pickedColor;
            }
            g_currentSelection = NONE;
            SetWindowLong(hWnd, GWL_EXSTYLE,
                GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        }
        break;

    case WM_MOUSELEAVE:
    case WM_CAPTURECHANGED:
        g_isDraggingHUD = false;
        break;

    case WM_USER + 100: // WM_TRAYICON
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayContextMenu(hWnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            ShowControlPanel();
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 2001) { // ID_TRAY_EXIT
            g_running = false;
            RemoveSystrayIcon(hWnd);
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
        g_running = false;
        RemoveSystrayIcon(hWnd);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ── Message-only window for raw input ────────────────────────────────────────
LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_INPUT) {
        int dx = GetRawInputDeltaX(lParam);
        if (dx != 0 && !g_allProfiles.empty()) {
            g_logic.Update(dx);
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ── Entry point ───────────────────────────────────────────────────────────────
#include <QResource>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // 0. Single Instance Check (Prevents multiple BetterAngle processes)
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\BetterAngle_Unique_Instance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0; // Exit silently if already running
    }

    Q_INIT_RESOURCE(qml);
    g_hInstance = hInstance;

    // 1. IMMEDIATE Recovery mode (SHIFT held) — Top priority before ANY logic
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        if (MessageBoxW(NULL,
            L"BetterAngle Recovery Mode\n\nReset all settings to defaults?",
            L"BetterAngle Recovery", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            std::filesystem::remove_all(GetAppRootPath());
            MessageBoxW(NULL, L"Reset complete. Starting in setup mode.", L"Done", MB_OK | MB_ICONINFORMATION);
        }
    }

    // Use static argc/argv
    static int   argc = 1;
    static char* arg0 = const_cast<char*>("BetterAngle");
    static char* argv_arr[] = { arg0, nullptr };
    char**       argv = argv_arr;

    qInstallMessageHandler(QtLogHandler);

    {   // Write first log line
        std::wstring logPath = GetAppRootPath() + L"debug.log";
        std::wofstream out(logPath, std::ios::app);
        if (out.is_open()) out << L"[BOOT] --- BetterAngle " << VERSION_WSTR << L" starting ---" << std::endl;
    }

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // GDI+
    GdiplusStartupInput gsi;
    GdiplusStartup(&g_gdiplusToken, &gsi, NULL);

    // Register Win32 window classes
    WNDCLASS wc = {};
    wc.lpfnWndProc   = HUDWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleHUD";
    RegisterClass(&wc);

    WNDCLASS wcMsg = {};
    wcMsg.lpfnWndProc   = MsgWndProc;
    wcMsg.hInstance     = hInstance;
    wcMsg.lpszClassName = L"BetterAngleMsgWnd";
    RegisterClass(&wcMsg);

    // Load settings synchronously
    LoadSettings();
    CleanupUpdateJunk();
    g_allProfiles = GetProfiles(GetProfilesPath());
    
    // Safety Guard: Force setup if profiles are missing
    if (g_allProfiles.empty() || g_needsSetup) {
        g_needsSetup = true;
        g_allProfiles.clear();
        Profile def;
        def.name = L"Default"; def.sensitivityX = 0.05; def.sensitivityY = 0.05;
        def.showCrosshair = true; def.crossThickness = 2.0f;
        def.crossColor = RGB(0, 255, 204); def.tolerance = 2;
        g_allProfiles.push_back(def);
        g_selectedProfileIdx = 0;
    }

    // SAFETY CLAMPING: Ensure index is never out of bounds
    if (g_selectedProfileIdx < 0 || g_selectedProfileIdx >= (int)g_allProfiles.size()) {
        g_selectedProfileIdx = 0;
    }

    g_currentProfile  = g_allProfiles[g_selectedProfileIdx];
    g_crossThickness  = g_currentProfile.crossThickness;
    g_crossColor      = g_currentProfile.crossColor;
    g_crossOffsetX    = g_currentProfile.crossOffsetX;
    g_crossOffsetY    = g_currentProfile.crossOffsetY;
    g_crossAngle      = g_currentProfile.crossAngle;
    g_crossPulse      = g_currentProfile.crossPulse;
    g_logic.LoadProfile(g_currentProfile.sensitivityX);

    // Create message-only window for raw mouse input
    HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0,
                                   0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    RegisterRawMouse(hMsgWnd);

    // Create layered HUD overlay window (covers virtual desktop)
    g_virtScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    g_virtScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    g_hHUD = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"BetterAngleHUD", L"BetterAngle HUD", WS_POPUP,
        g_virtScreenX, g_virtScreenY, sw, sh,
        NULL, NULL, hInstance, NULL);

    // Ensure the HUD is truly transparent and click-through
    SetLayeredWindowAttributes(g_hHUD, 0, 255, LWA_ALPHA);

    // SetLayeredWindowAttributes(g_hHUD, 0, 255, LWA_ALPHA); // REMOVED: Switching to UpdateLayeredWindow
    AddSystrayIcon(g_hHUD);
    SetTimer(g_hHUD, 1, 16, NULL);    // ~60 fps repaint
    SetTimer(g_hHUD, 2, 30000, NULL); // auto-save
    RefreshHotkeys(g_hHUD);

    // Background threads
    std::thread(CheckForUpdates).detach();
    std::thread(DetectorThread).detach();

    // QML engine — context properties MUST be set before load()
    qDebug() << "[BOOT] Initialising QML engine...";
    EnsureEngineInitialized();

    qDebug() << "[BOOT] Loading Splash...";
    ShowSplashScreen();

    qDebug() << "[BOOT] Entering event loop.";
    return app.exec();
}
