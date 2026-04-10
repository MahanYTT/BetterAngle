#include <windows.h>
#include <gdiplus.h>
#include <thread>
#include <chrono>
#include <string>
#include "shared/Startup.h"
#include "shared/State.h"

using namespace Gdiplus;

void ShowSplashLoader(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInst;
    wc.hbrBackground = CreateSolidBrush(RGB(15, 17, 22));
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
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);

    {
        Graphics g(hdcMem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        FontFamily ff(L"Segoe UI");
        Font titleFont(&ff, 28, FontStyleBold, UnitPixel);
        Font subFont(&ff, 14, FontStyleRegular, UnitPixel);
        Font gearFont(&ff, 10, FontStyleRegular, UnitPixel);

        SolidBrush white(Color(255, 255, 255, 255));
        SolidBrush neon(Color(255, 0, 255, 255));
        SolidBrush gray(Color(255, 100, 105, 115));

        DWORD start = GetTickCount();
        DWORD duration = 1800;

        while (true) {
            DWORD elapsed = GetTickCount() - start;
            if (elapsed > duration) break;

            float progress = (float)elapsed / (float)(duration - 200);
            if (progress > 1.0f) progress = 1.0f;

            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            g.Clear(Color(255, 15, 17, 22));
            
            // Neon Border
            Pen neonPen(Color(255, 0, 255, 255), 1);
            g.DrawRectangle(&neonPen, 0, 0, w - 1, h - 1);

            // Title
            g.DrawString(L"BetterAngle Pro", -1, &titleFont, PointF(40, 60), &white);
            g.DrawString(L"HIGH PERFORMANCE GAMING OPTIMIZER", -1, &gearFont, PointF(42, 100), &neon);
            
            std::wstring status = progress < 0.7f ? L"INITIALIZING ENGINE..." : L"READY TO LOCK IN...";
            g.DrawString(status.c_str(), -1, &subFont, PointF(40, 140), &gray);

            // Progress Bar
            SolidBrush barBg(Color(255, 40, 45, 55));
            g.FillRectangle(&barBg, 40, 180, 370, 6);
            RectF barRect(40, 180, 370 * progress, 6);
            LinearGradientBrush grad(barRect, Color(255, 0, 200, 255), Color(255, 0, 255, 200), LinearGradientModeHorizontal);
            g.FillRectangle(&grad, barRect);

            std::wstring verLabel = L"v" + std::wstring(VERSION_WSTR) + L" | STABLE";
            g.DrawString(verLabel.c_str(), -1, &gearFont, PointF(40, 200), &gray);

            BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Smooth Fade Out
        for (int a = 255; a >= 0; a -= 15) {
            SetLayeredWindowAttributes(hSplash, 0, (BYTE)a, LWA_ALPHA);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(hSplash, hdc);
    DestroyWindow(hSplash);
}
