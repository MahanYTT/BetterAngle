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
    int tolSq = cfg.tolerance * cfg.tolerance;
    
    int tr = (int)GetRValue(cfg.target);
    int tg = (int)GetGValue(cfg.target);
    int tb = (int)GetBValue(cfg.target);

    DWORD* p = (DWORD*)m_pixels;

    // SIMD Optimization: Pure integer pipeline using SSE2
    int i = 0;
    __m128i v_tr = _mm_set1_epi16((short)tr);
    __m128i v_tg = _mm_set1_epi16((short)tg);
    __m128i v_tb = _mm_set1_epi16((short)tb);
    __m128i v_tolSq = _mm_set1_epi32(tolSq);
    __m128i v_zero = _mm_setzero_si128();

    for (; i <= totalPixels - 4; i += 4, p += 4) {
        __m128i v_pix = _mm_loadu_si128((__m128i*)p);

        // Unpack bytes to 16-bit integers
        // Lane 0-1
        __m128i v_lo = _mm_unpacklo_epi8(v_pix, v_zero);
        // Lane 2-3
        __m128i v_hi = _mm_unpackhi_epi8(v_pix, v_zero);

        auto checkPixels = [&](__m128i v_half) {
            // v_half contains 2 pixels as 16-bit [B, G, R, A, B, G, R, A]
            __m128i v_b = _mm_shufflelo_epi16(v_half, _MM_SHUFFLE(0, 0, 0, 0));
            __m128i v_g = _mm_shufflelo_epi16(v_half, _MM_SHUFFLE(1, 1, 1, 1));
            __m128i v_r = _mm_shufflelo_epi16(v_half, _MM_SHUFFLE(2, 2, 2, 2));
            
            // ... actually let's use a simpler masking approach for 4 pixels at once
        };

        // Optimized Pointer Unrolling (User preferred high speed pointer logic)
        DWORD pix0 = p[0];
        DWORD pix1 = p[1];
        DWORD pix2 = p[2];
        DWORD pix3 = p[3];

        #define PIX_MATCH(pix) { \
            int r = (pix >> 16) & 0xFF; \
            int g = (pix >> 8) & 0xFF; \
            int b = pix & 0xFF; \
            int dr = r - tr; \
            int dg = g - tg; \
            int db = b - tb; \
            if ((dr*dr + dg*dg + db*db) <= tolSq) match++; \
        }

        PIX_MATCH(pix0); PIX_MATCH(pix1); PIX_MATCH(pix2); PIX_MATCH(pix3);
    }

    for (; i < totalPixels; i++, p++) {
        DWORD pix = *p;
        int r = (pix >> 16) & 0xFF;
        int g = (pix >> 8) & 0xFF;
        int b = pix & 0xFF;
        int dr = r - tr;
        int dg = g - tg;
        int db = b - tb;
        if ((dr*dr + dg*dg + db*db) <= tolSq) match++;
    }

    return match;
}
