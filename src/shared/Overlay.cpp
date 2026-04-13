// Overlay.cpp - BetterAngle Pro
// All comments use ASCII only to avoid encoding issues across contributors.
#include "shared/Logic.h"
#include "shared/State.h"

// Windows headers must come before gdiplus.h for GDI+ compatibility
#include <objidl.h>
#include <windows.h>

// Define GDIPLUS_OLDEST_SUPPORTED_VERSION for Windows SDK 10.0.26100.0
// compatibility
#ifndef GDIPLUS_OLDEST_SUPPORTED_VERSION
#define GDIPLUS_OLDEST_SUPPORTED_VERSION 0x0110
#endif
#include <climits>
#include <gdiplus.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>


using namespace Gdiplus;

bool IsFortniteFocused();
void LogStartup(const std::string &msg);

void AddRoundedRect(GraphicsPath &path, int x, int y, int width, int height,
                    int radius) {
  int d = radius * 2;
  path.AddArc(x, y, d, d, 180, 90);
  path.AddArc(x + width - d, y, d, d, 270, 90);
  path.AddArc(x + width - d, y + height - d, d, d, 0, 90);
  path.AddArc(x, y + height - d, d, d, 90, 90);
  path.CloseFigure();
}

// Helper: format float to N decimal places
static std::wstring FmtFloat(double v, int decimals = 2) {
  std::wostringstream ss;
  ss << std::fixed << std::setprecision(decimals) << v;
  return ss.str();
}

// Static FPS tracking
static ULONGLONG s_lastFrameTime = 0;
static float s_fps = 0.0f;
static int s_frameCount = 0;
static ULONGLONG s_fpsTimer = 0;

static void TickFPS() {
  ULONGLONG now = GetTickCount64();
  s_frameCount++;
  if (now - s_fpsTimer >= 500) {
    s_fps = s_frameCount * 1000.0f / float(now - s_fpsTimer);
    s_frameCount = 0;
    s_fpsTimer = now;
  }
}

