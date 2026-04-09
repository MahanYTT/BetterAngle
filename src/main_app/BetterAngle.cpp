#include <atomic>
#include <dwmapi.h>
#include <fstream>
#include <gdiplus.h>
#include <iostream>
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
#include "shared/Remote.h"
#include "shared/Startup.h"
#include "shared/Tray.h"
#include "shared/Updater.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "shared/State.h"

// Global State
HWND g_hHUD = NULL;
HWND g_hPanel = NULL;
ULONG_PTR g_gdiplusToken = 0;
std::atomic<bool> g_running(true);
FovDetector g_detector;

Profile g_currentProfile;
std::vector<Profile> g_allProfiles;
int g_selectedProfileIdx = 0;

// FOV Detector Thread
void DetectorThread() {
  while (g_running) {
    {
      std::lock_guard<std::mutex> lock(g_stateMutex);
      if (!g_allProfiles.empty() && g_currentSelection == NONE) {
        Profile &p = g_allProfiles[g_selectedProfileIdx];
        RoiConfig cfg = {p.roi_x, p.roi_y,        p.roi_w,
                         p.roi_h, p.target_color, p.tolerance};

        g_detectionRatio = g_detector.Scan(cfg);
        if (g_detectionRatio > 0.05f) {
          g_logic.SetScale(p.scale_diving);
        } else {
          g_logic.SetScale(p.scale_normal);
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

// Screen Snapshot for Flicker-Free Selection (v4.9.15)
void CaptureDesktop() {
  std::lock_guard<std::mutex> lock(g_stateMutex);
  int sw = GetSystemMetrics(SM_CXSCREEN);
  int sh = GetSystemMetrics(SM_CYSCREEN);
  HDC hdcScreen = GetDC(NULL);
  if (!hdcScreen)
    return;

  HDC hdcMem = CreateCompatibleDC(hdcScreen);
  if (!hdcMem) {
    ReleaseDC(NULL, hdcScreen);
    return;
  }

  if (g_screenSnapshot)
    DeleteObject(g_screenSnapshot);
  g_screenSnapshot = CreateCompatibleBitmap(hdcScreen, sw, sh);
  if (!g_screenSnapshot) {
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return;
  }

  HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);
  BitBlt(hdcMem, 0, 0, sw, sh, hdcScreen, 0, 0, SRCCOPY);
  SelectObject(hdcMem, hOld);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdcScreen);
}

// HUD Window Procedure
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
  switch (message) {
  case WM_CREATE:
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    RegisterRawMouse(hWnd);
    RegisterHotKey(hWnd, 1, MOD_CONTROL, 'U'); // Toggle Panel
    RegisterHotKey(hWnd, 2, MOD_CONTROL, 'R'); // ROI Select
    RegisterHotKey(hWnd, 3, 0, VK_F10);        // Crosshair
    RegisterHotKey(hWnd, 4, MOD_CONTROL, 'G'); // Zero Angle
    RegisterHotKey(hWnd, 5, MOD_CONTROL, '9'); // Secret Debug
    return 0;

  case WM_HOTKEY: {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    switch (wParam) {
    case 1: // Toggle Panel
      if (IsIconic(g_hPanel))
        ShowWindow(g_hPanel, SW_RESTORE);
      else if (IsWindowVisible(g_hPanel))
        ShowWindow(g_hPanel, SW_MINIMIZE);
      else
        ShowWindow(g_hPanel, SW_SHOW);
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
        SetWindowLong(hWnd, GWL_EXSTYLE,
                      GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
      }
      break;
    case 3:
      g_showCrosshair = !g_showCrosshair;
      break;
    case 4:
      g_currentAngle = 0.0f;
      break;
    case 5:
      g_debugMode = !g_debugMode;
      break;
    }
  }
    return 0;
  case WM_LBUTTONDOWN: {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (g_currentSelection == SELECTING_ROI) {
      POINT cur;
      GetCursorPos(&cur);
      g_startPoint = cur;
      g_selectionRect = {cur.x, cur.y, cur.x, cur.y};
    } else if (g_currentSelection == SELECTING_COLOR) {
      // STAGE 2: PRECISION COLOR PICK (Snap-Shot Bypass)
      if (g_screenSnapshot) {
        HDC hdcScreen = GetDC(NULL);
        if (!hdcScreen)
          return 0;

        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        if (!hdcMem) {
          ReleaseDC(NULL, hdcScreen);
          return 0;
        }

        HGDIOBJ hOld = SelectObject(hdcMem, g_screenSnapshot);
        if (!hOld) {
          DeleteDC(hdcMem);
          ReleaseDC(NULL, hdcScreen);
          return 0;
        }

        POINT cur;
        GetCursorPos(&cur);
        COLORREF pixel = GetPixel(hdcMem, cur.x, cur.y);
        // Sync: GDI returns 0x00BBGGRR
        g_targetColor = pixel;
        g_pickedColor = pixel;

        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
      }

      // Finalize and Exit Selection
      g_currentSelection = NONE;
      g_isSelectionActive = false;
      SetWindowLong(hWnd, GWL_EXSTYLE,
                    GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

      if (!g_allProfiles.empty()) {
        Profile &p = g_allProfiles[g_selectedProfileIdx];
        p.target_color = g_targetColor;
        p.roi_x = min(g_selectionRect.left, g_selectionRect.right);
        p.roi_y = min(g_selectionRect.top, g_selectionRect.bottom);
        p.roi_w = abs(g_selectionRect.right - g_selectionRect.left);
        p.roi_h = abs(g_selectionRect.bottom - g_selectionRect.top);

        // Save to the actual profile path
        std::wstring profilePath = L"profiles/" + p.name + L".json";
        p.Save(profilePath);

        // Also maintain the legacy 'last_calibrated' for quick-load logic if
        // needed
        p.Save(L"profiles/last_calibrated.json");
      }
    }
  }
    return 0;

  case WM_MOUSEMOVE: {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (g_currentSelection != NONE) {
      if (g_currentSelection == SELECTING_ROI && (wParam & MK_LBUTTON)) {
        POINT cur;
        GetCursorPos(&cur);
        g_selectionRect.right = cur.x;
        g_selectionRect.bottom = cur.y;
      }
      InvalidateRect(hWnd, NULL, FALSE);
    }
  }
    return 0;

  case WM_LBUTTONUP: {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (g_currentSelection == SELECTING_ROI) {
      g_currentSelection = SELECTING_COLOR;
      InvalidateRect(hWnd, NULL, FALSE);
    }
  }
    return 0;

  case WM_INPUT: {
    // THE FIX: Send the mouse delta to the logic engine
    int dx = GetRawInputDeltaX(lParam);
    g_logic.Update(dx);
    return 0;
  }

  case WM_TIMER: {
    CURSORINFO ci = {sizeof(CURSORINFO)};
    if (GetCursorInfo(&ci)) {
      std::lock_guard<std::mutex> lock(g_stateMutex);
      g_isCursorVisible = (ci.flags & CURSOR_SHOWING);
    }
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
  }

  case WM_PAINT: {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    DrawOverlay(hWnd, g_logic.GetAngle(), g_status.c_str(), g_detectionRatio,
                g_showCrosshair);
  }
    return 0;

  case WM_DESTROY:
    g_running = false;
    PostQuitMessage(0);
    return 0;

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  // Phase 0: Silent Update & Relaunch Sequence
  try {
    if (CheckForUpdates()) {
      std::wstring verStr = std::wstring(g_latestVersionOnline.begin(),
                                         g_latestVersionOnline.end());
      // User requested: Download the latest .exe from GitHub to update_tmp.exe
      std::wstring downloadUrl = L"https://github.com/MahanYTT/BetterAngle/"
                                 L"releases/latest/download/BetterAngle.exe";
      if (DownloadUpdate(downloadUrl, L"update_tmp.exe")) {
        ApplyUpdateAndRestart();
      }
    }
  } catch (const std::exception &e) {
    MessageBox(NULL, L"Failed to check for updates", L"Error", MB_OK);
  }

  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  g_gdiplusToken = gdiplusToken;

  // Phase 1: Startup Sequence (Splash)
  try {
    ShowSplashLoader(hInstance);
  } catch (const std::exception &e) {
    MessageBox(NULL, L"Failed to show splash loader", L"Error", MB_OK);
  }

  // Initial Load (Syncing was handled or represented in Splash)
  {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    try {
      g_allProfiles = GetProfiles(L"profiles");
      if (!g_allProfiles.empty()) {
        g_currentProfile = g_allProfiles[0];
        g_logic.SetScale(g_currentProfile.scale_normal);
      }
    } catch (const std::exception &e) {
      MessageBox(NULL, L"Failed to load profiles", L"Error", MB_OK);
      return 1;
    }
  }

  // Phase 2: Create Control Panel (Interactive)
  try {
    g_hPanel = CreateControlPanel(hInstance);
    if (!g_hPanel) {
      MessageBox(NULL, L"Failed to create Control Panel", L"Error", MB_OK);
      return 1;
    }
  } catch (const std::exception &e) {
    MessageBox(NULL, L"Failed to create Control Panel", L"Error", MB_OK);
    return 1;
  }

  // Phase 3: Create HUD Window (Transparent Overlay)
  try {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = HUDWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleHUD";
    if (!RegisterClass(&wc)) {
      MessageBox(NULL, L"Failed to register HUD window class", L"Error", MB_OK);
      return 1;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hHUD = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                WS_EX_TOOLWINDOW,
                            L"BetterAngleHUD", L"BetterAngle HUD", WS_POPUP, 0,
                            0, screenW, screenH, NULL, NULL, hInstance, NULL);
    if (!g_hHUD) {
      MessageBox(NULL, L"Failed to create HUD window", L"Error", MB_OK);
      return 1;
    }

    RegisterRawMouse(g_hHUD);
    ShowWindow(g_hHUD, SW_SHOW);
    UpdateWindow(g_hHUD);
    SetTimer(g_hHUD, 1, 25, NULL);
  } catch (const std::exception &e) {
    MessageBox(NULL, L"Failed to create HUD window", L"Error", MB_OK);
    return 1;
  }

  std::thread detThread;
  try {
    detThread = std::thread(DetectorThread);
  } catch (const std::exception &e) {
    MessageBox(NULL, L"Failed to start detector thread", L"Error", MB_OK);
    if (g_hHUD) {
      DestroyWindow(g_hHUD);
    }
    if (g_hPanel) {
      DestroyWindow(g_hPanel);
    }
    return 1;
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    try {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      // If Panel is closed by user (WM_QUIT from Panel should not happen but
      // WM_DESTROY does)
      if (!IsWindow(g_hPanel) && !IsWindow(g_hHUD))
        break;
    } catch (const std::exception &e) {
      MessageBox(NULL, L"Error in message loop", L"Error", MB_OK);
      break;
    }
  }

  g_running = false;
  if (detThread.joinable())
    detThread.join();

  if (g_hHUD) {
    DestroyWindow(g_hHUD);
    g_hHUD = NULL;
  }
  if (g_hPanel) {
    DestroyWindow(g_hPanel);
    g_hPanel = NULL;
  }

  if (g_gdiplusToken) {
    GdiplusShutdown(g_gdiplusToken);
    g_gdiplusToken = 0;
  }

  {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (g_screenSnapshot) {
      DeleteObject(g_screenSnapshot);
      g_screenSnapshot = NULL;
    }
    g_allProfiles.clear();
    g_selectedProfileIdx = 0;
    g_currentProfile = Profile();
    g_currentSelection = NONE;
    g_isSelectionActive = false;
    g_isDiving = false;
    g_showROIBox = true;
    g_currentTab = 0;
    g_detectionRatio = 0.0f;
    g_isCheckingForUpdates = false;
    g_updateSpinAngle = 0.0f;
    g_updateAvailable = false;
    g_showCrosshair = false;
    g_pickedColor = RGB(255, 255, 255);
    g_targetColor = RGB(255, 255, 255);
    g_latestVersion = 4.920f;
    g_latestName = L"Pending Scan";
    g_selectionRect = {0, 0, 0, 0};
    g_startPoint = {0};
    g_status = "Connected (v" VERSION_STR " Pro)";
    g_latestVersionOnline = "v" VERSION_STR;
    g_currentAngle = 0.0f;
    g_debugMode = false;
    g_isCursorVisible = false;
    g_logic.SetScale(0.0);
    g_logic.SetZero();
  }

  return (int)msg.wParam;
}
