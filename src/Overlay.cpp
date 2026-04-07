#include "Overlay.h"
#include <gdiplus.h>
#include <iostream>
#include <string>

using namespace Gdiplus;

// Global Resources
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

void InitializeOverlay(HWND hwnd) {
    // Already in main.cpp
}

void CleanupOverlay() {
    // Already in main.cpp
}

void DrawOverlay(HWND hwnd, double angle, const char* status) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    Graphics graphics(hdc);

    // Double buffering could be used for smoother redraws, but GDI+ is usually fine.
    // CLEAR (Invisible by setting color to C_INVISIBLE)
    graphics.Clear(Color(0, 0, 0)); // Black will be transparent due to COLORKEY

    // Drawing the background box
    SolidBrush boxBrush(Color(255, 30, 34, 40));
    graphics.FillRectangle(&boxBrush, 30, 30, 400, 240);

    // Drawing the angle text
    Font font(L"Segoe UI", 48, FontStyleBold);
    SolidBrush textBrush(Color(255, 0, 255, 127)); // Green

    std::wstring angleStr = std::to_wstring(angle);
    angleStr = angleStr.substr(0, angleStr.find(L'.') + 2) + L"°";

    PointF origin(50.0f, 65.0f);
    graphics.DrawString(angleStr.c_str(), -1, &font, origin, &textBrush);

    // Drawing subtitle
    Font subFont(L"Segoe UI", 16);
    SolidBrush greyBrush(Color(255, 175, 182, 196));
    graphics.DrawString(L"CURRENT ANGLE", -1, &subFont, PointF(50, 40), &greyBrush);

    // Drawing status line
    std::string statusText = std::string("Status: ") + status;
    std::wstring wStatus = std::wstring(statusText.begin(), statusText.end());
    graphics.DrawString(wStatus.c_str(), -1, &subFont, PointF(50, 160), &greyBrush);

    EndPaint(hwnd, &ps);
}