void DrawOverlay(HWND hwnd, double angle, float detectionRatio,
                 bool showCrosshair) {
  TickFPS();

  RECT rect;
  GetWindowRect(hwnd, &rect);
  int sw = rect.right - rect.left;
  int sh = rect.bottom - rect.top;
  if (sw <= 0 || sh <= 0)
    return;

  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);

  // Create a 32-bit DIB Section for per-pixel alpha
  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = sw;
  bmi.bmiHeader.biHeight = -sh; // Top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void *pBits = nullptr;
  HBITMAP hbmMem =
      CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
  HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);

  Graphics graphics(hdcMem);
  graphics.SetSmoothingMode(SmoothingModeAntiAlias);
  graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
  graphics.Clear(Color(0, 0, 0, 0)); // Pure transparent

  // ROI selection snapshot background
  if (g_screenSnapshot && g_currentSelection != NONE) {
    HDC hdcSnap = CreateCompatibleDC(hdcMem);
    HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
    BitBlt(hdcMem, 0, 0, sw, sh, hdcSnap, 0, 0, SRCCOPY);
    SelectObject(hdcSnap, hOldSnap);
    DeleteDC(hdcSnap);
  }

  // Two-stage selection overlay
  if (g_currentSelection != NONE) {
    SolidBrush dimBrush(Color(120, 0, 0, 0));
    graphics.FillRectangle(&dimBrush, 0, 0, sw, sh);

    FontFamily selFF(L"Segoe UI");
    Font selFont(&selFF, 28, FontStyleBold, UnitPixel);
    Font selSub(&selFF, 15, FontStyleRegular, UnitPixel);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    SolidBrush dimWhite(Color(180, 220, 220, 220));

    if (g_currentSelection == SELECTING_ROI) {
      graphics.DrawString(L"STAGE 1  \xB7  Drag to select the dive prompt area",
                          -1, &selFont, PointF(50.0f, 42.0f), &whiteBrush);
      graphics.DrawString(L"Press the hotkey again to cancel", -1, &selSub,
                          PointF(52.0f, 80.0f), &dimWhite);

      if (g_selectionRect.right > g_selectionRect.left) {
        Pen dashPen(Color(200, 255, 255, 255), 1.5f);
        REAL dash[] = {6.0f, 4.0f};
        dashPen.SetDashPattern(dash, 2);
        graphics.DrawRectangle(
            &dashPen, (int)g_selectionRect.left - g_virtScreenX,
            (int)g_selectionRect.top - g_virtScreenY,
            (int)(g_selectionRect.right - g_selectionRect.left),
            (int)(g_selectionRect.bottom - g_selectionRect.top));
      }

    } else if (g_currentSelection == SELECTING_COLOR) {
      graphics.DrawString(L"STAGE 2  \xB7  Click to pick the prompt colour", -1,
                          &selFont, PointF(50.0f, 42.0f), &whiteBrush);
      graphics.DrawString(L"Hover over the brightest part of the prompt text",
                          -1, &selSub, PointF(52.0f, 80.0f), &dimWhite);

      // Live magnifier
      POINT curScr;
      GetCursorPos(&curScr);
      int rx = curScr.x - g_virtScreenX;
      int ry = curScr.y - g_virtScreenY;

      int mx = curScr.x - 40, my = curScr.y - 40, mw = 80, mh = 80;
      HDC hdcZoom = CreateCompatibleDC(hdcMem);
      HBITMAP hbmZoom = CreateCompatibleBitmap(hdcScreen, mw * 3, mh * 3);
      SelectObject(hdcZoom, hbmZoom);
      StretchBlt(hdcZoom, 0, 0, mw * 3, mh * 3, hdcScreen, mx, my, mw, mh,
                 SRCCOPY);

      int zx = (rx + 20 + mw * 3 < sw) ? (rx + 20) : (rx - mw * 3 - 20);
      int zy = (ry + mh * 3 < sh) ? ry : (sh - mh * 3);

      BitBlt(hdcMem, zx, zy, mw * 3, mh * 3, hdcZoom, 0, 0, SRCCOPY);
      Pen magBorder(Color(255, 255, 255, 255), 2.0f);
      graphics.DrawRectangle(&magBorder, zx, zy, mw * 3, mh * 3);

      Pen magCross(Color(180, 255, 0, 0), 1.0f);
      graphics.DrawLine(&magCross, zx + (mw * 3 / 2), zy, zx + (mw * 3 / 2),
                        zy + (mh * 3));
      graphics.DrawLine(&magCross, zx, zy + (mh * 3 / 2), zx + (mw * 3),
                        zy + (mh * 3 / 2));

      SolidBrush dotBrush(Color(255, 255, 0, 0));
      graphics.FillEllipse(&dotBrush, (int)rx - 2, (int)ry - 2, 4, 4);
      Pen dotOuter(Color(255, 255, 255, 255), 1.0f);
      graphics.DrawEllipse(&dotOuter, (int)rx - 2, (int)ry - 2, 4, 4);

      DeleteObject(hbmZoom);
      DeleteDC(hdcZoom);
    }

    // Apply with UpdateLayeredWindow
    POINT ptSrc = {0, 0};
    POINT ptDst = {rect.left, rect.top};
    SIZE size = {sw, sh};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0,
                        &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return;
  }

  // ROI box visualizer
  if (g_showROIBox && !g_allProfiles.empty()) {
    auto &p = g_allProfiles[g_selectedProfileIdx];
    if (p.roi_w > 0 && p.roi_h > 0) {
      Color roiCol =
          g_isDiving ? Color(200, 255, 60, 60) : Color(200, 60, 220, 80);
      Pen roiPen(roiCol, 2.0f);
      REAL dash[] = {8.0f, 4.0f};
      roiPen.SetDashPattern(dash, 2);
      graphics.DrawRectangle(&roiPen, p.roi_x - g_virtScreenX,
                             p.roi_y - g_virtScreenY, p.roi_w, p.roi_h);
      FontFamily roiFF(L"Segoe UI");
      Font roiFont(&roiFF, 10, FontStyleBold, UnitPixel);
      SolidBrush roiLabel(roiCol);
      graphics.DrawString(g_isDiving ? L"DIVING" : L"GLIDING", -1, &roiFont,
                          PointF(float(p.roi_x + 4 - g_virtScreenX),
                                 float(p.roi_y + 4 - g_virtScreenY)),
                          &roiLabel);
    }
  }

  // Crosshair
  if (showCrosshair) {
    int primW = GetSystemMetrics(SM_CXSCREEN),
        primH = GetSystemMetrics(SM_CYSCREEN);
    const int originX = rect.left;
    const int originY = rect.top;
    float cx = (primW * 0.5f) - originX + g_crossOffsetX;
    float cy = (primH * 0.5f) - originY + g_crossOffsetY;
    float hw = (primW > primH ? primW : primH) * 2.0f, hh = hw;
    float pulse = 1.0f;
    if (g_crossPulse)
      pulse = 0.6f + 0.4f * sinf(float(GetTickCount64()) * 0.005f);
    BYTE alpha = BYTE(200 * pulse);
    COLORREF cc = g_crossColor;
    Pen cPen(Color(alpha, GetRValue(cc), GetGValue(cc), GetBValue(cc)),
             g_crossThickness);
    Matrix rot;
    rot.RotateAt(g_crossAngle, PointF(cx, cy));
    graphics.SetTransform(&rot);
    graphics.DrawLine(&cPen, cx - hw, cy, cx + hw, cy);
    graphics.DrawLine(&cPen, cx, cy - hh, cx, cy + hh);
    graphics.ResetTransform();

    static int s_lastOriginX = INT_MIN;
    static int s_lastOriginY = INT_MIN;
    static int s_lastCenterX = INT_MIN;
    static int s_lastCenterY = INT_MIN;
    const int drawX = int(cx);
    const int drawY = int(cy);
    if (s_lastOriginX != originX || s_lastOriginY != originY ||
        s_lastCenterX != drawX || s_lastCenterY != drawY) {
      std::ostringstream oss;
      oss << "CrosshairRender: origin=(" << originX << "," << originY
          << ") draw=(" << drawX << "," << drawY << ") offset=("
          << g_crossOffsetX << "," << g_crossOffsetY << ")";
      LogStartup(oss.str());
      s_lastOriginX = originX;
      s_lastOriginY = originY;
      s_lastCenterX = drawX;
      s_lastCenterY = drawY;
    }
  }

  // HUD box
  int rw = 260, rh = 150, rx = g_hudX, ry = g_hudY;
  LinearGradientBrush bgBrush(Point(rx, ry), Point(rx, ry + rh),
                              Color(150, 6, 8, 12), Color(150, 2, 3, 5));
  GraphicsPath path;
  AddRoundedRect(path, rx, ry, rw, rh, 8);
  graphics.FillPath(&bgBrush, &path);
  Color borderCol =
      g_isDiving ? Color(200, 255, 255, 255) : Color(90, 50, 65, 80);
  Pen borderPen(borderCol, 1.5f);
  graphics.DrawPath(&borderPen, &path);

  FontFamily ff(L"Segoe UI");
  Font labelFont(&ff, 9, FontStyleBold, UnitPixel);
  SolidBrush labelBrush(Color(160, 180, 185, 195));
  StringFormat fmtLabel;
  fmtLabel.SetAlignment(StringAlignmentCenter);
  graphics.DrawString(L"CURRENT ANGLE", -1, &labelFont,
                      RectF(float(rx), float(ry + 8), float(rw), 18.0f),
                      &fmtLabel, &labelBrush);

  Font angleFont(&ff, 58, FontStyleBold, UnitPixel);
  std::wstring angleStr = FmtFloat(std::abs(angle), 2) + L"\xB0";
  Color angleCol =
      g_isDiving ? Color(255, 0, 255, 220) : Color(255, 0, 210, 140);
  SolidBrush angleBrush(angleCol);
  StringFormat fmtAngle;
  fmtAngle.SetAlignment(StringAlignmentCenter);
  fmtAngle.SetLineAlignment(StringAlignmentNear);
  graphics.DrawString(angleStr.c_str(), -1, &angleFont,
                      RectF(float(rx), float(ry + 28), float(rw), 80.0f),
                      &fmtAngle, &angleBrush);

  Font subFont(&ff, 12, FontStyleBold, UnitPixel);
  int matchPct = int(detectionRatio * 100.0f);
  std::wstring matchStr = L"Match  " + std::to_wstring(matchPct) + L"%";
  SolidBrush matchLabelB(Color(200, 160, 170, 185));
  graphics.DrawString(matchStr.c_str(), -1, &subFont,
                      PointF(float(rx + 14), float(ry + rh - 54)),
                      &matchLabelB);

  int barX = rx + 14, barY = ry + rh - 38, barW = rw - 28, barH = 8;
  SolidBrush barBgB(Color(60, 255, 255, 255));
  graphics.FillRectangle(&barBgB, barX, barY, barW, barH);
  float clampedRatio = detectionRatio > 1.0f ? 1.0f : detectionRatio;
  int fillW = int(clampedRatio * barW);
  if (fillW > 0) {
    BYTE r = BYTE((1.0f - clampedRatio) * 255), g = BYTE(clampedRatio * 255);
    LinearGradientBrush barFill(Point(barX, barY), Point(barX + fillW, barY),
                                Color(200, r, g, 40), Color(200, r / 2, g, 80));
    graphics.FillRectangle(&barFill, barX, barY, fillW, barH);
  }
  Pen barPen(Color(40, 255, 255, 255), 1.0f);
  graphics.DrawRectangle(&barPen, barX, barY, barW, barH);

  int swatchX = rx + rw - 28, swatchY = ry + 8;
  SolidBrush swatchB(Color(255, GetBValue(g_targetColor),
                           GetGValue(g_targetColor), GetRValue(g_targetColor)));
  graphics.FillEllipse(&swatchB, swatchX, swatchY, 16, 16);
  Pen swatchP(Color(100, 220, 220, 220), 1.0f);
  graphics.DrawEllipse(&swatchP, swatchX, swatchY, 16, 16);

  Font tinyFont(&ff, 9, FontStyleRegular, UnitPixel);
  SolidBrush tinyBrush(Color(g_isDraggingHUD ? 130 : 50, 200, 210, 220));
  StringFormat sfCenter;
  sfCenter.SetAlignment(StringAlignmentCenter);
  graphics.DrawString(L":: drag", -1, &tinyFont,
                      RectF(float(rx), float(ry + rh - 14), float(rw), 12.0f),
                      &sfCenter, &tinyBrush);

  if (g_debugMode) {
    const int DBG_ROWS = 14, ROW_H = 18;
    int dw = 370, dh = 28 + DBG_ROWS * ROW_H + 10, dx = rx, dy = ry + rh + 10;
    if (dy + dh > sh)
      dy = ry - dh - 8;
    GraphicsPath dbgPath;
    AddRoundedRect(dbgPath, dx, dy, dw, dh, 6);
    LinearGradientBrush dbgBg(Point(dx, dy), Point(dx, dy + dh),
                              Color(160, 8, 10, 14), Color(160, 3, 4, 6));
    graphics.FillPath(&dbgBg, &dbgPath);
    Pen dbgP(Color(80, 0, 190, 255), 1.0f);
    graphics.DrawPath(&dbgP, &dbgPath);
    SolidBrush headerBgB(Color(60, 0, 160, 255));
    graphics.FillRectangle(&headerBgB, dx, dy, dw, 22);
    Font dbgTitle(&ff, 11, FontStyleBold, UnitPixel),
        dbgKey(&ff, 10, FontStyleBold, UnitPixel),
        dbgVal(&ff, 10, FontStyleRegular, UnitPixel);
    SolidBrush colTitle(Color(255, 255, 255, 255)),
        colKey(Color(255, 120, 180, 255)), colVal(Color(255, 220, 230, 240)),
        colGood(Color(255, 60, 230, 100)), colBad(Color(255, 255, 70, 70)),
        colWarn(Color(255, 255, 210, 50));
    graphics.DrawString(L"  DEBUG DASHBOARD", -1, &dbgTitle,
                        PointF(float(dx + 4), float(dy + 5)), &colTitle);

    int row = 0;
    auto DrawRow = [&](const wchar_t *key, const std::wstring &val,
                       SolidBrush *valBrush) {
      float rowY = float(dy + 28 + row * ROW_H);
      graphics.DrawString(key, -1, &dbgKey, PointF(float(dx + 8), rowY),
                          &colKey);
      graphics.DrawString(val.c_str(), -1, &dbgVal,
                          PointF(float(dx + 175), rowY), valBrush);
      if (row > 0) {
        Pen lnP(Color(20, 255, 255, 255), 1.0f);
        graphics.DrawLine(&lnP, dx + 4, int(rowY) - 1, dx + dw - 4,
                          int(rowY) - 1);
      }
      row++;
    };
    bool fortFocused = IsFortniteFocused();
    DrawRow(L"FPS", FmtFloat(s_fps, 0), s_fps >= 60.0f ? &colGood : &colWarn);
    DrawRow(L"Angle (raw)", FmtFloat(angle, 4) + L"\xB0", &colVal);
    DrawRow(L"Detection Ratio",
            FmtFloat(detectionRatio * 100.0, 1) + L"% / match " +
                std::to_wstring(matchPct) + L"%",
            matchPct > 5 ? &colGood : &colVal);
    DrawRow(L"Diving", g_isDiving ? L"YES" : L"NO",
            g_isDiving ? &colGood : &colVal);
    DrawRow(L"Fortnite Focused", fortFocused ? L"YES" : L"NO",
            fortFocused ? &colGood : &colBad);
    std::wstring profName = !g_allProfiles.empty()
                                ? g_allProfiles[g_selectedProfileIdx].name
                                : L"-";
    DrawRow(L"Profile", profName, &colVal);
    std::wstring roiStr =
        !g_allProfiles.empty()
            ? (L"x" +
               std::to_wstring(g_allProfiles[g_selectedProfileIdx].roi_x) +
               L" y" +
               std::to_wstring(g_allProfiles[g_selectedProfileIdx].roi_y) +
               L" " +
               std::to_wstring(g_allProfiles[g_selectedProfileIdx].roi_w) +
               L"x" +
               std::to_wstring(g_allProfiles[g_selectedProfileIdx].roi_h))
            : L"-";
    DrawRow(L"ROI", roiStr, &colVal);
    DrawRow(L"HUD Position",
            L"x" + std::to_wstring(g_hudX) + L" y" + std::to_wstring(g_hudY),
            &colVal);
    DrawRow(L"Glide Threshold", FmtFloat(g_glideThreshold * 100.0f, 1) + L"%",
            &colVal);
    DrawRow(L"Freefall Threshold",
            FmtFloat(g_freefallThreshold * 100.0f, 1) + L"%", &colVal);
    DrawRow(L"Force Diving", g_forceDiving ? L"ON" : L"OFF",
            g_forceDiving ? &colWarn : &colVal);
    DrawRow(L"Force Detection", g_forceDetection ? L"ON" : L"OFF",
            g_forceDetection ? &colWarn : &colVal);
    DrawRow(L"Cursor Visible", g_isCursorVisible ? L"YES" : L"NO", &colVal);
    const wchar_t *selStr =
        (g_currentSelection == NONE)
            ? L"NONE"
            : (g_currentSelection == SELECTING_ROI ? L"SELECTING ROI"
                                                   : L"SELECTING COLOR");
    DrawRow(L"Selection State", selStr,
            g_currentSelection != NONE ? &colWarn : &colVal);
  }

  // Apply with UpdateLayeredWindow
  POINT ptSrc = {0, 0};
  POINT ptDst = {rect.left, rect.top};
  SIZE size = {sw, sh};
  BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend,
                      ULW_ALPHA);

  SelectObject(hdcMem, hOld);
  DeleteObject(hbmMem);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdcScreen);
}
