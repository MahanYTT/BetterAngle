#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <dwmapi.h>
#include <gdiplus.h>

#include "shared/Input.h"
#include "shared/Overlay.h"
#include "shared/Logic.h"
#include "shared/Detector.h"
#include "shared/Updater.h"
#include "shared/Profile.h"
#include "shared/Tray.h"
#include "shared/Remote.h"
#include "shared/Startup.h"
#include "shared/ControlPanel.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "shared/State.h"

// Global State
HWND g_hHUD = NULL;
HWND g_hPanel = NULL;
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
AngleLogic g_logic(800, 6.5);
FovDetector g_detector;

Profile g_currentProfile;
std::vector<Profile> g_allProfiles;
int g_selectedProfileIdx = 0;
bool g_isCursorVisible = false;

// FOV Detector Thread
void DetectorThread() {
    while (g_running) {
        if (!g_allProfiles.empty() && g_currentSelection == NONE) {
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            RoiConfig cfg = { p.roi_x, p.roi_y, p.roi_w, p.roi_h, p.target_color, p.tolerance };
            
            g_detectionRatio = g_detector.Scan(cfg);
            if (g_detectionRatio > 0.05f) {
                g_logic.SetScale(p.scale_diving);
            } else {
                g_logic.SetScale(p.scale_normal);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// HUD Window Procedure
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            RegisterRawMouse(hWnd);
            RegisterHotKey(hWnd, 1, MOD_CONTROL, 'U'); // Toggle Panel
            RegisterHotKey(hWnd, 2, MOD_CONTROL, 'R'); // ROI Select
            RegisterHotKey(hWnd, 3, 0, VK_F10);        // Crosshair
            return 0;

        case WM_HOTKEY:
            if (wParam == 1) { // Toggle Panel
                if (IsIconic(g_hPanel)) ShowWindow(g_hPanel, SW_RESTORE);
                else if (IsWindowVisible(g_hPanel)) ShowWindow(g_hPanel, SW_MINIMIZE);
                else ShowWindow(g_hPanel, SW_SHOW);
            } else if (wParam == 2) { // ROI Select Toggle
                if (g_currentSelection == NONE) {
                    g_currentSelection = SELECTING_ROI;
                    g_isSelectionActive = true;
                    long exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
                    exStyle &= ~WS_EX_TRANSPARENT;
                    SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);
                    SetForegroundWindow(hWnd);
                } else {
                    g_currentSelection = NONE;
                    g_isSelectionActive = false;
                    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
                }
            } else if (wParam == 3) {
                g_showCrosshair = !g_showCrosshair;
            }
            return 0;

        case WM_LBUTTONDOWN:
            if (g_currentSelection == SELECTING_ROI) {
                POINT cur; GetCursorPos(&cur);
                g_startPoint = cur;
                g_selectionRect = { cur.x, cur.y, cur.x, cur.y };
            } else if (g_currentSelection == SELECTING_COLOR) {
                // STAGE 2: PRECISION COLOR PICK (Color Scope)
                HDC hdcScreen = GetDC(NULL);
                POINT cur; GetCursorPos(&cur);
                g_targetColor = GetPixel(hdcScreen, cur.x, cur.y);
                g_pickedColor = g_targetColor;
                ReleaseDC(NULL, hdcScreen);

                // Finalize and Exit Selection
                g_currentSelection = NONE;
                g_isSelectionActive = false;
                SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

                if (!g_allProfiles.empty()) {
                    Profile& p = g_allProfiles[g_selectedProfileIdx];
                    p.target_color = g_targetColor;
                    p.roi_x = min(g_selectionRect.left, g_selectionRect.right);
                    p.roi_y = min(g_selectionRect.top, g_selectionRect.bottom);
                    p.roi_w = abs(g_selectionRect.right - g_selectionRect.left);
                    p.roi_h = abs(g_selectionRect.bottom - g_selectionRect.top);
                    p.Save(L"profiles/last_calibrated.json");
                }
            }
            return 0;

        case WM_MOUSEMOVE:
            if (g_currentSelection != NONE) {
                if (g_currentSelection == SELECTING_ROI && (wParam & MK_LBUTTON)) {
                    POINT cur; GetCursorPos(&cur);
                    g_selectionRect.right = cur.x;
                    g_selectionRect.bottom = cur.y;
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;

        case WM_LBUTTONUP:
            if (g_currentSelection == SELECTING_ROI) {
                g_currentSelection = SELECTING_COLOR;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;

        case WM_INPUT: {
            if (g_isCursorVisible || g_currentSelection != NONE) return 0;
            int dx = GetRawInputDeltaX(lParam);
            g_logic.Update(dx);
            return 0;
        }

        case WM_TIMER: {
            CURSORINFO ci = { sizeof(CURSORINFO) };
            if (GetCursorInfo(&ci)) {
                g_isCursorVisible = (ci.flags & CURSOR_SHOWING);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_PAINT:
            DrawOverlay(hWnd, g_logic.GetAngle(), g_status.c_str(), g_detectionRatio, g_showCrosshair);
            return 0;

        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Phase 0: Silent Update & Relaunch Sequence
    if (CheckForUpdates()) {
        std::wstring verStr = std::wstring(g_latestVersionOnline.begin(), g_latestVersionOnline.end());
        std::wstring downloadUrl = L"https://github.com/MahanYTT/BetterAngle/releases/latest/download/BetterAngle_" + verStr + L".exe";
        if (DownloadUpdate(downloadUrl, L"update_tmp.exe")) {
            ApplyUpdateAndRestart();
        }
    }

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // Phase 1: Startup Sequence (Splash)
    ShowSplashLoader(hInstance);

    // Initial Load (Syncing was handled or represented in Splash)
    g_allProfiles = GetProfiles(L"profiles");
    if (!g_allProfiles.empty()) g_currentProfile = g_allProfiles[0];

    // Phase 2: Create Control Panel (Interactive)
    g_hPanel = CreateControlPanel(hInstance);
    
    // Phase 3: Create HUD Window (Transparent Overlay)
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = HUDWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleHUD";
    RegisterClass(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hHUD = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"BetterAngleHUD", L"BetterAngle HUD",
        WS_POPUP,
        0, 0, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(g_hHUD, SW_SHOW);
    UpdateWindow(g_hHUD);
    SetTimer(g_hHUD, 1, 25, NULL);

    std::thread detThread(DetectorThread);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        // If Panel is closed by user (WM_QUIT from Panel should not happen but WM_DESTROY does)
        if (!IsWindow(g_hPanel) && !IsWindow(g_hHUD)) break;
    }

    g_running = false;
    if (detThread.joinable()) detThread.join();
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
