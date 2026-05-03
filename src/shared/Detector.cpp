#include "shared/Detector.h"
#include <algorithm>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

// ---------- shared pixel matching -------------------------------------------

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

// ---------- ctor / dtor -----------------------------------------------------

FovDetector::FovDetector() { InitDXGI(); }

FovDetector::~FovDetector() {
  ReleaseDXGI();
  if (m_hdcMem) { SelectObject(m_hdcMem, m_hOld); DeleteDC(m_hdcMem); }
  if (m_hbm)       DeleteObject(m_hbm);
  if (m_hdcScreen) ReleaseDC(NULL, m_hdcScreen);
}

// ---------- DXGI init / teardown --------------------------------------------

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

// ---------- main scan -------------------------------------------------------

int FovDetector::Scan(const RoiConfig &cfg) {
  if (cfg.w <= 0 || cfg.h <= 0) return 0;

  if (m_dxgiOk) {
    DXGI_OUTDUPL_FRAME_INFO fi{};
    IDXGIResource *res = nullptr;
    // Block up to 17ms for a new frame (~60Hz cadence); returns immediately if ready
    HRESULT hr = m_duplication->AcquireNextFrame(17, &fi, &res);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return 0; // no new frame, skip this cycle

    if (FAILED(hr)) {
      // Mode switch / session change — reinit and fall back to BitBlt this cycle
      ReleaseDXGI();
      InitDXGI();
      return ScanBitBlt(cfg);
    }

    ID3D11Texture2D *desktopTex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&desktopTex);
    res->Release();

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
    D3D11_BOX box{ (UINT)cfg.x, (UINT)cfg.y, 0,
                   (UINT)(cfg.x + cfg.w), (UINT)(cfg.y + cfg.h), 1 };
    m_d3dCtx->CopySubresourceRegion(m_stagingTex, 0, 0, 0, 0,
                                     desktopTex, 0, &box);
    desktopTex->Release();
    m_duplication->ReleaseFrame();

    // Map staging texture to a CPU-readable pointer
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
  bmi.bmiHeader.biHeight      = -h; // top-down
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
  BitBlt(m_hdcMem, 0, 0, cfg.w, cfg.h, m_hdcScreen, cfg.x, cfg.y, SRCCOPY);

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
