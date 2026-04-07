#include "shared/State.h"
#include "shared/Overlay.h"
#include "shared/Config.h"
#include "shared/ControlPanel.h"
#include <gdiplus.h>
#include <dwmapi.h>
#include <string>

using namespace Gdiplus;

HWND CreateControlPanel(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ControlPanelWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(10, 12, 15)); // Deeper Dark
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    int w = 400, h = 540;
    HWND hPanel = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"BetterAngleControlPanel", L"BetterAngle Pro | Command Center",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        100, 100, w, h,
        NULL, NULL, hInst, NULL
    );

    // v4.6.1: Keybind Buttons
    CreateWindow(L"BUTTON", L"Change Panel Key", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
                20, 240, 150, 30, hPanel, (HMENU)101, hInst, NULL);
    CreateWindow(L"BUTTON", L"Change ROI Key", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
                20, 280, 150, 30, hPanel, (HMENU)102, hInst, NULL);
    CreateWindow(L"BUTTON", L"Change Crosshair", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
                20, 320, 150, 30, hPanel, (HMENU)103, hInst, NULL);

    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            SetTimer(hWnd, 1, 100, NULL); // Refresh stats
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) >= 101 && LOWORD(wParam) <= 103) {
                 MessageBox(hWnd, L"Press the new key after closing this message (UI development in progress)", L"Key Recorder", MB_OK);
                 // Future: Wait for VK code to rebind g_config
            }
            return 0;

        case WM_TIMER:
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Graphics graphics(hdc);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

            // Draw Professional Dashboard
            FontFamily fontFamily(L"Segoe UI");
            Font headFont(&fontFamily, 22, FontStyleBold, UnitPixel);
            SolidBrush whiteBrush(Color(255, 255, 255, 255));
            graphics.DrawString(L"Pro Command Center", -1, &headFont, PointF(20, 20), &whiteBrush);

            Pen linePen(Color(100, 0, 255, 127), 2);
            graphics.DrawLine(&linePen, 20, 60, 360, 60);

            // Live Analytics
            Font detailFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
            SolidBrush greyBrush(Color(255, 180, 180, 180));
            int matchPct = (int)(g_detectionRatio * 100);
            std::wstring matchStr = L"Live Target Match: " + std::to_wstring(matchPct) + L"%";
            graphics.DrawString(matchStr.c_str(), -1, &detailFont, PointF(20, 80), &whiteBrush);

            // Settings Header
            Font subHeader(&fontFamily, 16, FontStyleBold, UnitPixel);
            graphics.DrawString(L"CUSTOM KEYBINDS", -1, &subHeader, PointF(20, 200), &whiteBrush);

            // Instructions
            Font smallFont(&fontFamily, 11, FontStyleItalic, UnitPixel);
            graphics.DrawString(L"Closing this window will hide it to the taskbar.", -1, &smallFont, PointF(20, 460), &greyBrush);
            graphics.DrawString(L"Press F10 for crosshair | Ctrl+R for ROI selection.", -1, &smallFont, PointF(20, 480), &greyBrush);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            ShowWindow(hWnd, SW_MINIMIZE); // v4.6.1: Minimize to keep taskbar icon visible
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
