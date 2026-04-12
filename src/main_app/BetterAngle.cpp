#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <atomic>
#include <algorithm>
#include <vector>

using namespace Gdiplus;

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
ULONG_PTR g_gdiplusToken = 0;
std::atomic<bool> g_winInitialized{false};

// External Globals
extern BetterAngleBackend* g_backend;
extern AngleLogic          g_logic;
extern std::atomic<int>    g_loadingProgress;
extern std::mutex          g_profileMutex;

// Helper Functions
void SetHUDClickable(HWND hWnd, bool clickable) {
    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    if (clickable) exStyle &= ~WS_EX_TRANSPARENT; else exStyle |= WS_EX_TRANSPARENT;
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

HBITMAP CaptureScreen() {
    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN), h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(hdcMem, hbm);
    BitBlt(hdcMem, 0, 0, w, h, hdc, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), SRCCOPY);
    DeleteDC(hdcMem); ReleaseDC(NULL, hdc);
    return hbm;
}

COLORREF GetPixelColor(int x, int y) {
    HDC hdc = GetDC(NULL);
    COLORREF c = GetPixel(hdc, x, y);
    ReleaseDC(NULL, hdc);
    return c;
}

// Window Procedures
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: DrawOverlay(hWnd, g_currentAngle, g_detectionRatio, g_showCrosshair); return 0;
    case WM_TIMER:
        if (wp == 1) InvalidateRect(hWnd, NULL, FALSE);
        if (wp == 2) SaveSettings();
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_HOTKEY:
        if (wp == 1 && g_backend) g_backend->requestToggleControlPanel();
        if (wp == 2) { 
            g_currentSelection = (g_currentSelection == SELECTING_ROI) ? NONE : SELECTING_ROI;
            if (g_currentSelection == SELECTING_ROI) {
                if (g_screenSnapshot) DeleteObject(g_screenSnapshot); 
                g_screenSnapshot = CaptureScreen(); SetHUDClickable(hWnd, true);
            } else { SetHUDClickable(hWnd, false); }
            InvalidateRect(hWnd, NULL, FALSE);
        }
        if (wp == 3) { g_showCrosshair = !g_showCrosshair; InvalidateRect(hWnd, NULL, FALSE); }
        if (wp == 4) { g_logic.SetZero(); InvalidateRect(hWnd, NULL, FALSE); }
        if (wp == 5) { g_debugMode = !g_debugMode; InvalidateRect(hWnd, NULL, FALSE); }
        return 0;
    case WM_LBUTTONDOWN:
        if (g_currentSelection == SELECTING_ROI) {
            g_isSelectionActive = true; g_startPoint = { (short)LOWORD(lp), (short)HIWORD(lp) };
        } else if (g_currentSelection == SELECTING_COLOR) {
            POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) }; 
            g_targetColor = GetPixelColor(pt.x, pt.y); g_currentSelection = NONE; SetHUDClickable(hWnd, false);
        } else {
            g_isDraggingHUD = true; GetCursorPos(&g_dragStartMouse);
            g_dragStartHUD = { g_hudX, g_hudY }; SetCapture(hWnd);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (g_isSelectionActive) {
            POINT cur = { (short)LOWORD(lp), (short)HIWORD(lp) };
            g_selectionRect = { (std::min)(g_startPoint.x, cur.x), (std::min)(g_startPoint.y, cur.y), (std::max)(g_startPoint.x, cur.x), (std::max)(g_startPoint.y, cur.y) };
        } else if (g_isDraggingHUD) {
            POINT cur; GetCursorPos(&cur);
            g_hudX = g_dragStartHUD.x + (cur.x - g_dragStartMouse.x);
            g_hudY = g_dragStartHUD.y + (cur.y - g_dragStartMouse.y);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_LBUTTONUP:
        if (g_isSelectionActive) {
            g_isSelectionActive = false; g_currentSelection = SELECTING_COLOR;
            if (!g_allProfiles.empty()) {
                auto& p = g_allProfiles[g_selectedProfileIdx];
                p.roi_x = g_selectionRect.left; p.roi_y = g_selectionRect.top;
                p.roi_w = g_selectionRect.right - g_selectionRect.left;
                p.roi_h = g_selectionRect.bottom - g_selectionRect.top;
            }
        } else if (g_isDraggingHUD) { g_isDraggingHUD = false; ReleaseCapture(); SaveSettings(); }
        return 0;
    case WM_TRAYICON:
        if (lp == WM_RBUTTONUP) ShowTrayContextMenu(hWnd);
        if (lp == WM_LBUTTONUP) ShowControlPanel();
        return 0;
    case WM_COMMAND: if (LOWORD(wp) == ID_TRAY_EXIT) PostQuitMessage(0); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, msg, wp, lp);
}

LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_INPUT) {
        UINT dwSize; GetRawInputData((HRAWINPUT)lp, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        if (dwSize > 0) {
            std::vector<BYTE> lpb(dwSize);
            if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                RAWINPUT* raw = (RAWINPUT*)lpb.data();
                if (raw->header.dwType == RIM_TYPEMOUSE) g_logic.Update(raw->data.mouse.lLastX);
            }
        }
    }
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wp, lp);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // ── Modern DPI Awareness (V2) for Windows 10/11 ──────────────
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Gdiplus::GdiplusStartupInput gsi; Gdiplus::GdiplusStartup(&g_gdiplusToken, &gsi, NULL);
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"BetterAnglePro_MainInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        MessageBoxW(NULL, L"BetterAngle Pro is already running.\n\nIf the app is not showing, please close the 'BetterAngle.exe' process in Task Manager and try again.", L"BetterAngle Pro - Already Running", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    g_loadingProgress = 5; 
    g_hInstance = hInstance;

    // ── Atomic Boot Thread (Starts BEFORE Splash/Engine) ───────────────
    std::thread([hInstance]() {
        auto startTime = std::chrono::steady_clock::now();
        try {
            g_loadingProgress = 10;
            { std::lock_guard<std::mutex> lock(g_profileMutex); LoadSettings(); }
            
            g_loadingProgress = 30;
            CleanupUpdateJunk();
            
            g_loadingProgress = 40;
            { 
                std::lock_guard<std::mutex> lock(g_profileMutex); 
                g_allProfiles = GetProfiles(GetProfilesPath()); 
                if (g_allProfiles.empty()) {
                    Profile def; def.name = L"Default"; def.sensitivityX = 0.05; def.sensitivityY = 0.05;
                    def.showCrosshair = true; def.crossThickness = 2.0f;
                    def.crossColor = RGB(0, 255, 204); def.tolerance = 2;
                    g_allProfiles.push_back(def); g_selectedProfileIdx = 0;
                    def.Save(GetProfilesPath() + L"Default.json"); SaveSettings();
                }
                if (g_selectedProfileIdx < 0 || g_selectedProfileIdx >= (int)g_allProfiles.size()) g_selectedProfileIdx = 0;
                g_currentProfile = g_allProfiles[g_selectedProfileIdx];
            }
            
            g_loadingProgress = 70;
            {
                std::lock_guard<std::mutex> lock(g_profileMutex);
                g_crossThickness = g_currentProfile.crossThickness; g_crossColor = g_currentProfile.crossColor;
                g_crossOffsetX = g_currentProfile.crossOffsetX; g_crossOffsetY = g_currentProfile.crossOffsetY;
                g_crossPulse = g_currentProfile.crossPulse; g_logic.LoadProfile(g_currentProfile.sensitivityX);
            }
            
            g_loadingProgress = 100;
            std::this_thread::sleep_until(startTime + std::chrono::milliseconds(2500));
            if (g_backend) QMetaObject::invokeMethod(g_backend, "requestShowControlPanel", Qt::QueuedConnection);
        } catch (...) {
            g_loadingProgress = 100; if (g_backend) QMetaObject::invokeMethod(g_backend, "requestShowControlPanel", Qt::QueuedConnection);
        }
    }).detach();

    static int argc = 1; static char* arg0 = const_cast<char*>("BetterAngle"); static char* argv_arr[] = { arg0, nullptr };
    QGuiApplication app(argc, argv_arr); app.setQuitOnLastWindowClosed(false);
    Q_INIT_RESOURCE(qml);

    EnsureEngineInitialized(); ShowSplashScreen();

    // ── Nuclear Backup (Fail-safe) ───────────────────────────────────
    QTimer::singleShot(6000, []() { 
        if (g_backend) QMetaObject::invokeMethod(g_backend, "requestShowControlPanel", Qt::QueuedConnection); 
    });

    QTimer::singleShot(0, [hInstance]() {
        WNDCLASS wc = { 0 }; wc.lpfnWndProc = HUDWndProc; wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_CROSS); wc.lpszClassName = L"BetterAngleHUD"; RegisterClass(&wc);
        WNDCLASS wcMsg = { 0 }; wcMsg.lpfnWndProc = MsgWndProc; wcMsg.hInstance = hInstance;
        wcMsg.lpszClassName = L"BetterAngleMsgWnd"; RegisterClass(&wcMsg);

        HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL); RegisterRawMouse(hMsgWnd);
        int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN), sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        g_hHUD = CreateWindowEx(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE, L"BetterAngleHUD", L"BetterAngle HUD", WS_POPUP, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), sw, sh, NULL, NULL, hInstance, NULL);
        SetLayeredWindowAttributes(g_hHUD, 0, 255, LWA_ALPHA);
        g_winInitialized = true;

        QTimer::singleShot(500, [hInstance]() {
            AddSystrayIcon(g_hHUD); SetTimer(g_hHUD, 1, 32, NULL); SetTimer(g_hHUD, 2, 30000, NULL); RefreshHotkeys(g_hHUD);
            CreateControlPanel(hInstance);
        });
    });
    return app.exec();
}
