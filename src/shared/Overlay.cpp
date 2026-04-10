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

        FontFamily selFF(L"Segoe UI");
        Font       selFont(&selFF, 28, FontStyleBold, UnitPixel);
        Font       selSub (&selFF, 15, FontStyleRegular, UnitPixel);

        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush dimWhite  (Color(180, 220, 220, 220));

        if (g_currentSelection == SELECTING_ROI) {
            graphics.DrawString(L"STAGE 1  ·  Drag to select the dive prompt area",
                                -1, &selFont, PointF(50.0f, 42.0f), &whiteBrush);
            graphics.DrawString(L"Press the hotkey again to cancel",
                                -1, &selSub,  PointF(52.0f, 80.0f), &dimWhite);

            // Live rubber-band rect while dragging
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
                                -1, &selFont, PointF(50.0f, 42.0f), &whiteBrush);
            graphics.DrawString(L"Hover over the brightest part of the prompt text",
                                -1, &selSub,  PointF(52.0f, 80.0f), &dimWhite);

            // Magnifier scope
            POINT mouse; GetCursorPos(&mouse);
            int scopeSize = 210;
            int offset    = 24;
            int scopeX    = mouse.x + offset;
            int scopeY    = mouse.y + offset;
            if (scopeX + scopeSize > sw) scopeX = mouse.x - scopeSize - offset;
            if (scopeY + scopeSize > sh) scopeY = mouse.y - scopeSize - offset;

            // Shadow ring
            graphics.DrawEllipse(&Pen(Color(80, 0, 0, 0), 6.0f), scopeX - 1, scopeY - 1, scopeSize + 2, scopeSize + 2);

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

            // Precision crosshair lines inside scope
            int centerX = scopeX + scopeSize / 2;
            int centerY = scopeY + scopeSize / 2;
            Pen xhairPen(Color(200, 255, 50, 50), 1.0f);
            graphics.DrawLine(&xhairPen, centerX - scopeSize/2, centerY, centerX + scopeSize/2, centerY);
            graphics.DrawLine(&xhairPen, centerX, centerY - scopeSize/2, centerX, centerY + scopeSize/2);

            // Red centre dot
            graphics.ResetClip();
            graphics.FillEllipse(&SolidBrush(Color(255, 255, 50, 50)), centerX - 3, centerY - 3, 6, 6);

            // Scope ring  
            Pen scopeRing(Color(220, 255, 255, 255), 2.5f);
            graphics.DrawEllipse(&scopeRing, scopeX, scopeY, scopeSize, scopeSize);
        }
    }

    // ── Live ROI box (always visible) ────────────────────────────────────
    if (g_showROIBox && g_selectionRect.right > g_selectionRect.left) {
        // Colour: green idle, red diving, white during selection
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
            // High-frequency pulse using QueryPerformanceCounter for accuracy
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
    // === Main Glass HUD ====================================================
    // ══════════════════════════════════════════════════════════════════════
    const int rx = g_hudX, ry = g_hudY, rw = 340, rh = 200;
    const int RAD = 18;

    // Outer glow  
    {
        Color glowCol = g_isDiving ? Color(60, 240, 240, 240) : Color(40, 120, 120, 120);
        SolidBrush glowBrush(glowCol);
        for (int g = 6; g >= 1; g--) {
            GraphicsPath gp;
            gp.AddArc(rx - g, ry - g, RAD, RAD, 180, 90);
            gp.AddArc(rx + rw - RAD + g, ry - g, RAD, RAD, 270, 90);
            gp.AddArc(rx + rw - RAD + g, ry + rh - RAD + g, RAD, RAD, 0, 90);
            gp.AddArc(rx - g, ry + rh - RAD + g, RAD, RAD, 90, 90);
            gp.CloseFigure();
            graphics.FillPath(&SolidBrush(Color(BYTE(30 - g * 4), 200, 200, 200)), &gp);
        }
    }

    // Panel body
    GraphicsPath path;
    path.AddArc(rx,           ry,           RAD, RAD, 180, 90);
    path.AddArc(rx + rw - RAD, ry,           RAD, RAD, 270, 90);
    path.AddArc(rx + rw - RAD, ry + rh - RAD, RAD, RAD,   0, 90);
    path.AddArc(rx,           ry + rh - RAD, RAD, RAD,  90, 90);
    path.CloseFigure();

    LinearGradientBrush bgBrush(Point(rx, ry), Point(rx, ry + rh),
                                Color(210, 12, 15, 20), Color(210, 4, 5, 8));
    graphics.FillPath(&bgBrush, &path);

    // Bright top highlight stripe
    LinearGradientBrush topStripe(Point(rx, ry), Point(rx, ry + 40),
                                  Color(30, 255, 255, 255), Color(0, 255, 255, 255));
    graphics.FillPath(&topStripe, &path);   // clips to rounded rect naturally

    // Border
    Color borderCol = g_isDiving ? Color(200, 255, 255, 255) : Color(100, 150, 150, 150);
    Pen borderPen(borderCol, 1.2f);
    graphics.DrawPath(&borderPen, &path);

    FontFamily ff(L"Segoe UI");

    // ── Label row ─────────────────────────────────────────────────────────
    Font     labelFont(&ff, 10, FontStyleRegular, UnitPixel);
    SolidBrush labelBrush(Color(180, 140, 155, 170));
    graphics.DrawString(L"CURRENT ANGLE", -1, &labelFont, PointF(float(rx + 18), float(ry + 14)), &labelBrush);

    // -- Angle text --------------------------------------------------------
    Font      angleFont(&ff, 68, FontStyleBold, UnitPixel);
    std::wstring    angleStr = FmtFloat(angle, 1) + L"°"; 
    // Colour: teal idle, cyan when diving
    Color angleCol = g_isDiving ? Color(255, 255, 255, 255) : Color(255, 210, 210, 210);
    SolidBrush angleBrush(angleCol);
    graphics.DrawString(angleStr.c_str(), -1, &angleFont, PointF(float(rx + 14), float(ry + 26)), &angleBrush);

    // -- Match % label -----------------------------------------------------
    Font subFont(&ff, 12, FontStyleBold, UnitPixel);
    int matchPct = int(detectionRatio * 100.0f);
    std::wstring matchStr = L"Match  " + std::to_wstring(matchPct) + L"%";
    Color matchLabelCol(200, 160, 170, 185);
    graphics.DrawString(matchStr.c_str(), -1, &subFont,
                        PointF(float(rx + 18), float(ry + rh - 56)), &SolidBrush(matchLabelCol));

    // ── Match progress bar ────────────────────────────────────────────────
    int barX = rx + 18, barY = ry + rh - 40, barW = rw - 36, barH = 9;
    // Track
    graphics.FillRectangle(&SolidBrush(Color(80, 255, 255, 255)), barX, barY, barW, barH);
    // Fill — colour-grade based on match level
    float clampedRatio = detectionRatio > 1.0f ? 1.0f : detectionRatio;
    int fillW = int(clampedRatio * barW);
    if (fillW > 0) {
        BYTE r = BYTE((1.0f - clampedRatio) * 255);
        BYTE g = BYTE(clampedRatio * 255);
        LinearGradientBrush barFill(Point(barX, barY), Point(barX + fillW, barY),
                                    Color(220, r, g, 40), Color(220, r / 2, g, 80));
        graphics.FillRectangle(&barFill, barX, barY, fillW, barH);
    }
    // Bar border
    graphics.DrawRectangle(&Pen(Color(50, 255, 255, 255), 1.0f), barX, barY, barW, barH);

    // ── Target colour swatch (anchored to HUD, top-right corner) ─────────
    int swatchX = rx + rw - 38, swatchY = ry + 14;
    Color swatch(255, GetBValue(g_targetColor), GetGValue(g_targetColor), GetRValue(g_targetColor));
    graphics.FillEllipse(&SolidBrush(swatch),        swatchX, swatchY, 18, 18);
    graphics.DrawEllipse(&Pen(Color(120, 220, 220, 220), 1.0f), swatchX, swatchY, 18, 18);

    // ── Drag hint ─────────────────────────────────────────────────────────
    Font tinyFont(&ff, 9, FontStyleRegular, UnitPixel);
    SolidBrush tinyBrush(Color(g_isDraggingHUD ? 130 : 55, 200, 210, 220));
    graphics.DrawString(L"⠿ drag", -1, &tinyFont,
                        PointF(float(rx + rw / 2 - 18), float(ry + rh - 14)), &tinyBrush);

    // ══════════════════════════════════════════════════════════════════════
    // ── Debug Dashboard (Ctrl+9) ──────────────────────────────────────────
    // ══════════════════════════════════════════════════════════════════════
    if (g_debugMode) {
        const int DBG_ROWS = 14;
        const int ROW_H    = 18;
        int dw = 370, dh = 28 + DBG_ROWS * ROW_H + 10;
        int dx = rx, dy = ry + rh + 10;

        // Clamp to screen bottom
        if (dy + dh > sh) dy = ry - dh - 8;

        // Background
        GraphicsPath dbgPath;
        dbgPath.AddRectangle(Rect(dx, dy, dw, dh));
        LinearGradientBrush dbgBg(Point(dx, dy), Point(dx, dy + dh),
                                  Color(225, 8, 10, 14), Color(225, 3, 4, 6));
        graphics.FillPath(&dbgBg, &dbgPath);
        graphics.DrawPath(&Pen(Color(100, 0, 190, 255), 1.0f), &dbgPath);

        // Title bar stripe
        graphics.FillRectangle(&SolidBrush(Color(100, 40, 40, 40)), dx, dy, dw, 22);

        Font dbgTitle(&ff, 11, FontStyleBold,    UnitPixel);
        Font dbgKey  (&ff, 10, FontStyleBold,    UnitPixel);
        Font dbgVal  (&ff, 10, FontStyleRegular, UnitPixel);

        SolidBrush colTitle (Color(255, 255, 255, 255));
        SolidBrush colKey   (Color(255, 180, 180, 180));   // neutral key
        SolidBrush colVal   (Color(255, 220, 230, 240));   // light value
        SolidBrush colGood  (Color(255,  60, 230, 100));   // green flag
        SolidBrush colBad   (Color(255, 255,  70,  70));   // red flag
        SolidBrush colWarn  (Color(255, 255, 210,  50));   // yellow warning

        graphics.DrawString(L"  DEBUG DASHBOARD", -1, &dbgTitle,
                            PointF(float(dx + 4), float(dy + 5)), &colTitle);

        // Helper lambda to draw a key=value row
        int row = 0;
        auto DrawRow = [&](const wchar_t* key, const std::wstring& val, SolidBrush* valBrush) {
            float y = float(dy + 28 + row * ROW_H);
            graphics.DrawString(key, -1, &dbgKey, PointF(float(dx + 8),  y), &colKey);
            graphics.DrawString(val.c_str(), -1, &dbgVal, PointF(float(dx + 175), y), valBrush);

            // Subtle separator
            if (row > 0)
                graphics.DrawLine(&Pen(Color(20, 255, 255, 255), 1.0f),
                                  dx + 4, int(y) - 1, dx + dw - 4, int(y) - 1);
            row++;
        };

        bool fortFocused = IsFortniteFocused();

        // ── Rows ──────────────────────────────────────────────────────────
        DrawRow(L"FPS",
                FmtFloat(s_fps, 0),
                s_fps >= 60.0f ? &colGood : &colWarn);

        DrawRow(L"Angle (raw)",
                FmtFloat(angle, 2) + L"°",
                &colVal);

        DrawRow(L"Detection Ratio",
                FmtFloat(detectionRatio * 100.0, 0) + L"% / match " + std::to_wstring(matchPct) + L"%",
                matchPct > 5 ? &colGood : &colVal);

        DrawRow(L"Diving",
                g_isDiving ? L"YES" : L"NO",
                g_isDiving ? &colGood : &colVal);

        DrawRow(L"Fortnite Focused",
                fortFocused ? L"YES" : L"NO",
                fortFocused ? &colGood : &colBad);

        // Profile name
        std::wstring profName = !g_allProfiles.empty() ? g_allProfiles[g_selectedProfileIdx].name : L"–";
        DrawRow(L"Profile", profName, &colVal);

        // ROI dimensions
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

        // HUD position
        DrawRow(L"HUD Position",
                L"x" + std::to_wstring(g_hudX) + L"  y" + std::to_wstring(g_hudY),
                &colVal);

        // Thresholds
        DrawRow(L"Glide Threshold",
                FmtFloat(g_glideThreshold * 100.0f, 1) + L"%",
                &colVal);
        DrawRow(L"Freefall Threshold",
                FmtFloat(g_freefallThreshold * 100.0f, 1) + L"%",
                &colVal);

        // Force flags
        DrawRow(L"Force Diving",
                g_forceDiving ? L"ON" : L"OFF",
                g_forceDiving ? &colWarn : &colVal);
        DrawRow(L"Force Detection",
                g_forceDetection ? L"ON" : L"OFF",
                g_forceDetection ? &colWarn : &colVal);

        // Cursor visible
        DrawRow(L"Cursor Visible",
                g_isCursorVisible ? L"YES" : L"NO",
                &colVal);

        // Selection state
        const wchar_t* selStr = (g_currentSelection == NONE) ? L"NONE"
                              : (g_currentSelection == SELECTING_ROI) ? L"SELECTING ROI"
                              : L"SELECTING COLOR";
        DrawRow(L"Selection State", selStr, g_currentSelection != NONE ? &colWarn : &colVal);
    }

    // ── Blit to screen ────────────────────────────────────────────────────
    BitBlt(hdc, 0, 0, sw, sh, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
}
