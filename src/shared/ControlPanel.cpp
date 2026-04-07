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
    wc.hbrBackground = CreateSolidBrush(RGB(10, 12, 15)); 
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    int w = 400, h = 640; 
    HWND hPanel = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"BetterAngleControlPanel", L"BetterAngle Pro | Command Center",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        100, 100, w, h,
        NULL, NULL, hInst, NULL
    );

    // v4.7.3: Quit Button Logic (Owner Draw for Glass Effect)
    CreateWindow(L"BUTTON", L"QUIT", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 
                20, 530, 345, 45, hPanel, (HMENU)105, hInst, NULL);

    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            SetTimer(hWnd, 1, 100, NULL); 
            return 0;

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            if (pdis->CtlID == 105) { // Glossy QUIT Button
                Graphics g(pdis->hDC);
                g.SetSmoothingMode(SmoothingModeAntiAlias);
                
                Rect r(0, 0, pdis->rcItem.right, pdis->rcItem.bottom);
                
                // Rounded path
                GraphicsPath path;
                int corner = 12;
                path.AddArc(r.X, r.Y, corner, corner, 180, 90);
                path.AddArc(r.X + r.Width - corner, r.Y, corner, corner, 270, 90);
                path.AddArc(r.X + r.Width - corner, r.Y + r.Height - corner, corner, corner, 0, 90);
                path.AddArc(r.X, r.Y + r.Height - corner, corner, corner, 90, 90);
                path.CloseFigure();

                // Premium Red Glass Gradient
                Color c1 = Color(255, 180, 20, 30);
                Color c2 = Color(255, 100, 5, 10);
                if (pdis->itemState & ODS_SELECTED) { c1 = Color(255, 140, 10, 15); c2 = Color(255, 60, 2, 5); }
                
                LinearGradientBrush brush(r, c1, c2, LinearGradientModeVertical);
                g.FillPath(&brush, &path);

                // Glossy Sheen Overlay
                RectF sheenRect((float)r.X, (float)r.Y, (float)r.Width, (float)r.Height / 2.0f);
                LinearGradientBrush sheenBrush(sheenRect, Color(100, 255, 255, 255), Color(0, 255, 255, 255), LinearGradientModeVertical);
                g.FillRectangle(&sheenBrush, sheenRect);

                Pen borderPen(Color(120, 255, 255, 255), 1);
                g.DrawPath(&borderPen, &path);

                FontFamily ff(L"Segoe UI");
                Font font(&ff, 14, FontStyleBold, UnitPixel);
                SolidBrush white(Color(255, 255, 255, 255));
                
                StringFormat format;
                format.SetAlignment(StringAlignmentCenter);
                format.SetLineAlignment(StringAlignmentCenter);
                
                g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
                g.DrawString(L"QUIT SUITE", -1, &font, RectF(0, 0, (float)r.Width, (float)r.Height), &format, &white);
                return TRUE;
            }
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == 105) { // QUIT button clicked
                PostQuitMessage(0);
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
            graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit); 

            // Draw Professional Dashboard
            FontFamily fontFamily(L"Segoe UI");
            Font headFont(&fontFamily, 22, FontStyleBold, UnitPixel);
            SolidBrush whiteBrush(Color(255, 255, 255, 255));
            graphics.DrawString(L"Pro Command Center", -1, &headFont, PointF(20, 20), &whiteBrush);

            Pen linePen(Color(100, 0, 255, 127), 2);
            graphics.DrawLine(&linePen, 20, 60, 360, 60);

            // Live Analytics
            Font detailFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
            int matchPct = (int)(g_detectionRatio * 100);
            std::wstring matchStr = L"Live Target Match: " + std::to_wstring(matchPct) + L"%";
            graphics.DrawString(matchStr.c_str(), -1, &detailFont, PointF(20, 80), &whiteBrush);

            // Version Info
            Font smallFont(&fontFamily, 11, FontStyleItalic, UnitPixel);
            SolidBrush greyBrush(Color(255, 180, 180, 180));
            graphics.DrawString(L"Version: 4.7.3 Flagship Overhaul", -1, &smallFont, PointF(20, 470), &greyBrush);
            graphics.DrawString(L"Build Date: April 2026", -1, &smallFont, PointF(20, 490), &greyBrush);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            ShowWindow(hWnd, SW_MINIMIZE); 
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
