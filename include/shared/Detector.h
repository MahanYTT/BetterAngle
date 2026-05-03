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
  // DXGI + D3D11 core
  ID3D11Device           *m_d3dDevice   = nullptr;
  ID3D11DeviceContext    *m_d3dCtx      = nullptr;
  IDXGIOutputDuplication *m_duplication = nullptr;
  bool m_dxgiOk = false;

  // GPU compute shader resources
  ID3D11ComputeShader       *m_computeShader = nullptr;
  ID3D11Buffer              *m_cbParams      = nullptr; // constant buffer: r,g,b,tolSq
  ID3D11Texture2D           *m_roiTex        = nullptr; // DEFAULT usage, CS input
  ID3D11ShaderResourceView  *m_roiSRV        = nullptr;
  ID3D11Buffer              *m_countBuf      = nullptr; // UAV output: 1 UINT match count
  ID3D11UnorderedAccessView *m_countUAV      = nullptr;
  ID3D11Buffer              *m_countReadback = nullptr; // STAGING, CPU reads result
  int m_roiW = 0, m_roiH = 0;
  bool m_computeOk = false;

  // BitBlt fallback path
  HDC     m_hdcScreen = NULL;
  HDC     m_hdcMem    = NULL;
  HBITMAP m_hbm       = NULL;
  HGDIOBJ m_hOld      = NULL;
  int     m_curW = 0, m_curH = 0;
  void   *m_pixels    = nullptr;

  bool InitDXGI();
  void ReleaseDXGI();
  bool InitComputeShader();
  void EnsureComputeResources(int w, int h);
  void EnsureScreenDC();
  void EnsureResources(int w, int h);
  int  ScanBitBlt(const RoiConfig &cfg);
};

#endif // DETECTOR_H
