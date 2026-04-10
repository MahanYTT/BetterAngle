#include "shared/State.h"
#include "shared/Logic.h"
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

using namespace Gdiplus;

bool IsFortniteFocused();

// Helper: format float to N decimal places
static std::wstring FmtFloat(double v, int decimals = 2) {
    std::wostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << v;
    return ss.str();
}

// Static FPS tracking
static ULONGLONG s_lastFrameTime = 0;
static float     s_fps           = 0.0f;
static int       s_frameCount    = 0;
static ULONGLONG s_fpsTimer      = 0;

static void TickFPS() {
    ULONGLONG now = GetTickCount64();
    s_frameCount++;
    if (now - s_fpsTimer >= 500) {          // update every 0.5s
        s_fps = s_frameCount * 1000.0f / float(now - s_fpsTimer);
        s_frameCount = 0;
        s_fpsTimer   = now;
    }
}

void DrawOverlay(HWND hwnd, double angle, float detectionRatio, bool showCrosshair) {
    TickFPS();

    // Cache GDI+ Resources (Memory Leak Fix)
    static FontFamily ff(L"Segoe UI");
    static Font Font_Large(&ff, 54, FontStyleBold, UnitPixel);
    static Font Font_Med(&ff, 28, FontStyleBold, UnitPixel);
    static Font Font_Small(&ff, 15, FontStyleRegular, UnitPixel);
    static Font Font_Label(&ff, 12, FontStyleBold, UnitPixel);
    static Font Font_LabelSmall(&ff, 10, FontStyleRegular, UnitPixel);
    static Font Font_Tiny(&ff, 9, FontStyleRegular, UnitPixel);
    static Font Font_DbgTitle(&ff, 11, FontStyleBold, UnitPixel);
    static Font Font_DbgKey(&ff, 10, FontStyleBold, UnitPixel);
    static Font Font_DbgVal(&ff, 10, FontStyleRegular, UnitPixel);

    // Cached Brushes/Pens to prevent per-frame allocation leaks
    static SolidBrush dimBrush(Color(120, 0, 0, 0));
    static SolidBrush whiteBrush(Color(255, 255, 255, 255));
    static SolidBrush dimWhite(Color(180, 220, 220, 220));
    static SolidBrush redBrush(Color(255, 255, 50, 50));
    static SolidBrush bgPanelBrush(Color(255, 0, 0, 0));
    static SolidBrush matchLabelBrush(Color(120, 180, 190, 200));
    
    static Pen xhairScopePen(Color(200, 255, 50, 50), 1.0f);
    static Pen scopeRingPen(Color(220, 255, 255, 255), 2.5f);
    static Pen shadowRingPen(Color(80, 0, 0, 0), 6.0f);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rect;
    GetClientRect(hwnd, &rect);
    int sw = rect.right  - rect.left;
    int sh = rect.bottom - rect.top;

    HDC      hdcMem = CreateCompatibleDC(hdc);
    HBITMAP  hbmMem = CreateCompatibleBitmap(hdc, sw, sh);
    HGDIOBJ  hOld   = SelectObject(hdcMem, hbmMem);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    graphics.Clear(Color(0, 0, 0, 0));

    if (g_screenSnapshot && g_currentSelection != NONE) {
        HDC     hdcSnap  = CreateCompatibleDC(hdcMem);
        HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
        BitBlt(hdcMem, 0, 0, sw, sh, hdcSnap, 0, 0, SRCCOPY);
        SelectObject(hdcSnap, hOldSnap);
        DeleteDC(hdcSnap);
    }

    if (g_currentSelection != NONE) {
        graphics.FillRectangle(&dimBrush, 0, 0, sw, sh);

        if (g_currentSelection == SELECTING_ROI) {
            graphics.DrawString(L"STAGE 1  ·  Drag to select the dive prompt area",
                                -1, &Font_Med, PointF(50.0f, 42.0f), &whiteBrush);
            graphics.DrawString(L"Press the hotkey again to cancel",
                                -1, &Font_Small,  PointF(52.0f, 80.0f), &dimWhite);

            if (g_selectionRect.right > g_selectionRect.left) {
                Pen dashPen(Color(200, 255, 255, 255), 1.5f);
                REAL dash[] = { 6.0f, 4.0f };
                dashPen.SetDashPattern(dash, 2);
                graphics.DrawRectangle(&dashPen,
                    g_selectionRect.left, g_selectionRect.top,
                    g_selectionRect.right  - g_selectionRect.left,
                    g_selectionRect.bottom - g_selectionRect.top);
            }

        } else if (g_currentSelection == SELECTING_COLOR) {
            graphics.DrawString(L"STAGE 2  ·  Click to pick the prompt colour",
                                -1, &Font_Med, PointF(50.0f, 42.0f), &whiteBrush);
            graphics.DrawString(L"Hover over the brightest part of the prompt text",
                                -1, &Font_Small,  PointF(52.0f, 80.0f), &dimWhite);

            POINT mouse; GetCursorPos(&mouse);
            int scopeSize = 210, offset = 24;
            int scopeX = mouse.x + offset, scopeY = mouse.y + offset;
            if (scopeX + scopeSize > sw) scopeX = mouse.x - scopeSize - offset;
            if (scopeY + scopeSize > sh) scopeY = mouse.y - scopeSize - offset;

            graphics.DrawEllipse(&shadowRingPen, (REAL)scopeX - 1, (REAL)scopeY - 1, (REAL)scopeSize + 2, (REAL)scopeSize + 2);

            GraphicsPath scopePath;
            scopePath.AddEllipse(scopeX, scopeY, scopeSize, scopeSize);
            graphics.SetClip(&scopePath);

            if (g_screenSnapshot) {
                HDC     hdcSnap  = CreateCompatibleDC(hdcMem);
                HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
                StretchBlt(hdcMem, scopeX, scopeY, scopeSize, scopeSize,
                           hdcSnap, mouse.x - 16, mouse.y - 16, 33, 33, SRCCOPY);
                SelectObject(hdcSnap, hOldSnap);
                DeleteDC(hdcSnap);
            }

            int centerX = scopeX + scopeSize / 2, centerY = scopeY + scopeSize / 2;
            graphics.DrawLine(&xhairScopePen, centerX - scopeSize/2, centerY, centerX + scopeSize/2, centerY);
            graphics.DrawLine(&xhairScopePen, centerX, centerY - scopeSize/2, centerX, centerY + scopeSize/2);

            graphics.ResetClip();
            graphics.FillEllipse(&redBrush, centerX - 3, centerY - 3, 6, 6);
            graphics.DrawEllipse(&scopeRingPen, scopeX, scopeY, scopeSize, scopeSize);
        }
    }

    if (g_showROIBox && g_selectionRect.right > g_selectionRect.left) {
        Color boxColor = g_isDiving ? Color(220, 255, 60, 60) : Color(220, 60, 230, 80);
        if (g_currentSelection != NONE) boxColor = Color(200, 255, 255, 255);
        Pen roiPen(boxColor, 1.5f);
        graphics.DrawRectangle(&roiPen,
            g_selectionRect.left, g_selectionRect.top,
            g_selectionRect.right  - g_selectionRect.left,
            g_selectionRect.bottom - g_selectionRect.top);
    }

    if (showCrosshair) {
        float cx = sw / 2.0f + g_crossOffsetX, cy = sh / 2.0f + g_crossOffsetY;
        float a = 255.0f;
        if (g_crossPulse) {
            LARGE_INTEGER freq, cnt;
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&cnt);
            float t = float(cnt.QuadPart % (freq.QuadPart * 2)) / float(freq.QuadPart);
            float opacity = 0.5f + 0.5f * sinf(t * 3.14159265f * 2.0f);
            if (opacity < 0.05f) opacity = 0.05f;
            a = opacity * 255.0f;
        }
        Color  crossC((BYTE)a, GetRValue(g_crossColor), GetGValue(g_crossColor), GetBValue(g_crossColor));
        Pen    crossPen(crossC, g_crossThickness);
        crossPen.SetLineJoin(LineJoinRound);
        float rad = g_crossAngle * (3.1415926535f / 180.0f), sinR = sinf(rad), cosR = cosf(rad);
        float l = (sw > sh ? sw : sh) * 3.0f;
        graphics.DrawLine(&crossPen, cx - sinR*l, cy + cosR*l, cx + sinR*l, cy - cosR*l);
        graphics.DrawLine(&crossPen, cx - cosR*l, cy - sinR*l, cx + cosR*l, cy + sinR*l);
    }

    const int rx = g_hudX, ry = g_hudY, rw = 160, rh = 80, RAD = 10;
    GraphicsPath path;
    path.AddArc(rx, ry, RAD, RAD, 180, 90);
    path.AddArc(rx + rw - RAD, ry, RAD, RAD, 270, 90);
    path.AddArc(rx + rw - RAD, ry + rh - RAD, RAD, RAD, 0, 90);
    path.AddArc(rx, ry + rh - RAD, RAD, RAD, 90, 90);
    path.CloseFigure();

    graphics.FillPath(&bgPanelBrush, &path);
    Color borderCol = g_isDiving ? Color(255, 255, 255, 255) : Color(255, 50, 50, 50);
    Pen borderPen(borderCol, 1.0f);
    graphics.DrawPath(&borderPen, &path);

    std::wstring angleStr = FmtFloat(angle, 1) + L"°"; 
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    Color angleCol = g_isDiving ? Color(255, 255, 255, 255) : Color(255, 240, 240, 240);
    SolidBrush angleBrush(angleCol);
    graphics.DrawString(angleStr.c_str(), -1, &Font_Large, RectF((REAL)rx, (REAL)ry, (REAL)rw, (REAL)rh), &sf, &angleBrush);

    if (g_debugMode) {
        const int DBG_ROWS = 14, ROW_H = 18;
        int dw = 370, dh = 28 + DBG_ROWS * ROW_H + 10;
        int dx = rx, dy = ry + rh + 10;
        if (dy + dh > sh) dy = ry - dh - 8;

        GraphicsPath dbgPath;
        dbgPath.AddRectangle(Rect(dx, dy, dw, dh));
        
        // Static colors for debug rows
        static SolidBrush colTitle(Color(255, 255, 255, 255));
        static SolidBrush colKey(Color(255, 180, 180, 180));
        static SolidBrush colVal(Color(255, 220, 230, 240));
        static SolidBrush colGood(Color(255, 60, 230, 100));
        static SolidBrush colBad(Color(255, 255, 70, 70));
        static SolidBrush colWarn(Color(255, 255, 210, 50));
        static SolidBrush titleBarBrush(Color(100, 40, 40, 40));

        LinearGradientBrush dbgBg(Point(dx, dy), Point(dx, dy + dh), Color(225, 8, 10, 14), Color(225, 3, 4, 6));
        graphics.FillPath(&dbgBg, &dbgPath);
        graphics.DrawPath(&Pen(Color(100, 0, 190, 255), 1.0f), &dbgPath);
        graphics.FillRectangle(&titleBarBrush, dx, dy, dw, 22);

        graphics.DrawString(L"  DEBUG DASHBOARD", -1, &Font_DbgTitle, PointF(float(dx + 4), float(dy + 5)), &colTitle);

        int row = 0;
        auto DrawRow = [&](const wchar_t* key, const std::wstring& val, SolidBrush* valBrush) {
            float y = float(dy + 28 + row * ROW_H);
            graphics.DrawString(key, -1, &Font_DbgKey, PointF(float(dx + 8), y), &colKey);
            graphics.DrawString(val.c_str(), -1, &Font_DbgVal, PointF(float(dx + 175), y), valBrush);
            if (row > 0) graphics.DrawLine(&Pen(Color(20, 255, 255, 255), 1.0f), dx + 4, int(y) - 1, dx + dw - 4, int(y) - 1);
            row++;
        };

        bool fortFocused = IsFortniteFocused();
        int matchPct = int(detectionRatio * 100.0f);
        DrawRow(L"FPS", FmtFloat(s_fps, 0), s_fps >= 60.0f ? &colGood : &colWarn);
        DrawRow(L"Angle (raw)", FmtFloat(angle, 2) + L"°", &colVal);
        DrawRow(L"Detection Ratio", FmtFloat(detectionRatio * 100.0, 0) + L"% / match " + std::to_wstring(matchPct) + L"%", matchPct > 5 ? &colGood : &colVal);
        DrawRow(L"Diving", g_isDiving ? L"YES" : L"NO", g_isDiving ? &colGood : &colVal);
        DrawRow(L"Fortnite Focused", fortFocused ? L"YES" : L"NO", fortFocused ? &colGood : &colBad);

        std::wstring profName = !g_allProfiles.empty() ? g_allProfiles[g_selectedProfileIdx].name : L"–";
        DrawRow(L"Profile", profName, &colVal);

        {
            std::wstring roiStr = L"–";
            if (!g_allProfiles.empty()) {
                auto& p = g_allProfiles[g_selectedProfileIdx];
                roiStr = L"x" + std::to_wstring(p.roi_x) + L" y" + std::to_wstring(p.roi_y) + L" " + std::to_wstring(p.roi_w) + L"×" + std::to_wstring(p.roi_h);
            }
            DrawRow(L"ROI", roiStr, &colVal);
        }

        DrawRow(L"HUD Position", L"x" + std::to_wstring(g_hudX) + L" y" + std::to_wstring(g_hudY), &colVal);
        DrawRow(L"Glide Threshold", FmtFloat(g_glideThreshold * 100.0f, 1) + L"%", &colVal);
        DrawRow(L"Freefall Threshold", FmtFloat(g_freefallThreshold * 100.0f, 1) + L"%", &colVal);
        DrawRow(L"Force Diving", g_forceDiving ? L"ON" : L"OFF", g_forceDiving ? &colWarn : &colVal);
        DrawRow(L"Force Detection", g_forceDetection ? L"ON" : L"OFF", g_forceDetection ? &colWarn : &colVal);
        DrawRow(L"Cursor Visible", g_isCursorVisible ? L"YES" : L"NO", &colVal);

        const wchar_t* selStr = (g_currentSelection == NONE) ? L"NONE" : (g_currentSelection == SELECTING_ROI) ? L"SELECTING ROI" : L"SELECTING COLOR";
        DrawRow(L"Selection State", selStr, g_currentSelection != NONE ? &colWarn : &colVal);
    }

    SolidBrush tinyBrush(Color(g_isDraggingHUD ? 130 : 55, 200, 210, 220));
    StringFormat sfBottom; sfBottom.SetAlignment(StringAlignmentCenter);
    graphics.DrawString(L"⠿ drag", -1, &Font_Tiny, RectF((REAL)rx, (REAL)ry + rh - 14, (REAL)rw, 14.0f), &sfBottom, &tinyBrush);

    BitBlt(hdc, 0, 0, sw, sh, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld); DeleteObject(hbmMem); DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
