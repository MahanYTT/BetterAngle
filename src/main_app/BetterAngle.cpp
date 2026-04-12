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

#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#include "shared/State.h"

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
    out.flush();
  }
}

void QtLogHandler(QtMsgType type, const QMessageLogContext &ctx,
                  const QString &msg) {
  std::wstring prefix;
  switch (type) {
  case QtDebugMsg:
    prefix = L"[D] ";
    break;
  case QtInfoMsg:
    prefix = L"[I] ";
    break;
  case QtWarningMsg:
    prefix = L"[W] ";
    break;
  case QtCriticalMsg:
    prefix = L"[C] ";
    break;
  case QtFatalMsg:
    prefix = L"[FATAL] ";
    break;
  }
  WriteLog(prefix + msg.toStdWString());
}

// ─── HUD & Input ────────────────────────────────────────────────────────────
void DetectorThread() {
  while (g_running) {
    if (!g_allProfiles.empty() && g_currentSelection == NONE) {
      Profile &p = g_allProfiles[g_selectedProfileIdx];
      g_logic.LoadProfile(p.sensitivityX);

      bool fortFocused = IsFortniteFocused();
      g_fortniteFocusedCache = fortFocused;

      if (fortFocused || g_currentSelection != NONE) {
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

void CaptureDesktop() {
  int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  int ox = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int oy = GetSystemMetrics(SM_YVIRTUALSCREEN);
  HDC hScreen = GetDC(NULL);
  HDC hDC = CreateCompatibleDC(hScreen);
  if (!g_hBitmap)
    g_hBitmap = CreateCompatibleBitmap(hScreen, sw, sh);
  SelectObject(hDC, g_hBitmap);
  BitBlt(hDC, 0, 0, sw, sh, hScreen, ox, oy, SRCCOPY);
  DeleteDC(hDC);
  ReleaseDC(NULL, hScreen);
}

// ─── Window Procedures ──────────────────────────────────────────────────────
LRESULT CALLBACK HUDWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_TIMER:
    if (wParam == 1) {
      RECT rc;
      GetClientRect(hWnd, &rc);
      InvalidateRect(hWnd, &rc, FALSE);
    } else if (wParam == 2) {
      if (!g_allProfiles.empty()) {
        Profile &p = g_allProfiles[g_selectedProfileIdx];
        p.Save(GetProfilesPath() + p.name + L".json");
      }
    }
    break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    PaintHUD(hWnd, hdc);
    EndPaint(hWnd, &ps);
    break;
  }
  case WM_HOTKEY:
    HandleHotkey((int)wParam);
    break;
  case WM_USER + 1:
    HandleTrayMessage(hWnd, lParam);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_INPUT) {
    HandleRawInput(lParam);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ─── ENTRY POINT ────────────────────────────────────────────────────────────
// Use the standard Qt-recommended entry point: int main(int, char**)
// Do NOT use wWinMain with __argc/__argv — those CRT globals are unreliable
// in Unicode MSVC builds and cause silent crashes before Qt initializes.
int main(int argc, char *argv[]) {
  WriteLog(L"[START] Entering main function");
  // ── Recovery Mode ──────────────────────────────────────────────────────
  if (GetKeyState(VK_SHIFT) & 0x8000) {
    if (MessageBoxW(NULL,
                    L"BetterAngle Recovery Mode\n\nHolding SHIFT resets all "
                    L"settings to defaults. Proceed?",
                    L"BetterAngle Recovery",
                    MB_YESNO | MB_ICONQUESTION) == IDYES) {
      std::filesystem::remove_all(GetAppRootPath());
      MessageBoxW(NULL,
                  L"Settings reset. BetterAngle will start in setup mode.",
                  L"Done", MB_OK | MB_ICONINFORMATION);
    }
  }

  // ── Qt Application ─────────────────────────────────────────────────────
  // Install log handler BEFORE creating QGuiApplication
  qInstallMessageHandler(QtLogHandler);
  WriteLog(L"[QT] Log handler installed");
  QGuiApplication app(argc, argv);
  WriteLog(L"[QT] QGuiApplication created");
  app.setQuitOnLastWindowClosed(false);

  qInfo() << "BetterAngle starting, version" << VERSION_STR;

  // ── GDI+ ───────────────────────────────────────────────────────────────
  GdiplusStartupInput gsi;
  GdiplusStartup(&g_gdiplusToken, &gsi, NULL);
  WriteLog(L"[GDI+] GDI+ initialized");

  // ── Register HUD window class ───────────────────────────────────────────
  HINSTANCE hInstance = GetModuleHandle(NULL);
  WNDCLASS wc = {};
  wc.lpfnWndProc = HUDWndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"BetterAngleHUD";
  RegisterClass(&wc);
  WriteLog(L"[WIN32] HUD window class registered");

  // ── Load settings synchronously (fast, just reads files) ───────────────
  LoadSettings();
  WriteLog(L"[SETTINGS] Settings loaded");
  CleanupUpdateJunk();
  WriteLog(L"[SETTINGS] Update junk cleaned");
  g_allProfiles = GetProfiles(GetProfilesPath());
  WriteLog(L"[SETTINGS] Profiles loaded");

  if (g_allProfiles.empty() || g_needsSetup) {
    g_needsSetup = true;
    g_allProfiles.clear();
    Profile def;
    def.name = L"Default";
    def.sensitivityX = 0.05;
    def.sensitivityY = 0.05;
    def.showCrosshair = true;
    def.crossThickness = 2.0f;
    def.crossColor = RGB(0, 255, 204);
    def.tolerance = 2;
    g_allProfiles.push_back(def);
    g_selectedProfileIdx = 0;
  }

  g_currentProfile = g_allProfiles[g_selectedProfileIdx];
  g_crossThickness = g_currentProfile.crossThickness;
  g_crossColor = g_currentProfile.crossColor;
  g_crossOffsetX = g_currentProfile.crossOffsetX;
  g_crossOffsetY = g_currentProfile.crossOffsetY;
  g_crossAngle = g_currentProfile.crossAngle;
  g_crossPulse = g_currentProfile.crossPulse;
  g_logic.LoadProfile(g_currentProfile.sensitivityX);

  // ── Create Win32 HUD overlay window ────────────────────────────────────
  WNDCLASS wcMsg = {};
  wcMsg.lpfnWndProc = MsgWndProc;
  wcMsg.hInstance = hInstance;
  wcMsg.lpszClassName = L"BetterAngleMsgWnd";
  RegisterClass(&wcMsg);
  HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleMsgWnd", NULL, 0, 0, 0, 0, 0,
                                HWND_MESSAGE, NULL, hInstance, NULL);
  RegisterRawMouse(hMsgWnd);

  g_virtScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
  g_virtScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  g_hHUD = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
      L"BetterAngleHUD", L"BetterAngle HUD", WS_POPUP, g_virtScreenX,
      g_virtScreenY, screenW, screenH, NULL, NULL, hInstance, NULL);

  AddSystrayIcon(g_hHUD);
  SetTimer(g_hHUD, 1, 16, NULL);    // Repaint (60fps)
  SetTimer(g_hHUD, 2, 30000, NULL); // Auto-save

  // ── Background threads ─────────────────────────────────────────────────
  std::thread(CheckForUpdates).detach();
  std::thread(DetectorThread).detach();

  // ── QML Engine ─────────────────────────────────────────────────────────
  // Create backend and engine, register context, then load QML.
  // This is the ONLY correct order — context properties must be set
  // before load() is called.
  BetterAngleBackend backend;
  g_backend = &backend;
  WriteLog(L"[QML] Backend created");

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("backend", &backend);
  WriteLog(L"[QML] Context property set");

  // Load the single root QML file. Splash is handled inside main.qml
  // via a QML Timer/StackLayout — no C++ timer coordination needed.
  WriteLog(L"[QML] Loading main.qml from resource...");
  engine.load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));
  WriteLog(L"[QML] engine.load completed");

  if (engine.rootObjects().isEmpty()) {
    qCritical() << "FATAL: main.qml failed to load. Check debug.log.";
    MessageBoxW(NULL,
                L"BetterAngle failed to load its UI.\n\nCheck: "
                L"%LOCALAPPDATA%\\BetterAngle\\debug.log",
                L"BetterAngle — Fatal Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  qInfo() << "QML engine loaded successfully. Entering event loop.";
  return app.exec();
}
