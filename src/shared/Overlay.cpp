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
    static Font Font_Large(&ff, 54, FontStyleBold, UnitPixel); // Adjusted for Minimalist HUD
    static Font Font_Med(&ff, 28, FontStyleBold, UnitPixel);
    static Font Font_Small(&ff, 15, FontStyleRegular, UnitPixel);
    static Font Font_Label(&ff, 12, FontStyleBold, UnitPixel);
    static Font Font_LabelSmall(&ff, 10, FontStyleRegular, UnitPixel);
    static Font Font_Tiny(&ff, 9, FontStyleRegular, UnitPixel);
    static Font Font_DbgTitle(&ff, 11, FontStyleBold, UnitPixel);
    static Font Font_DbgKey(&ff, 10, FontStyleBold, UnitPixel);
    static Font Font_DbgVal(&ff, 10, FontStyleRegular, UnitPixel);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rect;
    GetClientRect(hwnd, &rect);
    int sw = rect.right  - rect.left;
    int sh = rect.bottom - rect.top;

    // === Double-buffered drawing ===========================================
    HDC      hdcMem = CreateCompatibleDC(hdc);
    HBITMAP  hbmMem = CreateCompatibleBitmap(hdc, sw, sh);
    HGDIOBJ  hOld   = SelectObject(hdcMem, hbmMem);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    graphics.Clear(Color(0, 0, 0, 0));

    // ── ROI selection snapshot background ────────────────────────────────
    if (g_screenSnapshot && g_currentSelection != NONE) {
        HDC     hdcSnap  = CreateCompatibleDC(hdcMem);
        HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
        BitBlt(hdcMem, 0, 0, sw, sh, hdcSnap, 0, 0, SRCCOPY);
        SelectObject(hdcSnap, hOldSnap);
        DeleteDC(hdcSnap);
    }

    // ── Two-stage selection overlay ───────────────────────────────────────
    if (g_currentSelection != NONE) {
        SolidBrush dimBrush(Color(120, 0, 0, 0));
        graphics.FillRectangle(&dimBrush, 0, 0, sw, sh);

        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush dimWhite  (Color(180, 220, 220, 220));

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
            int scopeSize = 210;
            int offset    = 24;
            int scopeX    = mouse.x + offset;
            int scopeY    = mouse.y + offset;
            if (scopeX + scopeSize > sw) scopeX = mouse.x - scopeSize - offset;
            if (scopeY + scopeSize > sh) scopeY = mouse.y - scopeSize - offset;

            graphics.DrawEllipse(&Pen(Color(80, 0, 0, 0), 6.0f), (REAL)scopeX - 1, (REAL)scopeY - 1, (REAL)scopeSize + 2, (REAL)scopeSize + 2);

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

            int centerX = scopeX + scopeSize / 2;
            int centerY = scopeY + scopeSize / 2;
            Pen xhairPen(Color(200, 255, 50, 50), 1.0f);
            graphics.DrawLine(&xhairPen, centerX - scopeSize/2, centerY, centerX + scopeSize/2, centerY);
            graphics.DrawLine(&xhairPen, centerX, centerY - scopeSize/2, centerX, centerY + scopeSize/2);

            graphics.ResetClip();
            graphics.FillEllipse(&SolidBrush(Color(255, 255, 50, 50)), centerX - 3, centerY - 3, 6, 6);

            Pen scopeRing(Color(220, 255, 255, 255), 2.5f);
            graphics.DrawEllipse(&scopeRing, scopeX, scopeY, scopeSize, scopeSize);
        }
    }

    // ── Live ROI box (always visible) ────────────────────────────────────
    if (g_showROIBox && g_selectionRect.right > g_selectionRect.left) {
        Color boxColor = g_isDiving ? Color(220, 255, 60, 60) : Color(220, 60, 230, 80);
        if (g_currentSelection != NONE) boxColor = Color(200, 255, 255, 255);
        Pen roiPen(boxColor, 1.5f);
        graphics.DrawRectangle(&roiPen,
            g_selectionRect.left, g_selectionRect.top,
            g_selectionRect.right  - g_selectionRect.left,
            g_selectionRect.bottom - g_selectionRect.top);
    }

    // ── Precision crosshair ───────────────────────────────────────────────
    if (showCrosshair) {
        float cx = sw / 2.0f + g_crossOffsetX;
        float cy = sh / 2.0f + g_crossOffsetY;

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

        float rad  = g_crossAngle * (3.1415926535f / 180.0f);
        float sinR = sinf(rad);
        float cosR = cosf(rad);
        float l    = (sw > sh ? sw : sh) * 3.0f;

        graphics.DrawLine(&crossPen, cx - sinR*l, cy + cosR*l, cx + sinR*l, cy - cosR*l);
        graphics.DrawLine(&crossPen, cx - cosR*l, cy - sinR*l, cx + cosR*l, cy + sinR*l);
    }

    // ══════════════════════════════════════════════════════════════════════
    // === Minimalist HUD (v4.20.42) =========================================
    // ══════════════════════════════════════════════════════════════════════
    const int rx = g_hudX, ry = g_hudY, rw = 160, rh = 80;
    const int RAD = 10;

    // Body
    GraphicsPath path;
    path.AddArc(rx,           ry,           RAD, RAD, 180, 90);
    path.AddArc(rx + rw - RAD, ry,           RAD, RAD, 270, 90);
    path.AddArc(rx + rw - RAD, ry + rh - RAD, RAD, RAD,   0, 90);
    path.AddArc(rx,           ry + rh - RAD, RAD, RAD,  90, 90);
    path.CloseFigure();

    SolidBrush bgBrush(Color(255, 0, 0, 0)); // Plain Solid Black
    graphics.FillPath(&bgBrush, &path);

    // Border
    Color borderCol = g_isDiving ? Color(255, 255, 255, 255) : Color(255, 50, 50, 50);
    Pen borderPen(borderCol, 1.0f);
    graphics.DrawPath(&borderPen, &path);

    // Center Angle Text
    std::wstring angleStr = FmtFloat(angle, 1) + L"°"; 

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    Color angleCol = g_isDiving ? Color(255, 255, 255, 255) : Color(255, 240, 240, 240);
    SolidBrush angleBrush(angleCol);
    
    RectF textRect((REAL)rx, (REAL)ry, (REAL)rw, (REAL)rh);
    graphics.DrawString(angleStr.c_str(), -1, &Font_Large, textRect, &sf, &angleBrush);

    // ══════════════════════════════════════════════════════════════════════
    // ── Debug Dashboard (Ctrl+9) ──────────────────────────────────────────
    // ══════════════════════════════════════════════════════════════════════
    if (g_debugMode) {
        const int DBG_ROWS = 14;
        const int ROW_H    = 18;
        int dw = 370, dh = 28 + DBG_ROWS * ROW_H + 10;
        int dx = rx, dy = ry + rh + 10;

        if (dy + dh > sh) dy = ry - dh - 8;

        GraphicsPath dbgPath;
        dbgPath.AddRectangle(Rect(dx, dy, dw, dh));
        LinearGradientBrush dbgBg(Point(dx, dy), Point(dx, dy + dh),
                                  Color(225, 8, 10, 14), Color(225, 3, 4, 6));
        graphics.FillPath(&dbgBg, &dbgPath);
        graphics.DrawPath(&Pen(Color(100, 0, 190, 255), 1.0f), &dbgPath);

        graphics.FillRectangle(&SolidBrush(Color(100, 40, 40, 40)), dx, dy, dw, 22);

        SolidBrush colTitle (Color(255, 255, 255, 255));
        SolidBrush colKey   (Color(255, 180, 180, 180));
        SolidBrush colVal   (Color(255, 220, 230, 240));
        SolidBrush colGood  (Color(255,  60, 230, 100));
        SolidBrush colBad   (Color(255, 255,  70,  70));
        SolidBrush colWarn  (Color(255, 255, 210,  50));

        graphics.DrawString(L"  DEBUG DASHBOARD", -1, &Font_DbgTitle,
                            PointF(float(dx + 4), float(dy + 5)), &colTitle);

        int row = 0;
        auto DrawRow = [&](const wchar_t* key, const std::wstring& val, SolidBrush* valBrush) {
            float y = float(dy + 28 + row * ROW_H);
            graphics.DrawString(key, -1, &Font_DbgKey, PointF(float(dx + 8),  y), &colKey);
            graphics.DrawString(val.c_str(), -1, &Font_DbgVal, PointF(float(dx + 175), y), valBrush);

            if (row > 0)
                graphics.DrawLine(&Pen(Color(20, 255, 255, 255), 1.0f),
                                  dx + 4, int(y) - 1, dx + dw - 4, int(y) - 1);
            row++;
        };

        bool fortFocused = IsFortniteFocused();

        DrawRow(L"FPS", FmtFloat(s_fps, 0), s_fps >= 60.0f ? &colGood : &colWarn);
        DrawRow(L"Angle (raw)", FmtFloat(angle, 2) + L"°", &colVal);
        DrawRow(L"Detection Ratio", FmtFloat(detectionRatio * 100.0, 0) + L"% / match " + std::to_wstring(matchPct) + L"%", matchPct > 5 ? &colGood : &colVal);
        DrawRow(L"Diving", g_isDiving ? L"YES" : L"NO", g_isDiving ? &colGood : &colVal);
        DrawRow(L"Fortnite Focused", fortFocused ? L"YES" : L"NO", fortFocused ? &colGood : &colBad);

        std::wstring profName = !g_allProfiles.empty() ? g_allProfiles[g_selectedProfileIdx].name : L"–";
        DrawRow(L"Profile", profName, &colVal);

        {
            std::wstring roiStr;
            if (!g_allProfiles.empty()) {
                auto& p = g_allProfiles[g_selectedProfileIdx];
                roiStr = L"x" + std::to_wstring(p.roi_x) +
                         L"  y" + std::to_wstring(p.roi_y) +
                         L"  " + std::to_wstring(p.roi_w) + L"×" + std::to_wstring(p.roi_h);
            } else { roiStr = L"–"; }
            DrawRow(L"ROI", roiStr, &colVal);
        }

        DrawRow(L"HUD Position", L"x" + std::to_wstring(g_hudX) + L"  y" + std::to_wstring(g_hudY), &colVal);
        DrawRow(L"Glide Threshold", FmtFloat(g_glideThreshold * 100.0f, 1) + L"%", &colVal);
        DrawRow(L"Freefall Threshold", FmtFloat(g_freefallThreshold * 100.0f, 1) + L"%", &colVal);
        DrawRow(L"Force Diving", g_forceDiving ? L"ON" : L"OFF", g_forceDiving ? &colWarn : &colVal);
        DrawRow(L"Force Detection", g_forceDetection ? L"ON" : L"OFF", g_forceDetection ? &colWarn : &colVal);
        DrawRow(L"Cursor Visible", g_isCursorVisible ? L"YES" : L"NO", &colVal);

        const wchar_t* selStr = (g_currentSelection == NONE) ? L"NONE"
                              : (g_currentSelection == SELECTING_ROI) ? L"SELECTING ROI"
                              : L"SELECTING COLOR";
        DrawRow(L"Selection State", selStr, g_currentSelection != NONE ? &colWarn : &colVal);
    }

    BitBlt(hdc, 0, 0, sw, sh, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
