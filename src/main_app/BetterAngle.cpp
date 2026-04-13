#include <algorithm>
#include <atomic>
#include <cmath>
#include <dwmapi.h>
#include <fstream>
#include <gdiplus.h>
#include <iostream>
#include <shlobj.h>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

#include "shared/ControlPanel.h"
#include "shared/Detector.h"
#include "shared/Input.h"
#include "shared/Logic.h"
#include "shared/Overlay.h"
#include "shared/Profile.h"
#include "shared/Tray.h"
#include "shared/Updater.h"
#include <QCoreApplication>
#include <QGuiApplication>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "shared/State.h"

// Global State
// Global handles defined in State.h/cpp
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
FovDetector g_detector;

// FOV Detector Thread
void DetectorThread() {
  while (g_running) {
    if (!g_allProfiles.empty() && g_currentSelection == NONE) {
      Profile &p = g_allProfiles[g_selectedProfileIdx];
      g_logic.LoadProfile(p.sensitivityX);

      // Only scan and change angle scale when in debug mode or if always active
      RoiConfig cfg = {p.roi_x, p.roi_y,        p.roi_w,
                       p.roi_h, p.target_color, p.tolerance};
      g_detectionRatio = g_detector.Scan(cfg);
      if (g_forceDetection)
        g_detectionRatio = 1.0f;

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
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// Screen Snapshot for Flicker-Free Selection (v4.9.15)
void CaptureDesktop() {
  int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);

  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);
  if (g_screenSnapshot)
    DeleteObject(g_screenSnapshot);
  g_screenSnapshot = CreateCompatibleBitmap(hdcScreen, sw, sh);
  HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);

  // Capture the entire virtual desktop
  BitBlt(hdcMem, 0, 0, sw, sh, hdcScreen, sx, sy, SRCCOPY);

  SelectObject(hdcMem, hOld);
  ReleaseDC(NULL, hdcScreen);
  DeleteDC(hdcMem);
}

// Refreshes all global hotkeys for the HUD window
bool RefreshHotkeys(HWND hWnd) {
  if (!hWnd)
    return false;

  for (int i = 1; i <= 6; i++)
    UnregisterHotKey(hWnd, i);

  if (g_allProfiles.empty())
    return false;

  Profile &p = g_allProfiles[g_selectedProfileIdx];
  // Use standard registration without MOD_NOREPEAT for maximum compatibility
  bool ok = true;
  ok &=
      RegisterHotKey(hWnd, 1, p.keybinds.toggleMod, p.keybinds.toggleKey) != 0;
  ok &= RegisterHotKey(hWnd, 2, p.keybinds.roiMod, p.keybinds.roiKey) != 0;
  ok &= RegisterHotKey(hWnd, 3, p.keybinds.crossMod, p.keybinds.crossKey) != 0;
  ok &= RegisterHotKey(hWnd, 4, p.keybinds.zeroMod, p.keybinds.zeroKey) != 0;
  ok &= RegisterHotKey(hWnd, 5, p.keybinds.debugMod, p.keybinds.debugKey) != 0;
  return ok;
}

