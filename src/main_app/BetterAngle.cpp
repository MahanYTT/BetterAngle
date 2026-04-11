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
#include <filesystem>

#include "shared/Input.h"
#include "shared/Overlay.h"
#include "shared/Logic.h"
#include "shared/Detector.h"
#include "shared/Updater.h"
#include "shared/Profile.h"
#include "shared/Tray.h"
#include "shared/ControlPanel.h"
#include "shared/FirstTimeSetup.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QTimer>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "shared/State.h"

// Global State
// Global handles defined in State.h/cpp
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
FovDetector g_detector;

// Verbose logging for QML and Startup
void QtLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::wstring logPath = GetAppRootPath() + L"debug.log";
    std::wofstream out(logPath, std::ios::app);
    if (out.is_open()) {
        switch (type) {
            case QtDebugMsg: out << L"[DEBUG] "; break;
            case QtInfoMsg: out << L"[INFO] "; break;
            case QtWarningMsg: out << L"[WARN] "; break;
            case QtCriticalMsg: out << L"[CRIT] "; break;
            case QtFatalMsg: out << L"[FATAL] "; break;
        }
        out << msg.toStdWString() << L" (" << context.file << L":" << context.line << L")" << std::endl;
    }
}

// FOV Detector Thread
void DetectorThread() {
    while (g_running) {
        if (!g_allProfiles.empty() && g_currentSelection == NONE) {
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            g_logic.LoadProfile(p.sensitivityX);

            bool fortFocused = IsFortniteFocused();
            g_fortniteFocusedCache = fortFocused;

            if (fortFocused || g_currentSelection != NONE) {
                // Only scan and change angle scale when Fortnite is in focus OR during ROI selection
                RoiConfig cfg = { p.roi_x, p.roi_y, p.roi_w, p.roi_h, p.target_color, p.tolerance };
                g_detectionRatio = g_detector.Scan(cfg);
                if (g_forceDetection) g_detectionRatio = 1.0f;

                if (g_forceDiving) {
                    g_isDiving = true;
                    g_logic.SetDivingState(true);
                } else if (g_detectionRatio >= g_freefallThreshold) {
                    g_isDiving = true;
                    g_logic.SetDivingState(true);
                } else if (g_detectionRatio <= g_glideThreshold) {
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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Screen Snapshot for Flicker-Free Selection (v4.9.15)
void CaptureDesktop() {
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    g_virtScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    g_virtScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (g_screenSnapshot) DeleteObject(g_screenSnapshot);
    g_screenSnapshot = CreateCompatibleBitmap(hdcScreen, sw, sh);
    HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);
    
    // Capture the entire virtual desktop
    BitBlt(hdcMem, 0, 0, sw, sh, hdcScreen, g_virtScreenX, g_virtScreenY, SRCCOPY);
    
    SelectObject(hdcMem, hOld);
    ReleaseDC(NULL, hdcScreen);
    DeleteDC(hdcMem);
}

// Refreshes all global hotkeys for the HUD window
void RefreshHotkeys(HWND hWnd) {
    if (!hWnd) {
        qDebug() << "RefreshHotkeys: HUD window not ready yet.";
        return;
    }
    for (int i = 1; i <= 6; i++) UnregisterHotKey(hWnd, i);

    if (!g_allProfiles.empty()) {
        Profile& p = g_allProfiles[g_selectedProfileIdx];
        
        auto reg = [&](int id, UINT mod, UINT key, const wchar_t* name) {
            if (key == 0) return;
            // Add MOD_NOREPEAT to ensure hotkey works reliably on Win7+
            if (!RegisterHotKey(hWnd, id, mod | MOD_NOREPEAT, key)) {
                qDebug() << "HOTKEY ERROR: Failed to register" << name << "Key:" << key << "Error:" << GetLastError();
            } else {
                qDebug() << "HOTKEY OK:" << name << "Registered successfully.";
            }
        };

        reg(1, p.keybinds.toggleMod, p.keybinds.toggleKey, L"Toggle");
        reg(2, p.keybinds.roiMod,    p.keybinds.roiKey,    L"ROI");
        reg(3, p.keybinds.crossMod,  p.keybinds.crossKey,  L"Crosshair");
        reg(4, p.keybinds.zeroMod,   p.keybinds.zeroKey,   L"Zero");
        reg(5, p.keybinds.debugMod,  p.keybinds.debugKey,  L"Debug");
    }
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
            RefreshHotkeys(hWnd);
            return 0;
            
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            // Handled via WM_HOTKEY to avoid double-toggles
            break;

        case WM_HOTKEY:
            switch (wParam)
            {
                case 1: // Toggle Panel
                    ShowControlPanel();
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
                        if (g_screenSnapshot) { DeleteObject(g_screenSnapshot); g_screenSnapshot = NULL; }
                        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;
                case 3: // Crosshair
                    g_showCrosshair = !g_showCrosshair;
                    if (!g_allProfiles.empty()) {
                        Profile& p = g_allProfiles[g_selectedProfileIdx];
                        p.showCrosshair = g_showCrosshair;
                        p.Save(GetProfilesPath() + p.name + L".json");
                    }
                    SaveSettings();
                    if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
                    break;
                case 4:
                    g_currentAngle = 0.0f;
                    g_logic.SetZero();
                    break;
                case 5:
                    g_debugMode = !g_debugMode;
                    break;
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayContextMenu(hWnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowControlPanel();
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
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
                    // Adjust color sample coord by the same virtual screen offset used in CaptureDesktop
                    COLORREF pixel = GetPixel(hdcMem, cur.x - g_virtScreenX, cur.y - g_virtScreenY);
                    
                    g_pickedColor = pixel;
                    g_targetColor = pixel;
                    SelectObject(hdcMem, hOld);
                    DeleteDC(hdcMem);
                    ReleaseDC(NULL, hdcScreen);
                }

                // Finalize and Exit Selection
                g_currentSelection = NONE;
                g_isSelectionActive = false;
                if (g_screenSnapshot) { DeleteObject(g_screenSnapshot); g_screenSnapshot = NULL; }
                SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
                InvalidateRect(hWnd, NULL, FALSE);

                if (!g_allProfiles.empty()) {
                    Profile& p = g_allProfiles[g_selectedProfileIdx];
                    p.target_color = g_pickedColor;
                    p.roi_x = g_selectionRect.left;
                    p.roi_y = g_selectionRect.top;
                    p.roi_w = g_selectionRect.right - g_selectionRect.left;
                    p.roi_h = g_selectionRect.bottom - g_selectionRect.top;
                    
                    // Re-save to ensure the correct values are on disk too
                    p.Save(GetProfilesPath() + p.name + L".json");
                    
                    g_showROIBox = true;
                    SaveSettings();
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
            if (wParam == 1) { // 60fps HUD / Input processing timer
                if (g_currentSelection == NONE) {
                    bool lDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                    POINT pt; GetCursorPos(&pt);
                    if (lDown && !g_isDraggingHUD) {
                        // Corrected hitbox: account for virtual screen offsets
                        if (pt.x >= g_hudX + g_virtScreenX && pt.x <= g_hudX + g_virtScreenX + 260 && 
                            pt.y >= g_hudY + g_virtScreenY && pt.y <= g_hudY + g_virtScreenY + 150) {
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

                    // SAFETY GUARD: Enforce Click-Through
                    long ex = GetWindowLong(hWnd, GWL_EXSTYLE);
                    if (!(ex & WS_EX_TRANSPARENT)) {
                        SetWindowLong(hWnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT);
                    }
                }

                static float lastAngle = -9999.0f;
                static bool  lastDiving = false;
                static bool  lastCursor = false;
                CURSORINFO ci = { sizeof(CURSORINFO) };
                if (GetCursorInfo(&ci)) g_isCursorVisible = (ci.flags & CURSOR_SHOWING);
                float ang = g_logic.GetAngle();
                
                bool pulseActive = (g_showCrosshair && g_crossPulse);
                
                if (ang != lastAngle || g_isDiving != lastDiving || g_isCursorVisible != lastCursor
                    || g_currentSelection != NONE || g_showCrosshair || pulseActive) {
                    lastAngle  = ang;
                    lastDiving = g_isDiving;
                    lastCursor = g_isCursorVisible;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            } else if (wParam == 2) { // 30s Auto-Save Periodic Timer
                SaveSettings();
                if (!g_allProfiles.empty() && g_selectedProfileIdx < (int)g_allProfiles.size()) {
                    g_allProfiles[g_selectedProfileIdx].Save(GetProfilesPath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                }
            }
            return 0;
        }

        case WM_PAINT:
            DrawOverlay(hWnd, g_logic.GetAngle(), g_detectionRatio, g_showCrosshair);
            return 0;

        case WM_SYSCOMMAND:
            // Block F10 from opening the system menu (interferes with Fn+F10 keybind)
            if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
            break;

        case WM_CLOSE:
            g_running = false;
            PostQuitMessage(0);
            QCoreApplication::quit();
            return 0;

        case WM_DESTROY:
            g_running = false;
            RemoveSystrayIcon(hWnd);
            QCoreApplication::exit(0);
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


// WinMain...

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // 1. Recovery Mode: Holding SHIFT during startup resets everything
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        if (MessageBoxW(NULL, L"BetterAngle: Hold SHIFT to Reset Settings?\n\nThis will clear your profiles and restore defaults. This is recommended if the app is not starting correctly.", L"BetterAngle Recovery", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            std::wstring root = GetAppRootPath();
            std::filesystem::remove_all(root);
            MessageBoxW(NULL, L"Settings have been reset. BetterAngle will now start in Setup mode.", L"Reset Complete", MB_OK | MB_ICONINFORMATION);
        }
    }

    // Use robust Win32 arguments for Qt
    qInstallMessageHandler(QtLogHandler);
    QGuiApplication app(__argc, __argv);

    // Phase 0: Kick off version check in background
    qDebug() << "BetterAngle Starting... Version:" << VERSION_STR;
    // g_updateAvailable will be set when done; the control panel UPDATES tab shows it.
    std::thread([]() {
        CheckForUpdates();
    }).detach();
    
    // Register HUD class early
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = HUDWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleHUD";
    RegisterClass(&wc);

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // Phase 1: Engine and Backend Setup
    EnsureEngineInitialized(); // Creates g_qmlEngine and registers "backend"

    // Phase 4: Launch UI (Splash first)
    ShowSplashScreen(); 
    
    // DELAY Dashboard load to prevent CPU/GPU contention
    QTimer::singleShot(1500, []() {
        qDebug() << "Loading Main Dashboard components...";
        CreateControlPanel(GetModuleHandle(NULL)); 
    });

    // 2. Defer heavy initialization
    QTimer::singleShot(100, [=]() {
        // Phase 1: Startup Sequence
        LoadSettings();
        CleanupUpdateJunk();

        // Check for profiles early
        g_allProfiles = GetProfiles(GetProfilesPath());

        if (g_allProfiles.empty() || g_needsSetup) {
            g_needsSetup = true;
            g_allProfiles.clear();
            Profile def;
            def.name = L"Default";
            def.sensitivityX = 0.05; def.sensitivityY = 0.05;
            def.showCrosshair = true;
            def.crossThickness = 2.0f;
            def.crossColor = RGB(0, 255, 204);
            def.tolerance = 2;
            g_allProfiles.push_back(def);
            g_selectedProfileIdx = 0;
        }

        g_currentProfile = g_allProfiles[g_selectedProfileIdx];
        
        // Sync Crosshair Settings
        g_crossThickness = g_currentProfile.crossThickness;
        g_crossColor     = g_currentProfile.crossColor;
        g_crossOffsetX   = g_currentProfile.crossOffsetX;
        g_crossOffsetY   = g_currentProfile.crossOffsetY;
        g_crossAngle     = g_currentProfile.crossAngle;
        g_crossPulse     = g_currentProfile.crossPulse;

        g_logic.LoadProfile(g_currentProfile.sensitivityX);

        // 3. Create Windows
        WNDCLASS wcMsg = { 0 };
        wcMsg.lpfnWndProc = MsgWndProc;
        wcMsg.hInstance = hInstance;
        wcMsg.lpszClassName = L"BetterAngleMsgWnd";
        RegisterClass(&wcMsg);
        HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
        RegisterRawMouse(hMsgWnd);
        
        g_virtScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
        g_virtScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        g_hHUD = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            L"BetterAngleHUD", L"BetterAngle HUD",
            WS_POPUP,
            g_virtScreenX, g_virtScreenY, screenW, screenH,
            NULL, NULL, hInstance, NULL
        );

        AddSystrayIcon(g_hHUD);
        
        SetTimer(g_hHUD, 1, 16, NULL); // Repaint Timer
        SetTimer(g_hHUD, 2, 30000, NULL); // Auto-Save Timer

        static std::thread detThread(DetectorThread);
        detThread.detach(); // Allow it to run independently
    });

    // 4. Start the event loop (this draws the splash immediately)
    app.setQuitOnLastWindowClosed(false);
    return app.exec();
}
