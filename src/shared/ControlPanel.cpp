#include "shared/ControlPanel.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include "shared/BetterAngleBackend.h"

QQmlApplicationEngine* g_qmlEngine = nullptr;

HWND CreateControlPanel(HINSTANCE hInstance) {
    if (!g_qmlEngine) {
        g_qmlEngine = new QQmlApplicationEngine();
        
        // Register backend
        BetterAngleBackend* backend = new BetterAngleBackend(g_qmlEngine);
        g_qmlEngine->rootContext()->setContextProperty("backend", backend);
        
        g_qmlEngine->load(QUrl(QStringLiteral("qrc:/src/gui/main.qml")));
    }
    
    // Qt manages its own windows, we return a dummy HWND to satisfy the existing architecture 
    // or we can extract the HWND from the QQuickWindow if absolutely necessary for the WinMain loop.
    return (HWND)1; 
}