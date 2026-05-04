#include "shared/Detector.h"
#include <algorithm>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

// ---------- shared pixel matching ------------------------------------------

#define PIX_MATCH_INT(pix)                      \
  {                                             \
    int r = (int)((pix >> 16) & 0xFF);          \
    int g = (int)((pix >> 8)  & 0xFF);          \
    int b = (int)(pix & 0xFF);                  \
    int dr = r - tr, dg = g - tg, db = b - tb; \
    if ((dr*dr + dg*dg + db*db) <= tolSq)       \
      match++;                                  \
  }

static int CountMatches(const DWORD *p, int total,
                        int tr, int tg, int tb, int tolSq) {
  int match = 0, i = 0;
  for (; i <= total - 4; i += 4, p += 4) {
    DWORD p0=p[0], p1=p[1], p2=p[2], p3=p[3];
    PIX_MATCH_INT(p0); PIX_MATCH_INT(p1);
    PIX_MATCH_INT(p2); PIX_MATCH_INT(p3);
  }
  for (; i < total; i++, p++) { DWORD pix = *p; PIX_MATCH_INT(pix); }
  return match;
}

// ---------- ctor / dtor ----------------------------------------------------

FovDetector::FovDetector() { InitDXGI(); }

FovDetector::~FovDetector() {
  ReleaseDXGI();
  if (m_hdcMem) { SelectObject(m_hdcMem, m_hOld); DeleteDC(m_hdcMem); }
  if (m_hbm)       DeleteObject(m_hbm);
  if (m_hdcScreen) ReleaseDC(NULL, m_hdcScreen);
}

// ---------- DXGI init / teardown -------------------------------------------

bool FovDetector::InitDXGI() {
  D3D_FEATURE_LEVEL fl;
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                  0, nullptr, 0, D3D11_SDK_VERSION,
                                  &m_d3dDevice, &fl, &m_d3dCtx);
  if (FAILED(hr)) return false;

  IDXGIDevice *dxgiDevice = nullptr;
  m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
  IDXGIAdapter *adapter = nullptr;
  dxgiDevice->GetAdapter(&adapter);
  dxgiDevice->Release();

  IDXGIOutput *output = nullptr;
  adapter->EnumOutputs(0, &output);
  adapter->Release();

  IDXGIOutput1 *output1 = nullptr;
  output->QueryInterface(__uuidof(IDXGIOutput1), (void **)&output1);
  output->Release();

  hr = output1->DuplicateOutput(m_d3dDevice, &m_duplication);
  output1->Release();

  m_dxgiOk = SUCCEEDED(hr);
  return m_dxgiOk;
}

void FovDetector::ReleaseDXGI() {
  if (m_stagingTex)  { m_stagingTex->Release();  m_stagingTex  = nullptr; }
  if (m_duplication) { m_duplication->Release(); m_duplication = nullptr; }
  if (m_d3dCtx)      { m_d3dCtx->Release();      m_d3dCtx      = nullptr; }
  if (m_d3dDevice)   { m_d3dDevice->Release();   m_d3dDevice   = nullptr; }
  m_dxgiOk = false;
}

void FovDetector::ReinitDisplay(int monitorIndex) {
  // Release only the duplication + staging tex; keep nothing (device recreated in InitDXGI)
  if (m_stagingTex)  { m_stagingTex->Release();  m_stagingTex  = nullptr; }
  if (m_duplication) { m_duplication->Release(); m_duplication = nullptr; }
  m_dxgiOk = false;
  m_stagingW = 0; m_stagingH = 0;

  if (!m_d3dDevice) return;

  IDXGIDevice *dxgiDevice = nullptr;
  m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
  if (!dxgiDevice) return;

  IDXGIAdapter *adapter = nullptr;
  dxgiDevice->GetAdapter(&adapter);
  dxgiDevice->Release();
  if (!adapter) return;

  IDXGIOutput *output = nullptr;
  // Try requested index; fall back to 0 if out of range
  if (FAILED(adapter->EnumOutputs((UINT)monitorIndex, &output)) || !output)
    adapter->EnumOutputs(0, &output);
  adapter->Release();
  if (!output) return;

  IDXGIOutput1 *output1 = nullptr;
  output->QueryInterface(__uuidof(IDXGIOutput1), (void **)&output1);
  output->Release();
  if (!output1) return;

  HRESULT hr = output1->DuplicateOutput(m_d3dDevice, &m_duplication);
  output1->Release();
  m_dxgiOk = SUCCEEDED(hr);
}

// ---------- main scan ------------------------------------------------------

