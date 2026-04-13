#include "shared/ControlPanel.h"
#include "shared/BetterAngleBackend.h"
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

QQmlApplicationEngine *g_qmlEngine = nullptr;
BetterAngleBackend *g_backend = nullptr;
QObject *g_splashRoot = nullptr;
std::wstring g_qmlErrors;

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
  }

  return (HWND)1;
}

void ShowSplashScreen() {
  EnsureEngineInitialized();

  // Reset errors for this load
  g_qmlErrors.clear();

  const qsizetype rootCountBefore = g_qmlEngine->rootObjects().size();
  LogStartup("Splash: Attempting to load Splash.qml.");
  g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/Splash.qml")));
  const qsizetype rootCountAfter = g_qmlEngine->rootObjects().size();
  LogStartup("Splash: Splash.qml load complete. Root object count " +
             std::to_string((int)rootCountBefore) + " -> " +
             std::to_string((int)rootCountAfter) + ".");
  if (rootCountAfter <= rootCountBefore) {
    LogStartup("SPLASH_FATAL: Splash.qml failed to create a new root object.");
    qDebug() << "[ERROR] Splash.qml failed to load.";

    std::wstring err = L"CRITICAL ERROR: Splash resources failed to load.\n\n";
    err += L"Engine Diagnostics:\n";
    if (!g_qmlErrors.empty()) {
      err += g_qmlErrors;
    } else {
      err += L"- No new root object was created for qrc:/src/gui/Splash.qml\n";
      err += L"- Root count before load: " +
             std::to_wstring((int)rootCountBefore) + L"\n";
      err += L"- Root count after load: " +
             std::to_wstring((int)rootCountAfter) + L"\n";
    }
    err +=
        L"\nThe application cannot continue without the startup UI components.";

    MessageBoxW(NULL, err.c_str(), L"BetterAngle Boot Error",
                MB_OK | MB_ICONERROR);
    exit(1);
  } else {
    g_splashRoot = g_qmlEngine->rootObjects().constLast();
    QObject::connect(g_splashRoot, &QObject::destroyed, [](QObject *) {
      LogStartup(
          "Splash: Splash root destroyed. Clearing tracked splash pointer.");
      g_splashRoot = nullptr;
    });
    LogStartup("Splash: Splash.qml loaded successfully and root tracked for "
               "direct close.");
  }
}

void CloseSplashScreenDirect() {
  if (!g_splashRoot) {
    LogStartup("Splash: No tracked splash root available for direct close.");
    return;
  }

  LogStartup("Splash: Direct C++ close requested for tracked splash root.");
  if (auto *window = qobject_cast<QWindow *>(g_splashRoot)) {
    window->close();
    LogStartup("Splash: Tracked splash QWindow close() invoked.");
  } else {
    const bool invoked =
        QMetaObject::invokeMethod(g_splashRoot, "close", Qt::DirectConnection);
    LogStartup(std::string("Splash: Fallback QObject close() invoke ") +
               (invoked ? "succeeded." : "failed."));
  }
}

void ShowControlPanel() {
  if (g_backend) {
    g_backend->requestToggleControlPanel();
  }
}