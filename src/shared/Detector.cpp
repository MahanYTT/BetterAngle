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

#include <emmintrin.h> // SSE2 support

int FovDetector::Scan(const RoiConfig& cfg) {
    if (cfg.w <= 0 || cfg.h <= 0) return 0;
    
    EnsureResources(cfg.w, cfg.h);
    BitBlt(m_hdcMem, 0, 0, cfg.w, cfg.h, m_hdcScreen, cfg.x, cfg.y, SRCCOPY);

    int match = 0;
    int totalPixels = cfg.w * cfg.h;
    
    // Pro-Grade Tolerance Buffer: Multiply by 1.5 to handle dynamic game lighting
    int adjustedTol = (int)(cfg.tolerance * 1.5f);
    int tolSq = adjustedTol * adjustedTol;
    
    // Explicit COLORREF extraction (0x00BBGGRR)
    int tr = (int)(cfg.target & 0xFF);
    int tg = (int)((cfg.target >> 8) & 0xFF);
    int tb = (int)((cfg.target >> 16) & 0xFF);

    DWORD* p = (DWORD*)m_pixels;

    for (int i = 0; i < totalPixels; i++, p++) {
        DWORD pix = *p;

        // Explicit GDI Pixel extraction (BGRA -> 0xXXRRGGBB)
        int pr = (int)((pix >> 16) & 0xFF);
        int pg = (int)((pix >> 8) & 0xFF);
        int pb = (int)(pix & 0xFF);

        int dr = pr - tr;
        int dg = pg - tg;
        int db = pb - tb;

        if ((dr * dr + dg * dg + db * db) <= tolSq) {
            match++;
        }
    }

    return match;
}
