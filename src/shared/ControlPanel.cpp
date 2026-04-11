#include "shared/ControlPanel.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include "shared/BetterAngleBackend.h"

QQmlApplicationEngine* g_qmlEngine = nullptr;
BetterAngleBackend* g_backend = nullptr;

HWND CreateControlPanel(HINSTANCE hInstance) {
    if (!g_qmlEngine) {
        g_qmlEngine = new QQmlApplicationEngine();
        
        // Register backend
        g_backend = new BetterAngleBackend(g_qmlEngine);
        g_qmlEngine->rootContext()->setContextProperty("backend", g_backend);
        
        g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));
        if (g_qmlEngine->rootObjects().isEmpty()) {
            MessageBoxW(NULL, L"Failed to load user interface (main.qml). Please check installation.", L"BetterAngle Error", MB_OK | MB_ICONERROR);
        }
    }
    
    // Qt manages its own windows, we return a dummy HWND to satisfy the existing architecture 
    // or we can extract the HWND from the QQuickWindow if absolutely necessary for the WinMain loop.
    return (HWND)1; 
}

void ShowControlPanel() {
    if (g_backend) {
        g_backend->requestShowControlPanel();
    }
}