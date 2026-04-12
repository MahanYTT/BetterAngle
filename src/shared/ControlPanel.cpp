#include "shared/ControlPanel.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QDebug>
#include "shared/BetterAngleBackend.h"

QQmlApplicationEngine* g_qmlEngine = nullptr;
BetterAngleBackend* g_backend = nullptr;
std::wstring g_qmlErrors;

void EnsureEngineInitialized() {
    if (!g_qmlEngine) {
        g_qmlEngine = new QQmlApplicationEngine();
        if (!g_qmlEngine) {
            MessageBoxW(NULL, L"FATAL: Failed to allocate QQmlApplicationEngine.\nQt initialization has failed.", L"BetterAngle Engine Error", MB_OK | MB_ICONERROR);
            exit(1);
        }

        // Trap QML warnings/errors for diagnostics
        QObject::connect(g_qmlEngine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &errors) {
            for (const auto& err : errors) {
                std::string s = err.toString().toStdString();
                g_qmlErrors += std::wstring(s.begin(), s.end()) + L"\n";
            }
        });

        g_backend = new BetterAngleBackend(g_qmlEngine);
        if (!g_backend) {
            MessageBoxW(NULL, L"FATAL: Failed to create BetterAngleBackend.\nLogic initialization failed.", L"BetterAngle Engine Error", MB_OK | MB_ICONERROR);
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

        qDebug() << "[BOOT] Attempting to load main.qml...";
        g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));

        // Splash is index 0, so Dashboard should be index 1+
        if (g_qmlEngine->rootObjects().isEmpty() || g_qmlEngine->rootObjects().size() < 2) {
            std::wstring err = L"CRITICAL: BetterAngle could not load the Main UI (main.qml).\n\n";
            err += L"Error Details:\n";
            if (!g_qmlErrors.empty()) {
                err += g_qmlErrors;
            } else {
                err += L"- No root objects found.\n";
                err += L"- Resource URL: qrc:/src/gui/main.qml\n";
            }
            err += L"\nThis usually happens if the binary is corrupted or resources are blocked.";

            MessageBoxW(NULL, err.c_str(), L"BetterAngle Error", MB_OK | MB_ICONERROR);
            exit(1);
        }
        qDebug() << "[BOOT] Dashboard UI loaded successfully.";
    }

    return (HWND)1;
}

void ShowSplashScreen() {
    EnsureEngineInitialized();
    
    // Reset errors for this load
    g_qmlErrors.clear();

    g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/Splash.qml")));
    if (g_qmlEngine->rootObjects().isEmpty()) {
        qDebug() << "[ERROR] Splash.qml failed to load.";
        
        std::wstring err = L"CRITICAL ERROR: Splash resources failed to load.\n\n";
        err += L"Engine Diagnostics:\n";
        if (!g_qmlErrors.empty()) {
            err += g_qmlErrors;
        } else {
            err += L"- No objects reached the engine.\n";
            err += L"- URL: qrc:/src/gui/Splash.qml\n";
        }
        err += L"\nThe application cannot continue without the startup UI components.";

        MessageBoxW(NULL, err.c_str(), L"BetterAngle Boot Error", MB_OK | MB_ICONERROR);
        exit(1);
    } else {
        qDebug() << "[BOOT] Splash.qml loaded successfully.";
    }
}

void ShowControlPanel() {
    if (g_backend) {
        g_backend->requestToggleControlPanel();
    }
}