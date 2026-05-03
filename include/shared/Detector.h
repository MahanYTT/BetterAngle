#ifndef DETECTOR_H
#define DETECTOR_H

#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.h>

struct RoiConfig {
  int x, y, w, h;
  COLORREF target;
  int tolerance;
};

class FovDetector {
public:
  FovDetector();
  ~FovDetector();
  int Scan(const RoiConfig &cfg);

private:
  // DXGI path
  ID3D11Device           *m_d3dDevice   = nullptr;
  ID3D11DeviceContext    *m_d3dCtx      = nullptr;
  IDXGIOutputDuplication *m_duplication = nullptr;
  ID3D11Texture2D        *m_stagingTex  = nullptr;
  int  m_stagingW = 0, m_stagingH = 0;
  bool m_dxgiOk   = false;

  // BitBlt fallback path
  HDC     m_hdcScreen = NULL;
  HDC     m_hdcMem    = NULL;
  HBITMAP m_hbm       = NULL;
  HGDIOBJ m_hOld      = NULL;
  int     m_curW = 0, m_curH = 0;
  void   *m_pixels    = nullptr;

  bool InitDXGI();
  void ReleaseDXGI();
public:
  void ReinitDisplay(int monitorIndex);
private:
  void EnsureScreenDC();
  void EnsureResources(int w, int h);
  int  ScanBitBlt(const RoiConfig &cfg);
};

#endif // DETECTOR_H