// Message-Only Window for Bullet-Proof Raw Input
LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
  if (message == WM_INPUT) {
    int dx = GetRawInputDeltaX(lParam);
    // Update angle accumulation (the decimal) based on raw input
    g_logic.Update(dx);
    return 0;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

// HUD Window Procedure
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
  switch (message) {
  case WM_CREATE:
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    RefreshHotkeys(hWnd);
    return 0;

  case WM_HOTKEY:
    switch (wParam) {
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
        if (g_screenSnapshot) {
          DeleteObject(g_screenSnapshot);
          g_screenSnapshot = NULL;
        }
        SetWindowLong(hWnd, GWL_EXSTYLE,
                      GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        InvalidateRect(hWnd, NULL, FALSE);
      }
      break;
    case 3:
      g_showCrosshair = !g_showCrosshair;
      SaveSettings();
      if (g_hHUD) {
        InvalidateRect(g_hHUD, NULL, FALSE);
        UpdateWindow(g_hHUD);
      }
      break;
    case 4:
      g_currentAngle = 0.0f;
      g_logic.SetZero();
      break;
    case 5:
      g_debugMode = !g_debugMode;
      InvalidateRect(hWnd, NULL, FALSE);
      UpdateWindow(hWnd);
      break;
    }
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
      POINT cur;
      GetCursorPos(&cur);
      g_startPoint = cur;
      g_selectionRect = {cur.x, cur.y, cur.x, cur.y};
    } else if (g_currentSelection == SELECTING_COLOR) {
      // STAGE 2: PRECISION COLOR PICK (Snap-Shot Bypass)
      if (g_screenSnapshot) {
        HDC hdcScreen = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);

        int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);

        POINT cur;
        GetCursorPos(&cur);
        // Adjust color sample coord by the same virtual screen offset used in
        // CaptureDesktop
        COLORREF pixel = GetPixel(hdcMem, cur.x - sx, cur.y - sy);

        g_pickedColor = pixel;
        g_targetColor = pixel;
        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
      }

      // Finalize and Exit Selection
      g_currentSelection = NONE;
      g_isSelectionActive = false;
      if (g_screenSnapshot) {
        DeleteObject(g_screenSnapshot);
        g_screenSnapshot = NULL;
      }
      SetWindowLong(hWnd, GWL_EXSTYLE,
                    GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
      InvalidateRect(hWnd, NULL, FALSE);

      if (!g_allProfiles.empty()) {
        Profile &p = g_allProfiles[g_selectedProfileIdx];
        p.target_color = g_pickedColor;
        p.roi_x = g_selectionRect.left;
        p.roi_y = g_selectionRect.top;
        p.roi_w = g_selectionRect.right - g_selectionRect.left;
        p.roi_h = g_selectionRect.bottom - g_selectionRect.top;

        // Save to the actual profile path
        std::wstring profilePath = GetProfilesPath() + p.name + L".json";
        p.Save(profilePath);

        // Also maintain the legacy 'last_calibrated' for quick-load logic if
        // needed
        p.Save(GetProfilesPath() + L"last_calibrated.json");
      }
    }
    return 0;

  case WM_MOUSEMOVE:
    if (g_currentSelection != NONE) {
      if (g_currentSelection == SELECTING_ROI && (wParam & MK_LBUTTON)) {
        POINT cur;
        GetCursorPos(&cur);
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
        POINT pt;
        GetCursorPos(&pt);
        if (lDown && !g_isDraggingHUD) {
          if (pt.x >= g_hudX && pt.x <= g_hudX + 260 && pt.y >= g_hudY &&
              pt.y <= g_hudY + 150) {
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
      static bool lastDiving = false;
      static bool lastCursor = false;
      CURSORINFO ci = {sizeof(CURSORINFO)};
      if (GetCursorInfo(&ci))
        g_isCursorVisible = (ci.flags & CURSOR_SHOWING);
      float ang = g_logic.GetAngle();

      bool pulseActive = (g_showCrosshair && g_crossPulse);

      if (ang != lastAngle || g_isDiving != lastDiving ||
          g_isCursorVisible != lastCursor || g_currentSelection != NONE ||
          g_showCrosshair || pulseActive) {
        lastAngle = ang;
        lastDiving = g_isDiving;
        lastCursor = g_isCursorVisible;
        InvalidateRect(hWnd, NULL, FALSE);
      }
    } else if (wParam == 2) { // 30s Auto-Save Periodic Timer
      SaveSettings();
      if (!g_allProfiles.empty() &&
          g_selectedProfileIdx < (int)g_allProfiles.size()) {
        g_allProfiles[g_selectedProfileIdx].Save(
            GetProfilesPath() + g_allProfiles[g_selectedProfileIdx].name +
            L".json");
      }
    }
    return 0;
  }

  case WM_PAINT:
    DrawOverlay(hWnd, g_logic.GetAngle(), g_detectionRatio, g_showCrosshair);
    return 0;

  case WM_SYSCOMMAND:
    // Block F10 from opening the system menu (interferes with Fn+F10 keybind)
    if ((wParam & 0xFFF0) == SC_KEYMENU)
      return 0;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  SetProcessDPIAware();
  int argc = 1;
  char *argv[] = {(char *)"BetterAngle.exe", nullptr};
  QGuiApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(
      false); // Prevent premature exit if windows are still initializing

  // Phase 0: Kick off version check in background — never blocks startup.
  // g_updateAvailable will be set when done; the control panel UPDATES tab
  // shows it.
  std::thread([]() { CheckForUpdates(); }).detach();

  GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

  LoadSettings();
  CleanupUpdateJunk();

  g_allProfiles = GetProfiles(GetProfilesPath());
  if (g_allProfiles.empty()) {
    Profile p;
    p.name = L"Default";
    p.tolerance = 2;
    p.sensitivityX = 0.1; // Default Standard
    p.sensitivityY = 0.1;
    p.roi_x = 760;
    p.roi_y = 640;
    p.roi_w = 400;
    p.roi_h = 70;
    p.target_color = RGB(150, 150, 150);
    p.crossThickness = 2.0f;
    p.crossColor = RGB(255, 0, 0);
    p.Save(GetProfilesPath() + L"Default.json");

    g_allProfiles.push_back(p);
  }

  // Sensitivity is loaded from the JSON profile; Do not blindly overwrite it
  // here.
  g_currentProfile = g_allProfiles[g_selectedProfileIdx];

  g_selectedProfileIdx = 0;
  bool foundProfile = false;
  for (size_t i = 0; i < g_allProfiles.size(); i++) {
    if (g_allProfiles[i].name == g_lastLoadedProfileName) {
      g_selectedProfileIdx = i;
      foundProfile = true;
      break;
    }
  }

  // Safety: If last profile not found, fall back to what was in settings.json
  // index if that index is valid.
  if (!foundProfile && g_selectedProfileIdx < (int)g_allProfiles.size()) {
    // Keep original g_selectedProfileIdx loaded from settings.json
  } else if (!foundProfile) {
    g_selectedProfileIdx = 0;
  }

  if (g_lastLoadedProfileName.empty() && !g_allProfiles.empty()) {
    g_lastLoadedProfileName = g_allProfiles[0].name;
  }

  g_currentProfile = g_allProfiles[g_selectedProfileIdx];

  // Sync Crosshair Settings from Profile to Global State
  g_crossThickness = g_currentProfile.crossThickness;
  g_crossColor = g_currentProfile.crossColor;
  g_crossOffsetX = g_currentProfile.crossOffsetX;
  g_crossOffsetY = g_currentProfile.crossOffsetY;
  g_crossAngle = g_currentProfile.crossAngle;
  g_crossPulse = g_currentProfile.crossPulse;

  g_logic.LoadProfile(g_currentProfile.sensitivityX);

  // Hotkeys are registered exclusively in HUDWndProc WM_CREATE.
  // NULL-window registration would steal WM_HOTKEY messages before HUD can
  // handle them.

  // Message Window for Raw Input (Bypasses Layered Window UI Bugs)
  WNDCLASS wcMsg = {0};
  wcMsg.lpfnWndProc = MsgWndProc;
  wcMsg.hInstance = hInstance;
  wcMsg.lpszClassName = L"BetterAngleMsgWnd";
  RegisterClass(&wcMsg);
  HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0,
                                HWND_MESSAGE, NULL, hInstance, NULL);
  RegisterRawMouse(hMsgWnd);

  // Phase 2: Create Control Panel (Interactive) via Qt
  g_hPanel = CreateControlPanel(hInstance);

  // Phase 3: Create HUD Window (Transparent Overlay)
  WNDCLASS wc = {0};
  wc.lpfnWndProc = HUDWndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"BetterAngleHUD";
  RegisterClass(&wc);

  int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);

  g_hHUD = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
      L"BetterAngleHUD", L"BetterAngle HUD", WS_POPUP, screenX, screenY,
      screenW, screenH, NULL, NULL, hInstance, NULL);

  AddSystrayIcon(g_hHUD);
  ShowControlPanel(); // Force Dashboard to show on startup
  ShowWindow(g_hHUD, SW_SHOW);
  UpdateWindow(g_hHUD);
  SetTimer(g_hHUD, 1, 16, NULL);    // 60fps (~16ms) Repaint Timer
  SetTimer(g_hHUD, 2, 30000, NULL); // 30s Auto-Save Timer

  std::thread detThread(DetectorThread);

  // Run Qt Event Loop
  int exitCode = app.exec();

  g_running = false;
  if (detThread.joinable())
    detThread.join();

  // Final Save on Exit
  if (!g_allProfiles.empty()) {
    Profile &p = g_allProfiles[g_selectedProfileIdx];
    p.crossPulse = g_crossPulse;
    p.Save(GetProfilesPath() + p.name + L".json");
  }

  SaveSettings();

  RemoveSystrayIcon(g_hHUD);
  GdiplusShutdown(g_gdiplusToken);
  return exitCode;
}
