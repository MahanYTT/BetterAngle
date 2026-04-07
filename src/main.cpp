#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <dwmapi.h>
#include <gdiplus.h>

#include "include/Input.h"
#include "include/Overlay.h"
#include "include/Logic.h"
#include "include/Detector.h"
#include "include/Updater.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Global State
HWND g_hWnd = NULL;
ULONG_PTR g_gdiplusToken;
std::atomic<bool> g_running(true);
AngleLogic g_logic(800, 6.5);
FovDetector g_detector;
std::string g_status = "Initializing...";

// FOV Detector Thread
void DetectorThread() {
    RoiConfig cfg = { 960-150, 800, 300, 60, RGB(255, 255, 255), 20 }; // Example ROI
    while (g_running) {
        float ratio = g_detector.Scan(cfg);
        if (ratio > 0.05f) {
            g_status = "Dive/Glide Detected (" + std::to_string((int)(ratio * 100)) + "%)";
            g_logic.SetScale(0.005); // Example: faster scale in freefall
        } else {
            g_status = "Normal Tracking";
            g_logic.SetScale(0.003); // Example: normal scale
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            RegisterRawMouse(hWnd);
            return 0;
        case WM_INPUT: {
            int dx = GetRawInputDeltaX(lParam);
            g_logic.Update(dx);
            return 0;
        }
        case WM_TIMER:
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        case WM_PAINT:
            DrawOverlay(hWnd, g_logic.GetAngle(), g_status.c_str());
            return 0;
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleOverlay";
    RegisterClass(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"BetterAngleOverlay", L"BetterAngle",
        WS_POPUP,
        0, 0, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 0;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    // Set a timer for overlay refresh (40 FPS => 25ms)
    SetTimer(g_hWnd, 1, 25, NULL);

    // Start background threads
    std::thread detThread(DetectorThread);

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_running = false;
    if (detThread.joinable()) detThread.join();

    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