int FovDetector::Scan(const RoiConfig &cfg) {
  if (cfg.w <= 0 || cfg.h <= 0) return 0;

  if (m_dxgiOk) {
    DXGI_OUTDUPL_FRAME_INFO fi{};
    IDXGIResource *res = nullptr;
    HRESULT hr = m_duplication->AcquireNextFrame(0, &fi, &res);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return -1; // no new frame — caller must skip edge detection

    if (FAILED(hr)) {
      ReleaseDXGI();
      InitDXGI();
      return ScanBitBlt(cfg);
    }

    ID3D11Texture2D *desktopTex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&desktopTex);
    res->Release();

    D3D11_TEXTURE2D_DESC desc;
    desktopTex->GetDesc(&desc);

    // If the monitor is HDR or using a non-standard 8-bit format, fallback to BitBlt
    // which gracefully converts HDR down to an SDR bitmap we can read.
    if (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && 
        desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) {
      desktopTex->Release();
      m_duplication->ReleaseFrame();
      return ScanBitBlt(cfg);
    }

    // Recreate staging texture if ROI size changed
    if (!m_stagingTex || m_stagingW != cfg.w || m_stagingH != cfg.h) {
      if (m_stagingTex) { m_stagingTex->Release(); m_stagingTex = nullptr; }
      D3D11_TEXTURE2D_DESC sd{};
      sd.Width            = (UINT)cfg.w;
      sd.Height           = (UINT)cfg.h;
      sd.MipLevels        = 1;
      sd.ArraySize        = 1;
      sd.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
      sd.SampleDesc.Count = 1;
      sd.Usage            = D3D11_USAGE_STAGING;
      sd.CPUAccessFlags   = D3D11_CPU_ACCESS_READ;
      m_d3dDevice->CreateTexture2D(&sd, nullptr, &m_stagingTex);
      m_stagingW = cfg.w;
      m_stagingH = cfg.h;
    }

    // Copy ROI sub-rect from the full desktop texture into the small staging texture
    // cfg.x/y are monitor-relative; add monitorOffset to get screen-space coords for DXGI
    int bx = cfg.x + cfg.monitorOffsetX;
    int by = cfg.y + cfg.monitorOffsetY;
    D3D11_BOX box{ (UINT)bx, (UINT)by, 0,
                   (UINT)(bx + cfg.w), (UINT)(by + cfg.h), 1 };
    m_d3dCtx->CopySubresourceRegion(m_stagingTex, 0, 0, 0, 0,
                                     desktopTex, 0, &box);
    desktopTex->Release();
    m_duplication->ReleaseFrame();

    // Map staging texture to a CPU-readable pointer and run pixel loop
    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = m_d3dCtx->Map(m_stagingTex, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) return ScanBitBlt(cfg);

    int tolSq = cfg.tolerance * cfg.tolerance;
    int tr = (int)GetRValue(cfg.target);
    int tg = (int)GetGValue(cfg.target);
    int tb = (int)GetBValue(cfg.target);

    int match = 0;
    for (int row = 0; row < cfg.h; row++) {
      const DWORD *rowPtr = reinterpret_cast<const DWORD *>(
          static_cast<const BYTE *>(mapped.pData) + row * mapped.RowPitch);
      match += CountMatches(rowPtr, cfg.w, tr, tg, tb, tolSq);
    }

    m_d3dCtx->Unmap(m_stagingTex, 0);
    return match;
  }

  return ScanBitBlt(cfg);
}

// ---------- BitBlt fallback (original logic) --------------------------------

void FovDetector::EnsureScreenDC() {
  if (!m_hdcScreen) m_hdcScreen = GetDC(NULL);
}

void FovDetector::EnsureResources(int w, int h) {
  EnsureScreenDC();
  if (w == m_curW && h == m_curH && m_hdcMem) return;

  if (m_hdcMem) {
    SelectObject(m_hdcMem, m_hOld);
    DeleteDC(m_hdcMem);
    DeleteObject(m_hbm);
  }

  m_hdcMem = CreateCompatibleDC(m_hdcScreen);

  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth       = w;
  bmi.bmiHeader.biHeight      = -h;
  bmi.bmiHeader.biPlanes      = 1;
  bmi.bmiHeader.biBitCount    = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  m_hbm  = CreateDIBSection(m_hdcScreen, &bmi, DIB_RGB_COLORS, &m_pixels, NULL, 0);
  m_hOld = SelectObject(m_hdcMem, m_hbm);
  m_curW = w;
  m_curH = h;
}

int FovDetector::ScanBitBlt(const RoiConfig &cfg) {
  EnsureResources(cfg.w, cfg.h);
  // BitBlt uses GetDC(NULL) which is the full virtual desktop — need screen-space coords
  BitBlt(m_hdcMem, 0, 0, cfg.w, cfg.h, m_hdcScreen,
         cfg.x + cfg.monitorOffsetX, cfg.y + cfg.monitorOffsetY, SRCCOPY);

  int tolSq = cfg.tolerance * cfg.tolerance;
  int tr = (int)GetRValue(cfg.target);
  int tg = (int)GetGValue(cfg.target);
  int tb = (int)GetBValue(cfg.target);

  int match = 0;
  for (int row = 0; row < cfg.h; row++) {
    const DWORD *rowPtr = reinterpret_cast<const DWORD *>(m_pixels) + row * cfg.w;
    match += CountMatches(rowPtr, cfg.w, tr, tg, tb, tolSq);
  }
  return match;
}
