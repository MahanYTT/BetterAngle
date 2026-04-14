// Overlay.cpp - BetterAngle Pro
// All comments use ASCII only to avoid encoding issues across contributors.
#include "shared/Logic.h"
#include "shared/State.h"
#include <gdiplus.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>


using namespace Gdiplus;

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
  GetClientRect(hwnd, &rect);
  int sw = rect.right - rect.left;
  int sh = rect.bottom - rect.top;
  if (sw <= 0 || sh <= 0) return;

  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = sw;
  bmi.bmiHeader.biHeight = -sh; // top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void* pBits = nullptr;
  HBITMAP hbmMem = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
  HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);

  if (!pBits) {
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return;
  }

  // Pre-fill selection background if needed
  if (g_screenSnapshot && g_currentSelection != NONE) {
    HDC hdcSnap = CreateCompatibleDC(hdcMem);
    HGDIOBJ hOldSnap = SelectObject(hdcSnap, g_screenSnapshot);
    BitBlt(hdcMem, 0, 0, sw, sh, hdcSnap, 0, 0, SRCCOPY);
    SelectObject(hdcSnap, hOldSnap);
    DeleteDC(hdcSnap);

    // Force opaque alpha for the desktop snapshot so the window catches clicks
    DWORD* pixels = (DWORD*)pBits;
    int count = sw * sh;
    for (int i = 0; i < count; ++i) pixels[i] |= 0xFF000000;
  } else {
    // Clear to fully transparent
    memset(pBits, 0, sw * sh * 4);
  }

  Bitmap bmp(sw, sh, sw * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
  Graphics graphics(&bmp);
  graphics.SetSmoothingMode(SmoothingModeAntiAlias);
  graphics.SetInterpolationMode(InterpolationModeHighQuality);
  graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
  graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

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
            &dashPen, (int)g_selectionRect.left, (int)g_selectionRect.top,
            (int)(g_selectionRect.right - g_selectionRect.left),
            (int)(g_selectionRect.bottom - g_selectionRect.top));
      }

    } else if (g_currentSelection == SELECTING_COLOR) {
      graphics.DrawString(L"STAGE 2  \xB7  Click to pick the prompt colour", -1,
                          &selFont, PointF(50.0f, 42.0f), &whiteBrush);
      graphics.DrawString(L"Hover over the brightest part of the prompt text",
                          -1, &selSub, PointF(52.0f, 80.0f), &dimWhite);

      // Draw the selected ROI rectangle
      if (g_selectionRect.right > g_selectionRect.left) {
        Pen dashPen(Color(200, 255, 255, 255), 1.5f);
        REAL dash[] = {6.0f, 4.0f};
        dashPen.SetDashPattern(dash, 2);
        graphics.DrawRectangle(
            &dashPen, (int)g_selectionRect.left, (int)g_selectionRect.top,
            (int)(g_selectionRect.right - g_selectionRect.left),
            (int)(g_selectionRect.bottom - g_selectionRect.top));
      }

      // Live magnifier
      POINT curScr;
      GetCursorPos(&curScr);
      POINT cur = curScr;
      ScreenToClient(hwnd, &cur);

      int mx = curScr.x - 40, my = curScr.y - 40, mw = 80, mh = 80;
      HDC hdcScr = GetDC(NULL);
      HDC hdcZoom = CreateCompatibleDC(hdcMem);
      HBITMAP hbmZoom = CreateCompatibleBitmap(hdcMem, mw * 3, mh * 3);
      SelectObject(hdcZoom, hbmZoom);
      StretchBlt(hdcZoom, 0, 0, mw * 3, mh * 3, hdcScr, mx, my, mw, mh,
                 SRCCOPY);

      // Position magnifier relative to client cursor
      int zx =
          (cur.x + 20 + mw * 3 < sw) ? (cur.x + 20) : (cur.x - mw * 3 - 20);
      int zy = (cur.y + mh * 3 < sh) ? cur.y : (sh - mh * 3);

      // Draw magnifier content and border
      Bitmap zoomBmp(hbmZoom, NULL);
      graphics.DrawImage(&zoomBmp, zx, zy, mw * 3, mh * 3);
      Pen magBorder(Color(255, 255, 255, 255), 2.0f);
      graphics.DrawRectangle(&magBorder, zx, zy, mw * 3, mh * 3);

      // Magnifier center crosshair
      Pen magCross(Color(180, 255, 0, 0), 1.0f);
      graphics.DrawLine(&magCross, zx + (mw * 3 / 2), zy, zx + (mw * 3 / 2),
                        zy + (mh * 3));
      graphics.DrawLine(&magCross, zx, zy + (mh * 3 / 2), zx + (mw * 3),
                        zy + (mh * 3 / 2));

      // Precision dot at cursor tip
      SolidBrush dotBrush(Color(255, 255, 0, 0));
      graphics.FillEllipse(&dotBrush, (int)cur.x - 2, (int)cur.y - 2, 4, 4);
      Pen dotOuter(Color(255, 255, 255, 255), 1.0f);
      graphics.DrawEllipse(&dotOuter, (int)cur.x - 2, (int)cur.y - 2, 4, 4);

      DeleteObject(hbmZoom);
      DeleteDC(hdcZoom);
      ReleaseDC(NULL, hdcScr);
    }

    goto render_done;
  }

  {
  // ROI box visualizer
  if (g_showROIBox && !g_allProfiles.empty()) {
    auto &p = g_allProfiles[g_selectedProfileIdx];
    if (p.roi_w > 0 && p.roi_h > 0) {
      Color roiCol =
          g_isDiving ? Color(200, 255, 60, 60) : Color(200, 60, 220, 80);
      Pen roiPen(roiCol, 2.0f);
      REAL dash[] = {8.0f, 4.0f};
      roiPen.SetDashPattern(dash, 2);
      graphics.DrawRectangle(&roiPen, p.roi_x, p.roi_y, p.roi_w, p.roi_h);

      FontFamily roiFF(L"Segoe UI");
      Font roiFont(&roiFF, 10, FontStyleBold, UnitPixel);
      SolidBrush roiLabel(roiCol);
      graphics.DrawString(g_isDiving ? L"DIVING" : L"GLIDING", -1, &roiFont,
                          PointF(float(p.roi_x + 4), float(p.roi_y + 4)),
                          &roiLabel);
    }
  }

  // Crosshair
  if (showCrosshair) {
    float cx = sw * 0.5f + g_crossOffsetX;
    float cy = sh * 0.5f + g_crossOffsetY;
    // Make crosshair massive like the Java reference
    float hw = (sw > sh ? sw : sh) * 3.0f;
    float hh = hw;

    float pulse = 1.0f;
    if (g_crossPulse) {
      ULONGLONG t = GetTickCount64();
      pulse = 0.6f + 0.4f * sinf(float(t) * 0.005f);
    }

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
  }

  // HUD box
  int rw = 260, rh = 150;
  int rx = g_hudX, ry = g_hudY;

  // Background gradient (More transparent for sleekness)
  LinearGradientBrush bgBrush(Point(rx, ry), Point(rx, ry + rh),
                              Color(150, 6, 8, 12), Color(150, 2, 3, 5));
  GraphicsPath path;
  AddRoundedRect(path, rx, ry, rw, rh, 8); // 8px rounded corners
  graphics.FillPath(&bgBrush, &path);

  // Border: white when diving, subtle when not
  Color borderCol =
      g_isDiving ? Color(200, 255, 255, 255) : Color(90, 50, 65, 80);
  Pen borderPen(borderCol, 1.5f);
  graphics.DrawPath(&borderPen, &path);

  // "CURRENT ANGLE" label
  FontFamily ff(L"Segoe UI");
  Font labelFont(&ff, 9, FontStyleBold, UnitPixel);
  SolidBrush labelBrush(Color(160, 180, 185, 195));
  StringFormat fmtLabel;
  fmtLabel.SetAlignment(StringAlignmentCenter);
  graphics.DrawString(L"CURRENT ANGLE", -1, &labelFont,
                      RectF(float(rx), float(ry + 8), float(rw), 18.0f),
                      &fmtLabel, &labelBrush);

  // Angle text — L"\xB0" is the degree symbol (safe ASCII escape)
  Font angleFont(&ff, 68, FontStyleBold, UnitPixel);
  std::wstring angleStr = FmtFloat(std::abs(angle), 1) + L"\xB0";
  Color angleCol =
      g_isDiving ? Color(255, 0, 220, 255) : Color(255, 0, 210, 140);
  SolidBrush angleBrush(angleCol);

  StringFormat fmtAngle;
  fmtAngle.SetAlignment(StringAlignmentCenter);
  fmtAngle.SetLineAlignment(StringAlignmentNear);
  graphics.DrawString(angleStr.c_str(), -1, &angleFont,
                      RectF(float(rx), float(ry + 26), float(rw), 80.0f),
                      &fmtAngle, &angleBrush);

  // Match % label
  Font subFont(&ff, 12, FontStyleBold, UnitPixel);
  int matchPct = int(detectionRatio * 100.0f);
  std::wstring matchStr = L"Match  " + std::to_wstring(matchPct) + L"%";
  Color matchLabelCol(200, 160, 170, 185);
  SolidBrush matchLabelB(matchLabelCol);
  graphics.DrawString(matchStr.c_str(), -1, &subFont,
                      PointF(float(rx + 14), float(ry + rh - 54)),
                      &matchLabelB);

  // Match progress bar
  int barX = rx + 14, barY = ry + rh - 38, barW = rw - 28, barH = 8;
  SolidBrush barBgB(Color(60, 255, 255, 255));
  graphics.FillRectangle(&barBgB, barX, barY, barW, barH);
  float clampedRatio = detectionRatio > 1.0f ? 1.0f : detectionRatio;
  int fillW = int(clampedRatio * barW);
  if (fillW > 0) {
    BYTE r = BYTE((1.0f - clampedRatio) * 255);
    BYTE g = BYTE(clampedRatio * 255);
    LinearGradientBrush barFill(Point(barX, barY), Point(barX + fillW, barY),
                                Color(200, r, g, 40), Color(200, r / 2, g, 80));
    graphics.FillRectangle(&barFill, barX, barY, fillW, barH);
  }
  Pen barPen(Color(40, 255, 255, 255), 1.0f);
  graphics.DrawRectangle(&barPen, barX, barY, barW, barH);

  // Target colour swatch (top-right corner)
  int swatchX = rx + rw - 28, swatchY = ry + 8;
  Color swatch(255, GetBValue(g_targetColor), GetGValue(g_targetColor),
               GetRValue(g_targetColor));
  SolidBrush swatchB(swatch);
  graphics.FillEllipse(&swatchB, swatchX, swatchY, 16, 16);
  Pen swatchP(Color(100, 220, 220, 220), 1.0f);
  graphics.DrawEllipse(&swatchP, swatchX, swatchY, 16, 16);

  // Drag hint (plain ASCII — no special characters to avoid rendering as
  // squares)
  Font tinyFont(&ff, 9, FontStyleRegular, UnitPixel);
  SolidBrush tinyBrush(Color(g_isDraggingHUD ? 130 : 50, 200, 210, 220));
  StringFormat sfCenter;
  sfCenter.SetAlignment(StringAlignmentCenter);
  graphics.DrawString(L":: drag", -1, &tinyFont,
                      RectF(float(rx), float(ry + rh - 14), float(rw), 12.0f),
                      &sfCenter, &tinyBrush);

  // =========================================================================
  // Modern Debug Dashboard (Ctrl+9)
  // =========================================================================
  if (g_debugMode) {
    // 1. Calculate height dynamically based on active rows
    std::vector<std::pair<std::wstring, std::pair<std::wstring, SolidBrush *>>>
        rows;

    SolidBrush colTitle(Color(255, 255, 255, 255));
    SolidBrush colKey(Color(255, 140, 200, 255));
    SolidBrush colVal(Color(255, 220, 230, 240));
    SolidBrush colGood(Color(255, 80, 240, 120));
    SolidBrush colBad(Color(255, 255, 80, 80));
    SolidBrush colWarn(Color(255, 255, 220, 60));

    rows.push_back(
        {L"FPS", {FmtFloat(s_fps, 0), s_fps >= 60.0f ? &colGood : &colWarn}});
    rows.push_back({L"Angle (raw)", {FmtFloat(angle, 4) + L"\xB0", &colVal}});
    rows.push_back({L"Detection Ratio",
                    {FmtFloat(detectionRatio * 100.0, 1) + L"%",
                     matchPct > 5 ? &colGood : &colVal}});
    rows.push_back({L"Diving State",
                    {g_isDiving ? L"ACTIVE" : L"INACTIVE",
                     g_isDiving ? &colGood : &colVal}});

    std::wstring profName = !g_allProfiles.empty()
                                ? g_allProfiles[g_selectedProfileIdx].name
                                : L"-";
    rows.push_back({L"Active Profile", {profName, &colVal}});

    if (!g_allProfiles.empty()) {
      auto &p = g_allProfiles[g_selectedProfileIdx];
      std::wstring roiStr =
          L"[" + std::to_wstring(p.roi_x) + L"," + std::to_wstring(p.roi_y) +
          L"] " + std::to_wstring(p.roi_w) + L"x" + std::to_wstring(p.roi_h);
      rows.push_back({L"ROI Bounds", {roiStr, &colVal}});
    }

    rows.push_back(
        {L"HUD Position",
         {L"x" + std::to_wstring(g_hudX) + L" y" + std::to_wstring(g_hudY),
          &colVal}});
    rows.push_back({L"Glide Threshold",
                    {FmtFloat(g_glideThreshold * 100.0f, 1) + L"%", &colVal}});
    rows.push_back(
        {L"Freefall Threshold",
         {FmtFloat(g_freefallThreshold * 100.0f, 1) + L"%", &colVal}});
    rows.push_back(
        {L"Force Diving",
         {g_forceDiving ? L"ON" : L"OFF", g_forceDiving ? &colWarn : &colVal}});
    rows.push_back({L"Force Detection",
                    {g_forceDetection ? L"ON" : L"OFF",
                     g_forceDetection ? &colWarn : &colVal}});
    rows.push_back({L"System Cursor",
                    {g_isCursorVisible ? L"VISIBLE" : L"HIDDEN", &colVal}});

    const wchar_t *selStr = (g_currentSelection == NONE) ? L"IDLE"
                            : (g_currentSelection == SELECTING_ROI)
                                ? L"ROI SELECT"
                                : L"COLOR SELECT";
    rows.push_back({L"Interaction State",
                    {selStr, g_currentSelection != NONE ? &colWarn : &colVal}});

    // 2. Render the glass panel
    const int ROW_H = 20;
    int dw = 380, dh = 30 + (int)rows.size() * ROW_H + 8;
    int dx = rx, dy = ry + rh + 12;
    if (dy + dh > sh)
      dy = ry - dh - 12;

    GraphicsPath dbgPath;
    AddRoundedRect(dbgPath, dx, dy, dw, dh, 8);

    // Premium "Glass" Background
    LinearGradientBrush dbgBg(Point(dx, dy), Point(dx, dy + dh),
                               Color(180, 10, 12, 18), Color(180, 5, 6, 10));
    graphics.FillPath(&dbgBg, &dbgPath);

    // Inner Glow Border
    Pen dbgEdge(Color(60, 255, 255, 255), 1.0f);
    graphics.DrawPath(&dbgEdge, &dbgPath);

    // Header
    SolidBrush headerBg(Color(100, 20, 30, 50));
    graphics.FillRectangle(&headerBg, dx + 1, dy + 1, dw - 2, 26);
    Pen headerLine(Color(80, 0, 200, 255), 1.5f);
    graphics.DrawLine(&headerLine, dx + 8, dy + 27, dx + dw - 8, dy + 27);

    Font dbgTitle(&ff, 10, FontStyleBold, UnitPixel);
    StringFormat sfMid;
    sfMid.SetAlignment(StringAlignmentNear);
    sfMid.SetLineAlignment(StringAlignmentCenter);
    graphics.DrawString(L"  APP METRICS & DIAGNOSTICS", -1, &dbgTitle,
                        RectF((float)dx, (float)dy, (float)dw, 28.0f), &sfMid,
                        &colTitle);

    // 3. Render rows
    Font dbgKey(&ff, 10, FontStyleBold, UnitPixel);
    Font dbgVal(&ff, 10, FontStyleRegular, UnitPixel);

    for (int i = 0; i < (int)rows.size(); ++i) {
      float yPos = (float)(dy + 32 + i * ROW_H);

      // Key
      graphics.DrawString(rows[i].first.c_str(), -1, &dbgKey,
                          PointF((float)dx + 12, yPos), &colKey);

      // Value
      graphics.DrawString(rows[i].second.first.c_str(), -1, &dbgVal,
                          PointF((float)dx + 185, yPos), rows[i].second.second);

      // Subtle Divider
      if (i < (int)rows.size() - 1) {
        Pen divPen(Color(15, 255, 255, 255), 1.0f);
        graphics.DrawLine(&divPen, (float)dx + 10, yPos + ROW_H - 2,
                          (float)dx + dw - 10, yPos + ROW_H - 2);
      }

      // LED indicator for states
      if (rows[i].first == L"Diving State" ||
          rows[i].first == L"Force Diving") {
        SolidBrush *ledCol =
            rows[i].second.first.find(L"ACTIVE") != std::wstring::npos ||
                    rows[i].second.first == L"ON"
                ? &colGood
                : &colBad;
        graphics.FillEllipse(ledCol, dx + dw - 20, (int)yPos + 4, 6, 6);
      }
    }
  }
  }

render_done:
  POINT ptSrc = {0, 0};
  RECT wRect;
  GetWindowRect(hwnd, &wRect);
  POINT ptWin = {wRect.left, wRect.top};
  SIZE size = {sw, sh};
  BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  
  UpdateLayeredWindow(hwnd, hdcScreen, &ptWin, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

  SelectObject(hdcMem, hOld);
  DeleteObject(hbmMem);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdcScreen);
}
