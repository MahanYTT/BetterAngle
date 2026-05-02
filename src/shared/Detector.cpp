#include "shared/Detector.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>


FovDetector::FovDetector()
    : m_hdcScreen(NULL), m_hdcMem(NULL), m_hbm(NULL), m_hOld(NULL), m_curW(0),
      m_curH(0), m_pixels(NULL) {}

FovDetector::~FovDetector() {
  if (m_hdcMem) {
    SelectObject(m_hdcMem, m_hOld);
    DeleteDC(m_hdcMem);
  }
  if (m_hbm)
    DeleteObject(m_hbm);
  if (m_hdcScreen)
    ReleaseDC(NULL, m_hdcScreen);
}

void FovDetector::EnsureScreenDC() {
  if (!m_hdcScreen) {
    m_hdcScreen = GetDC(NULL);
  }
}

void FovDetector::EnsureResources(int w, int h) {
  EnsureScreenDC();
  if (w == m_curW && h == m_curH && m_hdcMem)
    return;

  if (m_hdcMem) {
    SelectObject(m_hdcMem, m_hOld);
    DeleteDC(m_hdcMem);
    DeleteObject(m_hbm);
  }

  m_hdcMem = CreateCompatibleDC(m_hdcScreen);

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h; // Top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  m_hbm =
      CreateDIBSection(m_hdcScreen, &bmi, DIB_RGB_COLORS, &m_pixels, NULL, 0);
  m_hOld = SelectObject(m_hdcMem, m_hbm);
  m_curW = w;
  m_curH = h;
}

int FovDetector::Scan(const RoiConfig &cfg) {
  if (cfg.w <= 0 || cfg.h <= 0)
    return 0;

  EnsureScreenDC();
  EnsureResources(cfg.w, cfg.h);
  BitBlt(m_hdcMem, 0, 0, cfg.w, cfg.h, m_hdcScreen, cfg.x, cfg.y, SRCCOPY);

  int match = 0;
  int totalPixels = cfg.w * cfg.h;
  int tolSq = cfg.tolerance * cfg.tolerance;

  int tr = (int)GetRValue(cfg.target);
  int tg = (int)GetGValue(cfg.target);
  int tb = (int)GetBValue(cfg.target);

  DWORD *p = (DWORD *)m_pixels;

  int i = 0;
  for (; i <= totalPixels - 4; i += 4, p += 4) {
    // 4-pixel unrolled loop for better cache locality
    DWORD pix0 = p[0];
    DWORD pix1 = p[1];
    DWORD pix2 = p[2];
    DWORD pix3 = p[3];

#define PIX_MATCH_INT(pix)                                                     \
  {                                                                            \
    int r = (int)((pix >> 16) & 0xFF);                                         \
    int g = (int)((pix >> 8) & 0xFF);                                          \
    int b = (int)(pix & 0xFF);                                                 \
    int dr = r - tr;                                                           \
    int dg = g - tg;                                                           \
    int db = b - tb;                                                           \
    if ((dr * dr + dg * dg + db * db) <= tolSq)                                \
      match++;                                                                 \
  }

    PIX_MATCH_INT(pix0);
    PIX_MATCH_INT(pix1);
    PIX_MATCH_INT(pix2);
    PIX_MATCH_INT(pix3);
  }

  for (; i < totalPixels; i++, p++) {
    DWORD pix = *p;
    int r = (pix >> 16) & 0xFF;
    int g = (pix >> 8) & 0xFF;
    int b = pix & 0xFF;
    int dr = r - tr;
    int dg = g - tg;
    int db = b - tb;
    if ((dr * dr + dg * dg + db * db) <= tolSq)
      match++;
  }

  return match;
}
