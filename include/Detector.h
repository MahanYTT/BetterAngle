#ifndef DETECTOR_H
#define DETECTOR_H

#include <windows.h>
#include <vector>

struct RoiConfig {
    int x, y, w, h;
    COLORREF target;
    int tolerance;
};

class FovDetector {
public:
    FovDetector();
    ~FovDetector();

    float Scan(const RoiConfig& cfg);

private:
    HDC m_hdcScreen;
    HDC m_hdcMem;
    HBITMAP m_hbm;
    HGDIOBJ m_hOld;
    int m_curW, m_curH;

    void EnsureResources(int w, int h);
};

#endif // DETECTOR_H
