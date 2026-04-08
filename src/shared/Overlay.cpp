#include "shared/State.h"
#include "shared/Logic.h"
#include <gdiplus.h>
#include <iostream>
#include <string>

using namespace Gdiplus;

bool IsFortniteFocused();

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
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

    // Draw Background Snapshot (v4.9.15)
    if (g_screenSnapshot && g_currentSelection != NONE) {
        HDC hdcSnap = CreateCompatibleDC(hdcMem);
        HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
        BitBlt(hdcMem, 0, 0, sw, sh, hdcSnap, 0, 0, SRCCOPY);
        SelectObject(hdcSnap, hOldSnap);
        DeleteDC(hdcSnap);
    }

    // Cinematic Dimming & Two-Stage Selection
    if (g_currentSelection != NONE) {
        Gdiplus::SolidBrush dimBrush(Gdiplus::Color(100, 0, 0, 0)); // Light dim
        graphics.FillRectangle(&dimBrush, 0, 0, sw, sh);
        
        Gdiplus::FontFamily fontFamily(L"Segoe UI");
        Gdiplus::Font font(&fontFamily, 32, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
        
        if (g_currentSelection == SELECTING_ROI) {
            graphics.DrawString(L"STAGE 1: DRAG TO SELECT PROMPT AREA", -1, &font, Gdiplus::PointF(50.0f, 50.0f), &whiteBrush);
        } else if (g_currentSelection == SELECTING_COLOR) {
            graphics.DrawString(L"STAGE 2: CLICK TO PICK PRECISE COLOR", -1, &font, Gdiplus::PointF(50.0f, 50.0f), &whiteBrush);
            
            // Follow-Mouse Magnifier Scope (+20x, +20y offset)
            POINT mouse; GetCursorPos(&mouse);
            int scopeSize = 200;
            int offset = 20;
            int scopeX = mouse.x + offset;
            int scopeY = mouse.y + offset;

            // Screen boundary check
            if (scopeX + scopeSize > sw) scopeX = mouse.x - scopeSize - offset;
            if (scopeY + scopeSize > sh) scopeY = mouse.y - scopeSize - offset;

            GraphicsPath scopePath;
            scopePath.AddEllipse(scopeX, scopeY, scopeSize, scopeSize);
            graphics.SetClip(&scopePath);

            // Fetch zoomed content from SNAPSHOT (No Dimming)
            if (g_screenSnapshot) {
                HDC hdcSnap = CreateCompatibleDC(hdcMem);
                HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
                StretchBlt(hdcMem, scopeX, scopeY, scopeSize, scopeSize, 
                           hdcSnap, mouse.x - 15, mouse.y - 15, 31, 31, SRCCOPY);
                SelectObject(hdcSnap, hOldSnap);
                DeleteDC(hdcSnap);
            }

            // Precision Red Dot Crosshair
            SolidBrush redBrush(Color(255, 255, 0, 0));
            int centerX = scopeX + scopeSize / 2;
            int centerY = scopeY + scopeSize / 2;
            graphics.FillEllipse(&redBrush, centerX - 1, centerY - 1, 3, 3);
            
            graphics.ResetClip();
            graphics.DrawEllipse(&Pen(Color(255, 255, 255, 255), 3.0f), scopeX, scopeY, scopeSize, scopeSize);
        }
    }

    // Dynamic Live ROI Box
    if (g_showROIBox && g_selectionRect.right > g_selectionRect.left) {
        Gdiplus::Color boxColor = g_isDiving ? Gdiplus::Color(255, 255, 0, 0) : Gdiplus::Color(255, 0, 255, 0);
        if (g_currentSelection != NONE) boxColor = Gdiplus::Color(255, 0, 255, 0); 
        
        Gdiplus::Pen roiPen(boxColor, 2.0f);
        graphics.DrawRectangle(&roiPen, g_selectionRect.left, g_selectionRect.top, 
            g_selectionRect.right - g_selectionRect.left, g_selectionRect.bottom - g_selectionRect.top);
    }

    // Target Color Circle Feedback (BGR/RGB Sync Fix)
    int circleX = 200; 
    int circleY = 100;
    // User requested swaped B/R for sync: RGB(GetBValue, GetGValue, GetRValue)
    Gdiplus::Color previewColor(255, GetBValue(g_targetColor), GetGValue(g_targetColor), GetRValue(g_targetColor));
    Gdiplus::SolidBrush targetColorBrush(previewColor);
    Gdiplus::Pen circleBorder(Gdiplus::Color(255, 200, 200, 200), 1.0f);

    graphics.FillEllipse(&targetColorBrush, circleX, circleY, 15, 15);
    graphics.DrawEllipse(&circleBorder, circleX, circleY, 15, 15);

    // 2. Draw Precision Crosshair (F10)
    if (showCrosshair) {
        Gdiplus::Pen crossPen(Gdiplus::Color(255, 255, 0, 0), 1);
        graphics.DrawLine(&crossPen, 0, sh / 2, sw, sh / 2);
        graphics.DrawLine(&crossPen, sw / 2, 0, sw / 2, sh);
    }

    // 4. Draw Clean Glass HUD
    int rx = 40, ry = 40, rw = 320, rh = 180, r = 20;
    GraphicsPath path;
    path.AddArc(rx, ry, r, r, 180, 90);
    path.AddArc(rx + rw - r, ry, r, r, 270, 90);
    path.AddArc(rx + rw - r, ry + rh - r, r, r, 0, 90);
    path.AddArc(rx, ry + rh - r, r, r, 90, 90);
    path.CloseFigure();

    LinearGradientBrush brush(Point(rx, ry), Point(rx, ry + rh), 
                             Color(180, 15, 18, 22), Color(180, 5, 6, 8));
    graphics.FillPath(&brush, &path);
    Pen borderPen(Color(80, 255, 255, 255), 1);
    graphics.DrawPath(&borderPen, &path);

    // 5. Draw Angle Text
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 64, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(255, 0, 255, 127)); 

    std::wstring angleStr = std::to_wstring(angle);
    angleStr = angleStr.substr(0, angleStr.find(L'.') + 2) + L"\u00B0"; 
    graphics.DrawString(angleStr.c_str(), -1, &font, PointF(rx + 30, ry + 45), &textBrush);

    // 6. Match Percentage & Indicator
    int matchPct = (int)(detectionRatio * 100);
    std::wstring matchStr = L"Match: " + std::to_wstring(matchPct) + L"%";
    Font subFont(&fontFamily, 14, FontStyleBold, UnitPixel);
    SolidBrush greyBrush(Color(255, 160, 170, 180));
    graphics.DrawString(matchStr.c_str(), -1, &subFont, PointF(rx + 30, ry + 115), &greyBrush);

    Color indicatorColor = Color(255, 0, 255, 127); 
    if (detectionRatio > 0.05f) indicatorColor = Color(255, 0, 255, 255); 
    graphics.FillEllipse(&SolidBrush(indicatorColor), rx + 205, ry + 75, 14, 14); 

    // 7. Mini Labels
    graphics.DrawString(L"CURRENT ANGLE (LIVE)", -1, &subFont, PointF(rx + 30, ry + 25), &greyBrush);

    Font miniFont(&fontFamily, 10, FontStyleRegular, UnitPixel);
    graphics.DrawString(L"Release Workflow Fix v4.9.24 | Auto-Installer Ready", -1, &miniFont, PointF(rx + 30, ry + 150), &greyBrush);

    // 8. Debug Menu (Ctrl + 9)
    if (g_debugMode) {
        int dx = 40, dy = 250, dw = 320, dh = 180;
        GraphicsPath dbgPath;
        dbgPath.AddRectangle(Rect(dx, dy, dw, dh));
        graphics.FillPath(&SolidBrush(Color(200, 10, 10, 15)), &dbgPath);
        graphics.DrawPath(&Pen(Color(150, 255, 255, 255), 1), &dbgPath);

        Font dbgTitleFont(&fontFamily, 14, FontStyleBold, UnitPixel);
        Font dbgFont(&fontFamily, 11, FontStyleRegular, UnitPixel);
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush yellowBrush(Color(255, 255, 255, 0));

        graphics.DrawString(L"DEBUG DASHBOARD", -1, &dbgTitleFont, PointF(dx + 15, dy + 10), &yellowBrush);

        std::wstring rawAngle = L"Raw Angle: " + std::to_wstring(angle);
        std::wstring focusStr = L"Fortnite Focused: ";
        focusStr += IsFortniteFocused() ? L"YES" : L"NO";
        
        std::wstring scaleStr = L"Active Scale: " + std::to_wstring(g_logic.GetScale());
        std::wstring detStr = L"Raw Detection: " + std::to_wstring(detectionRatio);
        
        std::wstring cursorStr = L"Cursor Visible: ";
        cursorStr += g_isCursorVisible ? L"YES" : L"NO";
        
        std::wstring selectionStr = L"Selection State: ";
        selectionStr += (g_currentSelection == NONE) ? L"NONE" : ((g_currentSelection == SELECTING_ROI) ? L"ROI" : L"COLOR");

        graphics.DrawString(rawAngle.c_str(), -1, &dbgFont, PointF(dx + 15, dy + 40), &whiteBrush);
        graphics.DrawString(focusStr.c_str(), -1, &dbgFont, PointF(dx + 15, dy + 60), &whiteBrush);
        graphics.DrawString(scaleStr.c_str(), -1, &dbgFont, PointF(dx + 15, dy + 80), &whiteBrush);
        graphics.DrawString(detStr.c_str(), -1, &dbgFont, PointF(dx + 15, dy + 100), &whiteBrush);
        graphics.DrawString(cursorStr.c_str(), -1, &dbgFont, PointF(dx + 15, dy + 120), &whiteBrush);
        graphics.DrawString(selectionStr.c_str(), -1, &dbgFont, PointF(dx + 15, dy + 140), &whiteBrush);
        graphics.DrawString(L"Mode: SECRET DEBUG ACTIVE", -1, &dbgFont, PointF(dx + 15, dy + 160), &yellowBrush);
    }

    BitBlt(hdc, 0, 0, sw, sh, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
