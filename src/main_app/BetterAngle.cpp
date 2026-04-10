#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <dwmapi.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>

#include "shared/Input.h"
#include "shared/Overlay.h"
#include "shared/Logic.h"
#include "shared/Detector.h"
#include "shared/Updater.h"
#include "shared/Profile.h"
#include "shared/Tray.h"
#include "shared/Startup.h"
#include "shared/ControlPanel.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "shared/State.h"

// Global State
// Global HUD and Panel Handles
HWND g_hHUD = NULL;
HWND g_hPanel = NULL;
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
FovDetector g_detector;

// Only update sensitivity state when Fortnite is the foreground window
static bool IsFortniteFocused() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    wchar_t cls[256] = { 0 };
    GetClassNameW(fg, cls, 256);
    if (wcscmp(cls, L"UnrealWindow") != 0) return false;
    wchar_t title[256] = { 0 };
    GetWindowTextW(fg, title, 256);
    return wcsstr(title, L"Fortnite") != nullptr;
}

// FOV Detector Thread
void DetectorThread() {
    while (g_running) {
        if (!g_allProfiles.empty() && g_currentSelection == NONE) {
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            g_logic.LoadProfile(p.sensitivityX);

            bool fortFocused = IsFortniteFocused();
            g_fortniteFocusedCache = fortFocused;

            if (fortFocused || g_debugMode) {
                // Only scan and change angle scale when Fortnite is in focus
                RoiConfig cfg = { p.roi_x, p.roi_y, p.roi_w, p.roi_h, p.target_color, p.tolerance };
                g_detectionRatio = g_detector.Scan(cfg);
                if (g_forceDetection) g_detectionRatio = 1.0f;

                if (g_forceDiving) {
                    g_isDiving = true;
                    g_logic.SetDivingState(true);
                } else if (g_detectionRatio >= 0.01f) {
                    g_isDiving = true;
                    g_logic.SetDivingState(true);
                } else {
                    g_isDiving = false;
                    g_logic.SetDivingState(false);
                }
            } else {
                // Fortnite not in focus — hold at normal scale, no changes
                g_isDiving = false;
                g_logic.SetDivingState(false);
                g_detectionRatio = 0.0f;
                g_fortniteFocusedCache = false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Screen Snapshot for Flicker-Free Selection (v4.9.15)
void CaptureDesktop() {
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (g_screenSnapshot) DeleteObject(g_screenSnapshot);
    g_screenSnapshot = CreateCompatibleBitmap(hdcScreen, sw, sh);
    HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);
    BitBlt(hdcMem, 0, 0, sw, sh, hdcScreen, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    ReleaseDC(NULL, hdcScreen);
    DeleteDC(hdcMem);
}

// Message-Only Window for Bullet-Proof Raw Input
LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INPUT) {
        int dx = GetRawInputDeltaX(lParam);
        // Only update angle accumulation (the decimal) if Fortnite is focused or debug mode is ON
        if (g_fortniteFocusedCache || g_debugMode) {
            g_logic.Update(dx);
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// HUD Window Procedure
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            RegisterHotKey(hWnd, 1, g_keybinds.toggleMod, g_keybinds.toggleKey);
            RegisterHotKey(hWnd, 2, g_keybinds.roiMod, g_keybinds.roiKey);
            RegisterHotKey(hWnd, 3, g_keybinds.crossMod, g_keybinds.crossKey);
            RegisterHotKey(hWnd, 4, g_keybinds.zeroMod, g_keybinds.zeroKey);
            RegisterHotKey(hWnd, 5, g_keybinds.debugMod, g_keybinds.debugKey);
            return 0;

        case WM_HOTKEY:
            switch (wParam)
            {
                case 1: // Toggle Panel
                    if (IsIconic(g_hPanel)) ShowWindow(g_hPanel, SW_RESTORE);
                    else if (IsWindowVisible(g_hPanel)) ShowWindow(g_hPanel, SW_MINIMIZE);
                    else ShowWindow(g_hPanel, SW_SHOW);
                    break;
                case 2: // ROI Select Toggle
                    if (g_currentSelection == NONE) {
                        CaptureDesktop(); // Capture before dimming
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
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;
                case 3:
                    g_showCrosshair = !g_showCrosshair;
                    break;
        case 4:
            g_currentAngle = 0.0f;
            g_logic.SetZero();
            break;
        case 5:
            g_debugMode = !g_debugMode;
            break;
    }
    return 0;
        case WM_LBUTTONDOWN:
            if (g_currentSelection == SELECTING_ROI) {
                POINT cur; GetCursorPos(&cur);
                g_startPoint = cur;
                g_selectionRect = { cur.x, cur.y, cur.x, cur.y };
            } else if (g_currentSelection == SELECTING_COLOR) {
                // STAGE 2: PRECISION COLOR PICK (Snap-Shot Bypass)
                if (g_screenSnapshot) {
                    HDC hdcScreen = GetDC(NULL);
                    HDC hdcMem = CreateCompatibleDC(hdcScreen);
                    HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);
                    
                    POINT cur; GetCursorPos(&cur);
                    COLORREF pixel = GetPixel(hdcMem, cur.x, cur.y);
                    // Sync: GDI returns 0x00BBGGRR
                    g_pickedColor = pixel;
                    g_targetColor = pixel;
                    SelectObject(hdcMem, hOld);
                    DeleteDC(hdcMem);
                    ReleaseDC(NULL, hdcScreen);
                }

                // Finalize and Exit Selection
                g_currentSelection = NONE;
                g_isSelectionActive = false;
                SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
                InvalidateRect(hWnd, NULL, FALSE);

                if (!g_allProfiles.empty()) {
                    Profile& p = g_allProfiles[g_selectedProfileIdx];
                    p.target_color = g_pickedColor;
                    p.roi_x = g_selectionRect.left;
                    p.roi_y = g_selectionRect.top;
                    p.roi_w = g_selectionRect.right - g_selectionRect.left;
                    p.roi_h = g_selectionRect.bottom - g_selectionRect.top;
                    
                    // Save to the actual profile path
                    std::wstring profilePath = GetAppStoragePath() + p.name + L".json";
                    p.Save(profilePath);
                    
                    // Also maintain the legacy 'last_calibrated' for quick-load logic if needed
                    p.Save(GetAppStoragePath() + L"last_calibrated.json");
                }
            }
            return 0;

        case WM_MOUSEMOVE:
            if (g_currentSelection != NONE) {
                if (g_currentSelection == SELECTING_ROI && (wParam & MK_LBUTTON)) {
                    POINT cur; GetCursorPos(&cur);
                    g_selectionRect.left = (std::min)(g_startPoint.x, cur.x);
                    g_selectionRect.right = (std::max)(g_startPoint.x, cur.x);
                    g_selectionRect.top = (std::min)(g_startPoint.y, cur.y);
                    g_selectionRect.bottom = (std::max)(g_startPoint.y, cur.y);
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

        case WM_TIMER: {
            if (g_currentSelection == NONE) {
                bool lDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                POINT pt; GetCursorPos(&pt);
                if (lDown && !g_isDraggingHUD) {
                    if (pt.x >= g_hudX && pt.x <= g_hudX + 320 && 
                        pt.y >= g_hudY && pt.y <= g_hudY + 180) {
                        g_isDraggingHUD = true;
                        g_dragStartMouse = pt;
                        g_dragStartHUD.x = g_hudX;
                        g_dragStartHUD.y = g_hudY;
                    }
                } else if (!lDown && g_isDraggingHUD) {
                    g_isDraggingHUD = false;
                    SaveSettings();
                }
                
                if (g_isDraggingHUD && lDown) {
                    g_hudX = g_dragStartHUD.x + (pt.x - g_dragStartMouse.x);
                    g_hudY = g_dragStartHUD.y + (pt.y - g_dragStartMouse.y);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }

            // Only repaint if the angle or diving state actually changed
            static float lastAngle = -9999.0f;
            static bool  lastDiving = false;
            static bool  lastCursor = false;
            CURSORINFO ci = { sizeof(CURSORINFO) };
            if (GetCursorInfo(&ci)) g_isCursorVisible = (ci.flags & CURSOR_SHOWING);
            float ang = g_logic.GetAngle();
            
            // Repaint whenever state changes OR crosshair/pulse is active
            bool pulseActive = (g_showCrosshair && g_crossPulse);
            
            if (ang != lastAngle || g_isDiving != lastDiving || g_isCursorVisible != lastCursor
                || g_currentSelection != NONE || g_showCrosshair || pulseActive) {
                lastAngle  = ang;
                lastDiving = g_isDiving;
                lastCursor = g_isCursorVisible;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_PAINT:
            DrawOverlay(hWnd, g_logic.GetAngle(), g_detectionRatio, g_showCrosshair);
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
    // Phase 0: Kick off version check in background — never blocks startup.
    // g_updateAvailable will be set when done; the control panel UPDATES tab shows it.
    std::thread([]() {
        CheckForUpdates();
    }).detach();

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // Phase 1: Startup Sequence (Splash)
    ShowSplashLoader(hInstance);

    LoadSettings();
    g_allProfiles = GetProfiles(GetAppStoragePath());
    if (g_allProfiles.empty()) {
        Profile p;
        p.name = L"Default";
        p.tolerance = 25;
        p.roi_x = 760; p.roi_y = 640; p.roi_w = 400; p.roi_h = 70;
        p.target_color = RGB(150, 150, 150);
        p.crossThickness = 2.0f;
        p.crossColor = RGB(255,0,0);
        p.Save(GetAppStoragePath() + L"Default.json");
        g_allProfiles.push_back(p);
    }
    
    g_allProfiles[g_selectedProfileIdx].sensitivityX = FetchFortniteSensitivity();
    g_currentProfile = g_allProfiles[g_selectedProfileIdx];
    
    // Guard: if still empty after setup, something went wrong — exit cleanly
    if (g_allProfiles.empty()) {
        MessageBoxW(NULL, L"Setup failed to create a profile. Please restart.", L"BetterAngle Setup Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    g_selectedProfileIdx = 0;
    for (size_t i = 0; i < g_allProfiles.size(); i++) {
        if (g_allProfiles[i].name == g_lastLoadedProfileName) {
            g_selectedProfileIdx = i; break;
        }
    }
    if (g_lastLoadedProfileName.empty() && !g_allProfiles.empty()) {
        g_lastLoadedProfileName = g_allProfiles[0].name;
    }
    
    g_currentProfile = g_allProfiles[g_selectedProfileIdx];
    
    // Sync Crosshair Settings from Profile to Global State
    g_crossThickness = g_currentProfile.crossThickness;
    g_crossColor     = g_currentProfile.crossColor;
    g_crossOffsetX   = g_currentProfile.crossOffsetX;
    g_crossOffsetY   = g_currentProfile.crossOffsetY;
    g_crossAngle     = g_currentProfile.crossAngle;
    g_crossPulse     = g_currentProfile.crossPulse;

    g_logic.LoadProfile(g_currentProfile.sensitivityX); // Now using the auto-fetched 800 DPI matched sens

    // Message Window for Raw Input (Bypasses Layered Window UI Bugs)
    WNDCLASS wcMsg = { 0 };
    wcMsg.lpfnWndProc = MsgWndProc;
    wcMsg.hInstance = hInstance;
    wcMsg.lpszClassName = L"BetterAngleMsgWnd";
    RegisterClass(&wcMsg);
    HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    RegisterRawMouse(hMsgWnd);

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
    SetTimer(g_hHUD, 1, 8, NULL); // ~120fps for snappy crosshair & drag response

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
