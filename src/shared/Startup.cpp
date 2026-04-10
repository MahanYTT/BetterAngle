#include "shared/Startup.h"
#include <gdiplus.h>
#include <thread>
#include <chrono>

using namespace Gdiplus;

LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // Paint is handled in the main loop for the splash, but we'll clear here as safely.
        EndPaint(hWnd, &ps);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void ShowSplashLoader(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInst;
    wc.hbrBackground = CreateSolidBrush(RGB(15, 17, 22)); // Deep Charcoal
    wc.lpszClassName = L"BetterAngleSplash";
    RegisterClass(&wc);

    int w = 450, h = 250;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    HWND hSplash = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, L"BetterAngleSplash", L"Loading BetterAngle Pro", WS_POPUP, sx, sy, w, h, NULL, NULL, hInst, NULL);
    SetLayeredWindowAttributes(hSplash, 0, 255, LWA_ALPHA);
    ShowWindow(hSplash, SW_SHOW);
    UpdateWindow(hSplash);

    HDC hdc = GetDC(hSplash);
    {
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        FontFamily ff(L"Segoe UI");
        Font titleFont(&ff, 28, FontStyleBold, UnitPixel);
        Font subFont(&ff, 14, FontStyleRegular, UnitPixel);
        Font gearFont(&ff, 10, FontStyleRegular, UnitPixel);

        SolidBrush white(Color(255, 255, 255, 255));
        SolidBrush neon(Color(255, 0, 255, 255)); // Neon Cyan/Blue
        SolidBrush gray(Color(255, 100, 105, 115));
        SolidBrush bg(Color(255, 15, 17, 22));

        DWORD start = GetTickCount();
        while (GetTickCount() - start < 2200) {
            DWORD elapsed = GetTickCount() - start;
            float progress = (float)elapsed / 2000.0f;
            if (progress > 1.0f) progress = 1.0f;

            // Handle messages to keep UI alive
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // Background flicker-free draw
            g.Clear(Color(255, 15, 17, 22));
            
            // Neon Border
            Pen neonPen(Color(255, 0, 255, 255), 1);
            g.DrawRectangle(&neonPen, 0, 0, w - 1, h - 1);

            // Text
            g.DrawString(L"BetterAngle Pro", -1, &titleFont, PointF(40, 60), &white);
            g.DrawString(L"HIGH PERFORMANCE GAMING OPTIMIZER", -1, &gearFont, PointF(42, 100), &neon);
            
            std::wstring status = L"INITIALIZING ENGINE & ASSETS...";
            if (elapsed > 1500) status = L"OPTIMIZING CALCULATIONS...";
            g.DrawString(status.c_str(), -1, &subFont, PointF(40, 140), &gray);

            // Progress Bar Background
            SolidBrush barBg(Color(255, 40, 45, 55));
            g.FillRectangle(&barBg, 40, 180, 370, 6);

            // Progress Bar Foreground (Neon Cyan)
            RectF barRect(40, 180, 370 * progress, 6);
            LinearGradientBrush grad(barRect, Color(255, 0, 200, 255), Color(255, 0, 255, 200), LinearGradientModeHorizontal);
            g.FillRectangle(&grad, barRect);

            // Version
            g.DrawString(L"v4.12.1 | STABLE", -1, &gearFont, PointF(40, 200), &gray);

            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    } // Graphics g is destroyed here, SAFE before ReleaseDC

    ReleaseDC(hSplash, hdc);
    DestroyWindow(hSplash);
}
