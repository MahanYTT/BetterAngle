#include "shared/ControlPanel.h"
#include "shared/BetterAngleBackend.h"
#include "shared/State.h"
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>
#include <cstdint>

QQmlApplicationEngine *g_qmlEngine = nullptr;
BetterAngleBackend *g_backend = nullptr;
std::wstring g_qmlErrors;

namespace {
void SyncHUDWithPanelWindow(QWindow *panelWindow) {
  if (!panelWindow)
    return;

  const bool panelInteractive = panelWindow->isVisible() &&
                                panelWindow->visibility() != QWindow::Minimized;

  LogStartup(std::string("PanelWindowState: visible=") +
             (panelWindow->isVisible() ? "true" : "false") +
             " visibility=" + std::to_string(int(panelWindow->visibility())) +
             " active=" + (panelWindow->isActive() ? "true" : "false") +
             " interactive=" + (panelInteractive ? "true" : "false"));

  if (!g_hHUD)
    return;

  LONG_PTR hudExStyle = GetWindowLongPtr(g_hHUD, GWL_EXSTYLE);
  const bool hudShouldBeClickable =
      !panelInteractive && (g_isDraggingHUD || g_currentSelection != NONE);

  if (panelInteractive || !hudShouldBeClickable)
    hudExStyle |= WS_EX_TRANSPARENT;
  else
    hudExStyle &= ~WS_EX_TRANSPARENT;

  SetWindowLongPtr(g_hHUD, GWL_EXSTYLE, hudExStyle);
  SetWindowPos(g_hHUD, panelInteractive ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0,
               0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE |
                   SWP_SHOWWINDOW);
  UpdateWindow(g_hHUD);
  InvalidateRect(g_hHUD, NULL, FALSE);

  if (g_hPanel) {
    EnableWindow(g_hPanel, TRUE);
    SetWindowPos(g_hPanel, panelInteractive ? HWND_TOPMOST : HWND_NOTOPMOST, 0,
                 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    if (panelInteractive) {
      BringWindowToTop(g_hPanel);
      SetForegroundWindow(g_hPanel);
      SetActiveWindow(g_hPanel);
      SetFocus(g_hPanel);
    }
  }

  LogStartup(
      std::string("PanelWindowState: HUD synchronized exStyleTransparent=") +
      ((hudExStyle & WS_EX_TRANSPARENT) ? "true" : "false") +
      " zOrder=" + (panelInteractive ? "NOTOPMOST" : "TOPMOST") +
      " clickable=" + (hudShouldBeClickable ? "true" : "false") +
      " panelForcedTop=" + (panelInteractive ? "true" : "false"));
}
} // namespace

void EnsureEngineInitialized() {
  if (!g_qmlEngine) {
    LogStartup("Engine: Initializing QQmlApplicationEngine...");
    g_qmlEngine = new QQmlApplicationEngine();
    if (!g_qmlEngine) {
      MessageBoxW(NULL,
                  L"FATAL: Failed to allocate QQmlApplicationEngine.\nQt "
                  L"initialization has failed.",
                  L"BetterAngle Engine Error", MB_OK | MB_ICONERROR);
      exit(1);
    }

    // Trap QML warnings/errors for diagnostics
    QObject::connect(g_qmlEngine, &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError> &errors) {
                       for (const auto &err : errors) {
                         std::string s = err.toString().toStdString();
                         LogStartup("QML_WARNING: " + s);
                         g_qmlErrors +=
                             std::wstring(s.begin(), s.end()) + L"\n";
                       }
                     });

    g_backend = new BetterAngleBackend(g_qmlEngine);
    if (!g_backend) {
      MessageBoxW(NULL,
                  L"FATAL: Failed to create BetterAngleBackend.\nLogic "
                  L"initialization failed.",
                  L"BetterAngle Engine Error", MB_OK | MB_ICONERROR);
      exit(1);
    }

    // Register "backend" context property BEFORE any load() call.
    g_qmlEngine->rootContext()->setContextProperty("backend", g_backend);
  }
}

HWND CreateControlPanel(HINSTANCE hInstance) {
  EnsureEngineInitialized();

  static bool mainLoaded = false;
  if (!mainLoaded) {
    mainLoaded = true;

    // Reset errors for this load
    g_qmlErrors.clear();

    const qsizetype rootCountBefore = g_qmlEngine->rootObjects().size();
    LogStartup("Panel: Attempting to load main.qml.");
    g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));
    const qsizetype rootCountAfter = g_qmlEngine->rootObjects().size();
    LogStartup("Panel: main.qml load complete. Root object count " +
               std::to_string((int)rootCountBefore) + " -> " +
               std::to_string((int)rootCountAfter) + ".");

    if (rootCountAfter <= rootCountBefore) {
      LogStartup("PANEL_FATAL: main.qml failed to create a new root object.");
      std::wstring err =
          L"CRITICAL ERROR: BetterAngle Main UI failed to load.\n\n";
      err += L"Diagnostics:\n";
      if (!g_qmlErrors.empty()) {
        err += g_qmlErrors;
      } else {
        err += L"- No new root object was created for qrc:/src/gui/main.qml\n";
        err += L"- Root count before load: " +
               std::to_wstring((int)rootCountBefore) + L"\n";
        err += L"- Root count after load: " +
               std::to_wstring((int)rootCountAfter) + L"\n";
      }
      err += L"\nThis failure is usually due to missing graphics drivers or "
             L"corrupted installation.";

      MessageBoxW(NULL, err.c_str(), L"BetterAngle Engine Failure",
                  MB_OK | MB_ICONERROR);
      exit(1);
    }
    LogStartup("Panel: Dashboard UI loaded successfully.");

    QObject *rootObject = g_qmlEngine->rootObjects().isEmpty()
                              ? nullptr
                              : g_qmlEngine->rootObjects().last();
    QWindow *panelWindow = qobject_cast<QWindow *>(rootObject);
    if (panelWindow) {
      g_hPanel = reinterpret_cast<HWND>(panelWindow->winId());
      LogStartup(std::string("Panel: Native panel handle acquired: ") +
                 std::to_string(reinterpret_cast<uintptr_t>(g_hPanel)));

      QObject::connect(panelWindow, &QWindow::visibleChanged, [panelWindow]() {
        SyncHUDWithPanelWindow(panelWindow);
      });
      QObject::connect(panelWindow, &QWindow::visibilityChanged,
                       [panelWindow](QWindow::Visibility) {
                         SyncHUDWithPanelWindow(panelWindow);
                       });
      QObject::connect(panelWindow, &QWindow::activeChanged, [panelWindow]() {
        SyncHUDWithPanelWindow(panelWindow);
      });

      SyncHUDWithPanelWindow(panelWindow);
    } else {
      LogStartup("Panel: Failed to cast root object to QWindow; HUD/panel sync "
                 "unavailable.");
    }
  }

  return g_hPanel ? g_hPanel : (HWND)1;
}

void ShowControlPanel() {
  if (g_backend) {
    g_backend->requestToggleControlPanel();
  }
}