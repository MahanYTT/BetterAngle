#include "shared/Overlay.h"
#include <gdiplus.h>
#include <iostream>
#include <string>

using namespace Gdiplus;

extern bool g_isSelectionMode;
extern RECT g_selectionRect;

void DrawOverlay(HWND hwnd, double angle, const char* status, float detectionRatio, bool showCrosshair) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    int sw = rect.right - rect.left;
    int sh = rect.bottom - rect.top;
    
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, sw, sh);
    HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);
    
    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
    graphics.Clear(Color(0, 0, 0));

    // 1. Draw Precision Crosshair (F10)
    if (showCrosshair) {
        Pen crossPen(Color(255, 255, 0, 0), 1); // 1px Red
        graphics.DrawLine(&crossPen, 0, sh / 2, sw, sh / 2); // Horizontal
        graphics.DrawLine(&crossPen, sw / 2, 0, sw / 2, sh); // Vertical
    }

    // 2. Draw Visual ROI Selector (Ctrl+R)
    if (g_isSelectionMode) {
        Pen selPen(Color(255, 0, 255, 0), 2);
        graphics.DrawRectangle(&selPen, (int)g_selectionRect.left, (int)g_selectionRect.top, 
                              (int)(g_selectionRect.right - g_selectionRect.left), 
                              (int)(g_selectionRect.bottom - g_selectionRect.top));
    }

    // 3. Draw Clean Glass HUD
    int rx = 40, ry = 40, rw = 320, rh = 180;
    GraphicsPath path;
    path.AddRoundRect(Rect(rx, ry, rw, rh), 20);

    LinearGradientBrush brush(Point(rx, ry), Point(rx, ry + rh), 
                             Color(180, 15, 18, 22), Color(180, 5, 6, 8));
    graphics.FillPath(&brush, &path);
    Pen borderPen(Color(80, 255, 255, 255), 1);
    graphics.DrawPath(&borderPen, &path);

    // 4. Draw Angle Text
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 64, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(255, 0, 255, 127)); 

    std::wstring angleStr = std::to_wstring(angle);
    angleStr = angleStr.substr(0, angleStr.find(L'.') + 2) + L"\u00B0"; 
    graphics.DrawString(angleStr.c_str(), -1, &font, PointF(rx + 30, ry + 45), &textBrush);

    // 5. Match Percentage & Indicator
    int matchPct = (int)(detectionRatio * 100);
    std::wstring matchStr = L"Match: " + std::to_wstring(matchPct) + L"%";
    Font subFont(&fontFamily, 14, FontStyleBold, UnitPixel);
    SolidBrush greyBrush(Color(255, 160, 170, 180));
    graphics.DrawString(matchStr.c_str(), -1, &subFont, PointF(rx + 30, ry + 115), &greyBrush);

    Color indicatorColor = Color(255, 0, 255, 127); 
    if (detectionRatio > 0.05f) indicatorColor = Color(255, 0, 255, 255); 
    graphics.FillEllipse(&SolidBrush(indicatorColor), rx + 205, ry + 75, 14, 14); 

    // 6. Labels
    graphics.DrawString(L"CURRENT ANGLE (LIVE)", -1, &subFont, PointF(rx + 30, ry + 25), &greyBrush);

    Font miniFont(&fontFamily, 10, FontStyleRegular, UnitPixel);
    graphics.DrawString(L"Pro v4.6.1 | F10: Crosshair | Ctrl+R: ROI", -1, &miniFont, PointF(rx + 30, ry + 150), &greyBrush);

    BitBlt(hdc, 0, 0, sw, sh, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
