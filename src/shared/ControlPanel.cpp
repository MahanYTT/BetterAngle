#include "shared/ControlPanel.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QDebug>
#include "shared/BetterAngleBackend.h"

QQmlApplicationEngine* g_qmlEngine = nullptr;
BetterAngleBackend* g_backend = nullptr;

void EnsureEngineInitialized() {
    if (!g_qmlEngine) {
        g_qmlEngine = new QQmlApplicationEngine();
        if (!g_qmlEngine) {
            MessageBoxW(NULL, L"FATAL: Failed to allocate QQmlApplicationEngine.\nQt initialization has failed.", L"BetterAngle Engine Error", MB_OK | MB_ICONERROR);
            exit(1);
        }

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

        // Load main.qml. Dashboard.qml and FirstTimeSetup.qml are in the
        // same qrc:/src/gui/ directory and Qt auto-discovers them as types.
        qDebug() << "[BOOT] Attempting to load main.qml...";
        g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));

        // Splash is index 0, so Dashboard should be index 1+
        if (g_qmlEngine->rootObjects().isEmpty() || g_qmlEngine->rootObjects().size() < 2) {
            std::wstring err = L"CRITICAL: BetterAngle could not load the Main UI (main.qml).\n\n";
            err += L"Error Details:\n";
            if (g_qmlEngine->rootObjects().isEmpty()) err += L"- No root objects found.\n";
            err += L"- Resource URL: qrc:/src/gui/main.qml\n\n";
            err += L"This usually happens if the binary is corrupted or resources are blocked.";

            MessageBoxW(NULL, err.c_str(), L"BetterAngle Error", MB_OK | MB_ICONERROR);
            exit(1);
        }
        qDebug() << "[BOOT] Dashboard UI loaded successfully.";
    }

    return (HWND)1;
}

void ShowSplashScreen() {
    EnsureEngineInitialized();
    g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/Splash.qml")));
    if (g_qmlEngine->rootObjects().isEmpty()) {
        qDebug() << "[ERROR] Splash.qml failed to load.";
        MessageBoxW(NULL, 
            L"CRITICAL ERROR: Splash resources failed to load.\n\nThe application cannot continue without the startup UI components.", 
            L"BetterAngle Boot Error", MB_OK | MB_ICONERROR);
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