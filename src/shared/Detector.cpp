#include "shared/Detector.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdlib>

FovDetector::FovDetector() 
    : m_hdcScreen(NULL), m_hdcMem(NULL), m_hbm(NULL), m_hOld(NULL), m_curW(0), m_curH(0), m_pixels(NULL) {
    m_hdcScreen = GetDC(NULL);
}

FovDetector::~FovDetector() {
    if (m_hdcMem) {
        SelectObject(m_hdcMem, m_hOld);
        DeleteDC(m_hdcMem);
    }
    if (m_hbm) DeleteObject(m_hbm);
    ReleaseDC(NULL, m_hdcScreen);
}

void FovDetector::EnsureResources(int w, int h) {
    if (w == m_curW && h == m_curH && m_hdcMem) return;

    if (m_hdcMem) {
        SelectObject(m_hdcMem, m_hOld);
        DeleteDC(m_hdcMem);
        DeleteObject(m_hbm);
    }

    m_hdcMem = CreateCompatibleDC(m_hdcScreen);
    
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_hbm = CreateDIBSection(m_hdcScreen, &bmi, DIB_RGB_COLORS, &m_pixels, NULL, 0);
    m_hOld = SelectObject(m_hdcMem, m_hbm);
    m_curW = w;
    m_curH = h;
}

float FovDetector::Scan(const RoiConfig& cfg) {
    if (cfg.w <= 0 || cfg.h <= 0) return 0.0f;
    
    EnsureResources(cfg.w, cfg.h);
    BitBlt(m_hdcMem, 0, 0, cfg.w, cfg.h, m_hdcScreen, cfg.x, cfg.y, SRCCOPY);

    int match = 0;
    BYTE tr = GetRValue(cfg.target);
    BYTE tg = GetGValue(cfg.target);
    BYTE tb = GetBValue(cfg.target);

    DWORD* pPixels = (DWORD*)m_pixels;
    int totalPixels = cfg.w * cfg.h;
    
    int tolSq = cfg.tolerance * cfg.tolerance;
    
    for (int i = 0; i < totalPixels; i++) {
        DWORD p = pPixels[i];
        int r = p & 0xFF;
        int g = (p >> 8) & 0xFF;
        int b = (p >> 16) & 0xFF;

        int dr = r - (int)tr;
        int dg = g - (int)tg;
        int db = b - (int)tb;
        
        // Euclidean distance squared is faster and more accurate for matching
        if ((dr*dr + dg*dg + db*db) <= tolSq) {
            match++;
        }
    }

    return (float)match / (float)totalPixels;
}
