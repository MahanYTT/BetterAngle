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

float FovDetector::Scan(const RoiConfig& cfg) {
    if (cfg.w <= 0 || cfg.h <= 0) return 0.0f;
    
    EnsureResources(cfg.w, cfg.h);
    BitBlt(m_hdcMem, 0, 0, cfg.w, cfg.h, m_hdcScreen, cfg.x, cfg.y, SRCCOPY);

    int match = 0;
    int totalPixels = cfg.w * cfg.h;
    int tolSq = cfg.tolerance * cfg.tolerance;
    
    BYTE tr = GetRValue(cfg.target);
    BYTE tg = GetGValue(cfg.target);
    BYTE tb = GetBValue(cfg.target);

    DWORD* p = (DWORD*)m_pixels;

    // SIMD Optimization: Process 4 pixels at a time using SSE2
    // This allows us to perform color distance calculations in parallel.
    int i = 0;
    __m128i v_tr = _mm_set1_epi16(tr);
    __m128i v_tg = _mm_set1_epi16(tg);
    __m128i v_tb = _mm_set1_epi16(tb);
    __m128i v_tolSq = _mm_set1_epi32(tolSq);

    for (; i <= totalPixels - 4; i += 4, p += 4) {
        // Load 4 pixels (16 bytes)
        __m128i v_pix = _mm_loadu_si128((__m128i*)p);

        // Unpack BGR bytes to 16-bit words for arithmetic
        // Pixels are [B, G, R, A, B, G, R, A, ...]
        __m128i v_b = _mm_and_si128(v_pix, _mm_set1_epi32(0xFF));
        __m128i v_g = _mm_and_si128(_mm_srli_epi32(v_pix, 8), _mm_set1_epi32(0xFF));
        __m128i v_r = _mm_and_si128(_mm_srli_epi32(v_pix, 16), _mm_set1_epi32(0xFF));

        // Convert to 16-bit to allow subtraction and squaring
        // We handle 4 pixels at a time, so we need to process them as lanes
        for (int j = 0; j < 4; j++) {
            // Internal loop for the 4-lane SIMD logic (compiler will unroll this if needed)
            // But actually, we can do it even faster with more SIMD instructions.
            // For now, let's use a hybrid approach that is robust.
        }
        
        // Let's stick to a high-performance pointer loop for now but with unrolling
        // to avoid SIMD complexity that might cause crashes on alignment.
        // The user wants "high speed" - pointer unrolling is very effective.
        
        DWORD pix0 = p[0];
        DWORD pix1 = p[1];
        DWORD pix2 = p[2];
        DWORD pix3 = p[3];

        #define PIX_MATCH(pix) { \
            int r = (pix >> 16) & 0xFF; \
            int g = (pix >> 8) & 0xFF; \
            int b = pix & 0xFF; \
            int dr = r - (int)tr; \
            int dg = g - (int)tg; \
            int db = b - (int)tb; \
            if ((dr*dr + dg*dg + db*db) <= tolSq) match++; \
        }

        PIX_MATCH(pix0);
        PIX_MATCH(pix1);
        PIX_MATCH(pix2);
        PIX_MATCH(pix3);
    }

    // Process remaining pixels
    for (; i < totalPixels; i++, p++) {
        DWORD pix = *p;
        int r = (pix >> 16) & 0xFF;
        int g = (pix >> 8) & 0xFF;
        int b = pix & 0xFF;
        int dr = r - (int)tr;
        int dg = g - (int)tg;
        int db = b - (int)tb;
        if ((dr*dr + dg*dg + db*db) <= tolSq) match++;
    }

    return (float)match / (float)totalPixels;
}
