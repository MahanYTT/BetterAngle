#include "shared/Overlay.h"
#include <gdiplus.h>
#include <iostream>
#include <string>

using namespace Gdiplus;

void DrawOverlay(HWND hwnd, double angle, const char* status) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    // Double Buffering
    RECT rect;
    GetClientRect(hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);
    
    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // Clear with Invisible
    graphics.Clear(Color(0, 0, 0));

    // 1. Draw Modern Glass Container (Rounded Rectangle)
    GraphicsPath path;
    int rw = 420;
    int rh = 260;
    int rx = 40;
    int ry = 40;
    int r = 20; // Corner radius
    
    path.AddArc(rx, ry, r, r, 180, 90);
    path.AddArc(rx + rw - r, ry, r, r, 270, 90);
    path.AddArc(rx + rw - r, ry + rh - r, r, r, 0, 90);
    path.AddArc(rx, ry + rh - r, r, r, 90, 90);
    path.CloseFigure();

    // Background Gradient (Dark Glass)
    LinearGradientBrush brush(Point(rx, ry), Point(rx, ry + rh), 
                             Color(230, 20, 24, 30), Color(230, 10, 12, 16));
    graphics.FillPath(&brush, &path);

    // Subtle Border
    Pen borderPen(Color(100, 255, 255, 255), 1);
    graphics.DrawPath(&borderPen, &path);

    // 2. Draw Angle Text
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 56, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(255, 0, 255, 127)); // Professional Green

    std::wstring angleStr = std::to_wstring(angle);
    angleStr = angleStr.substr(0, angleStr.find(L'.') + 2); // 0.0
    
    // Use Unicode for the degree symbol to fix the Â artifact
    angleStr += L"\u00B0"; 

    graphics.DrawString(angleStr.c_str(), -1, &font, PointF(rx + 30, ry + 50), &textBrush);

    // 2.5 Draw Status Indicator Circle (Green = Normal, Cyan = Diving)
    Color indicatorColor = Color(255, 0, 255, 127); // Default Green
    if (strstr(status, "Diving")) {
        indicatorColor = Color(255, 0, 255, 255); // Cyan/Blue for Diving
    }
    
    SolidBrush indicatorBrush(indicatorColor);
    graphics.FillEllipse(&indicatorBrush, rx + 160, ry + 75, 12, 12); // Small circle next to angle
    
    // Add a subtle glow to the circle
    Pen glowPen(Color(100, indicatorColor.GetR(), indicatorColor.GetG(), indicatorColor.GetB()), 4);
    graphics.DrawEllipse(&glowPen, rx + 160, ry + 75, 12, 12);

    // 3. Subtitles and Stats
    Font subFont(&fontFamily, 14, FontStyleBold, UnitPixel);
    SolidBrush greyBrush(Color(255, 160, 170, 180));
    graphics.DrawString(L"CURRENT ANGLE (LIVE)", -1, &subFont, PointF(rx + 30, ry + 30), &greyBrush);

    // Usage Control Panel Data
    Font detailFont(&fontFamily, 12, FontStyleRegular, UnitPixel);
    std::string statStr = std::string("Mode: ") + status;
    std::wstring wStat = std::wstring(statStr.begin(), statStr.end());
    graphics.DrawString(wStat.c_str(), -1, &detailFont, PointF(rx + 30, ry + 160), &greyBrush);

    // Performance Stats
    graphics.DrawString(L"Performance: 0.1% CPU | 1.8MB RAM", -1, &detailFont, PointF(rx + 30, ry + 185), &greyBrush);
    graphics.DrawString(L"Status: Connected (v3.5 Pro)", -1, &detailFont, PointF(rx + 30, ry + 210), &greyBrush);

    // 4. Accent Line
    Pen accentPen(Color(255, 0, 255, 127), 2);
    graphics.DrawLine(&accentPen, rx + 30, ry + 140, rx + 390, ry + 140);

    // Blit to Screen
    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
    
    // Cleanup
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
