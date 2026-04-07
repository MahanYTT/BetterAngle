#include "shared/State.h"
#include <gdiplus.h>
#include <iostream>
#include <string>

using namespace Gdiplus;

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
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit); // v4.7.3: UHD Sharpening
    graphics.Clear(Color(0, 0, 0, 0));

    // 1. Cinematic Screen Dimming
    if (g_isSelectionMode) {
        SolidBrush dimBrush(Color(120, 0, 0, 0)); // Semi-transparent black
        graphics.FillRectangle(&dimBrush, 0, 0, sw, sh);
    }

    // 2. Draw Precision Crosshair (F10)
    if (showCrosshair) {
        Pen crossPen(Color(255, 255, 0, 0), 1);
        graphics.DrawLine(&crossPen, 0, sh / 2, sw, sh / 2);
        graphics.DrawLine(&crossPen, sw / 2, 0, sw / 2, sh);
    }

    // 3. Draw Visual ROI Selector & Microscope
    if (g_isSelectionMode) {
        // Guidance Text
        FontFamily ff(L"Segoe UI");
        Font guideFont(&ff, 28, FontStyleBold, UnitPixel);
        SolidBrush white(Color(255, 255, 255, 255));
        
        std::wstring guide = (g_selectionStep == 0) ? L"STEP 1: DRAG BOX OVER PROMPT" : L"STEP 2: PICK TARGET COLOR";
        graphics.DrawString(guide.c_str(), -1, &guideFont, PointF(sw / 2.0f - 200, sh / 2.0f - 100), &white);

        if (g_selectionStep == 0) {
            Pen selPen(Color(255, 0, 255, 255), 2); // Cyan for selection
            graphics.DrawRectangle(&selPen, (int)g_selectionRect.left, (int)g_selectionRect.top, 
                                  (int)(g_selectionRect.right - g_selectionRect.left), 
                                  (int)(g_selectionRect.bottom - g_selectionRect.top));
        } else {
            // PIXEL MICROSCOPE (Magnifier)
            POINT cur;
            GetCursorPos(&cur);
            ScreenToClient(hwnd, &cur);

            int zoomSize = 140;
            int halfZoom = zoomSize / 2;
            int zoomFactor = 10;
            int captureSize = zoomSize / zoomFactor;

            // Capture screen around mouse
            HDC hdcScreen = GetDC(NULL);
            HDC hdcZoom = CreateCompatibleDC(hdcScreen);
            HBITMAP hbmZoom = CreateCompatibleBitmap(hdcScreen, captureSize, captureSize);
            SelectObject(hdcZoom, hbmZoom);
            
            POINT screenCur;
            GetCursorPos(&screenCur);
            BitBlt(hdcZoom, 0, 0, captureSize, captureSize, hdcScreen, screenCur.x - captureSize/2, screenCur.y - captureSize/2, SRCCOPY);
            
            Bitmap bmp(hbmZoom, NULL);
            graphics.DrawImage(&bmp, cur.x + 20, cur.y + 20, zoomSize, zoomSize); // Draw zoomed area

            // Microscope Frame & Crosshair
            Pen lensPen(Color(255, 255, 255, 255), 2);
            graphics.DrawRectangle(&lensPen, cur.x + 20, cur.y + 20, zoomSize, zoomSize);
            graphics.DrawLine(&lensPen, cur.x + 20 + halfZoom, cur.y + 20, cur.x + 20 + halfZoom, cur.y + 20 + zoomSize);
            graphics.DrawLine(&lensPen, cur.x + 20, cur.y + 20 + halfZoom, cur.x + 20 + zoomSize, cur.y + 20 + halfZoom);

            DeleteObject(hbmZoom);
            DeleteDC(hdcZoom);
            ReleaseDC(NULL, hdcScreen);
        }
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
    graphics.DrawString(L"Flagship Suite v4.7.3 | Pro HD Engine", -1, &miniFont, PointF(rx + 30, ry + 150), &greyBrush);

    BitBlt(hdc, 0, 0, sw, sh, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
