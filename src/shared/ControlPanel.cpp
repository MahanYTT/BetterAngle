#include "shared/ControlPanel.h"
#include "shared/BetterAngleBackend.h"
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

QQmlApplicationEngine *g_qmlEngine = nullptr;
BetterAngleBackend *g_backend = nullptr;

void EnsureEngineInitialized() {
  if (!g_qmlEngine) {
    g_qmlEngine = new QQmlApplicationEngine();
    g_backend = new BetterAngleBackend(g_qmlEngine);

    // Register "backend" context property BEFORE any load() call.
    g_qmlEngine->rootContext()->setContextProperty("backend", g_backend);
  }
}

HWND CreateControlPanel(HINSTANCE hInstance) {
  EnsureEngineInitialized();

  // Use a static flag to prevent loading main.qml more than once.
  static bool mainLoaded = false;
  if (!mainLoaded) {
    mainLoaded = true;

    // Load main.qml. Dashboard.qml and FirstTimeSetup.qml are in the
    // same qrc:/src/gui/ directory and Qt auto-discovers them as types —
    // no import statement or qmldir needed.
    g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));

    if (g_qmlEngine->rootObjects().isEmpty()) {
      qDebug() << "[ERROR] main.qml failed to load. Root objects:"
               << g_qmlEngine->rootObjects().size();
      MessageBoxW(NULL,
                  L"CRITICAL: Failed to load the dashboard UI (main.qml).\n\n"
                  L"No root QML window was created.",
                  L"BetterAngle UI Error", MB_OK | MB_ICONERROR);
      exit(1);
    }
    qDebug() << "[BOOT] main.qml loaded successfully. Root objects:"
             << g_qmlEngine->rootObjects().size();
  }

  // Find the actual HWND of the Qt window to provide a valid handle for logging/management
  // The title must match what is set in main.qml
  HWND hPanel = FindWindowW(NULL, L"BetterAngle Pro Angle HUD");
  if (hPanel) {
    return hPanel;
  }

  return (HWND)1;
}



void ShowControlPanel() {
  if (g_backend) {
    g_backend->requestShowControlPanel();
  }
}