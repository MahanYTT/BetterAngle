#include "shared/Detector.h"
#include <algorithm>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ---------- HLSL compute shader source -------------------------------------
// Each thread handles one pixel. InterlockedAdd accumulates match count.
// BGRA format from DXGI: channel order is x=B, y=G, z=R, w=A.

static const char s_hlsl[] = R"HLSL(
Texture2D<uint4> roiTex    : register(t0);
RWBuffer<uint>   matchCount : register(u0);

cbuffer ColorParams : register(b0) {
    uint targetR;
    uint targetG;
    uint targetB;
    uint tolSq;
};

[numthreads(16, 16, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    uint w, h;
    roiTex.GetDimensions(w, h);
    if (id.x >= w || id.y >= h) return;

    uint4 pix = roiTex.Load(int3(id.xy, 0));
    int dr = (int)pix.z - (int)targetR;
    int dg = (int)pix.y - (int)targetG;
    int db = (int)pix.x - (int)targetB;
    if ((dr*dr + dg*dg + db*db) <= (int)tolSq)
        InterlockedAdd(matchCount[0], 1u);
}
)HLSL";

// ---------- shared CPU pixel matching (BitBlt fallback) --------------------

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

// ---------- DXGI + compute init / teardown ---------------------------------

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

  if (FAILED(hr)) return false;
  m_dxgiOk = true;

  m_computeOk = InitComputeShader();
  return true;
}

bool FovDetector::InitComputeShader() {
  // Compile HLSL at runtime — no external file needed
  ID3DBlob *blob = nullptr, *err = nullptr;
  HRESULT hr = D3DCompile(s_hlsl, sizeof(s_hlsl), nullptr, nullptr, nullptr,
                           "CSMain", "cs_5_0", 0, 0, &blob, &err);
  if (err) err->Release();
  if (FAILED(hr) || !blob) return false;

  hr = m_d3dDevice->CreateComputeShader(blob->GetBufferPointer(),
                                         blob->GetBufferSize(),
                                         nullptr, &m_computeShader);
  blob->Release();
  if (FAILED(hr)) return false;

  // Constant buffer: 4 x uint (r, g, b, tolSq) = 16 bytes
  D3D11_BUFFER_DESC cbd{};
  cbd.ByteWidth      = 16;
  cbd.Usage          = D3D11_USAGE_DYNAMIC;
  cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
  cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  return SUCCEEDED(m_d3dDevice->CreateBuffer(&cbd, nullptr, &m_cbParams));
}

void FovDetector::EnsureComputeResources(int w, int h) {
  if (m_roiW == w && m_roiH == h && m_roiTex) return;

  // Release existing GPU resources
  if (m_roiSRV)        { m_roiSRV->Release();        m_roiSRV        = nullptr; }
  if (m_roiTex)        { m_roiTex->Release();         m_roiTex        = nullptr; }
  if (m_countUAV)      { m_countUAV->Release();       m_countUAV      = nullptr; }
  if (m_countBuf)      { m_countBuf->Release();       m_countBuf      = nullptr; }
  if (m_countReadback) { m_countReadback->Release();  m_countReadback = nullptr; }

  // ROI texture: DEFAULT usage, SRV-bindable, CS reads from it
  D3D11_TEXTURE2D_DESC td{};
  td.Width            = (UINT)w;
  td.Height           = (UINT)h;
  td.MipLevels        = 1;
  td.ArraySize        = 1;
  td.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage            = D3D11_USAGE_DEFAULT;
  td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
  if (FAILED(m_d3dDevice->CreateTexture2D(&td, nullptr, &m_roiTex))) return;
  if (FAILED(m_d3dDevice->CreateShaderResourceView(m_roiTex, nullptr, &m_roiSRV))) return;

  // Count buffer: 1 UINT, UAV-bindable
  D3D11_BUFFER_DESC bd{};
  bd.ByteWidth           = sizeof(UINT);
  bd.Usage               = D3D11_USAGE_DEFAULT;
  bd.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
  bd.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
  if (FAILED(m_d3dDevice->CreateBuffer(&bd, nullptr, &m_countBuf))) return;

  D3D11_UNORDERED_ACCESS_VIEW_DESC uavd{};
  uavd.Format              = DXGI_FORMAT_R32_TYPELESS;
  uavd.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
  uavd.Buffer.NumElements  = 1;
  uavd.Buffer.Flags        = D3D11_BUFFER_UAV_FLAG_RAW;
  if (FAILED(m_d3dDevice->CreateUnorderedAccessView(m_countBuf, &uavd, &m_countUAV))) return;

  // Readback buffer: STAGING, CPU reads result
  D3D11_BUFFER_DESC rb{};
  rb.ByteWidth      = sizeof(UINT);
  rb.Usage          = D3D11_USAGE_STAGING;
  rb.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  if (FAILED(m_d3dDevice->CreateBuffer(&rb, nullptr, &m_countReadback))) return;

  m_roiW = w;
  m_roiH = h;
}

void FovDetector::ReleaseDXGI() {
  if (m_roiSRV)        { m_roiSRV->Release();        m_roiSRV        = nullptr; }
  if (m_roiTex)        { m_roiTex->Release();         m_roiTex        = nullptr; }
  if (m_countUAV)      { m_countUAV->Release();       m_countUAV      = nullptr; }
  if (m_countBuf)      { m_countBuf->Release();       m_countBuf      = nullptr; }
  if (m_countReadback) { m_countReadback->Release();  m_countReadback = nullptr; }
  if (m_cbParams)      { m_cbParams->Release();       m_cbParams      = nullptr; }
  if (m_computeShader) { m_computeShader->Release();  m_computeShader = nullptr; }
  if (m_duplication)   { m_duplication->Release();    m_duplication   = nullptr; }
  if (m_d3dCtx)        { m_d3dCtx->Release();         m_d3dCtx        = nullptr; }
  if (m_d3dDevice)     { m_d3dDevice->Release();      m_d3dDevice     = nullptr; }
  m_dxgiOk = false;
  m_computeOk = false;
  m_roiW = 0; m_roiH = 0;
}

// ---------- main scan ------------------------------------------------------

int FovDetector::Scan(const RoiConfig &cfg) {
  if (cfg.w <= 0 || cfg.h <= 0) return 0;

  if (m_dxgiOk) {
    DXGI_OUTDUPL_FRAME_INFO fi{};
    IDXGIResource *res = nullptr;
    HRESULT hr = m_duplication->AcquireNextFrame(17, &fi, &res);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return 0;

    if (FAILED(hr)) {
      ReleaseDXGI();
      InitDXGI();
      return ScanBitBlt(cfg);
    }

    ID3D11Texture2D *desktopTex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&desktopTex);
    res->Release();

    // ---- GPU compute path ------------------------------------------------
    if (m_computeOk) {
      EnsureComputeResources(cfg.w, cfg.h);

      if (m_roiTex && m_roiSRV && m_countBuf && m_countUAV && m_countReadback) {
        // Copy ROI from desktop texture into GPU-only ROI texture
        D3D11_BOX box{ (UINT)cfg.x, (UINT)cfg.y, 0,
                       (UINT)(cfg.x + cfg.w), (UINT)(cfg.y + cfg.h), 1 };
        m_d3dCtx->CopySubresourceRegion(m_roiTex, 0, 0, 0, 0, desktopTex, 0, &box);
        desktopTex->Release();
        m_duplication->ReleaseFrame();

        // Clear match counter to 0
        UINT zero[4] = {0, 0, 0, 0};
        m_d3dCtx->ClearUnorderedAccessViewUint(m_countUAV, zero);

        // Update constant buffer with current color params
        D3D11_MAPPED_SUBRESOURCE cbMapped{};
        if (SUCCEEDED(m_d3dCtx->Map(m_cbParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped))) {
          UINT *cb = (UINT *)cbMapped.pData;
          cb[0] = (UINT)GetRValue(cfg.target);
          cb[1] = (UINT)GetGValue(cfg.target);
          cb[2] = (UINT)GetBValue(cfg.target);
          cb[3] = (UINT)(cfg.tolerance * cfg.tolerance);
          m_d3dCtx->Unmap(m_cbParams, 0);
        }

        // Bind and dispatch compute shader
        m_d3dCtx->CSSetShader(m_computeShader, nullptr, 0);
        m_d3dCtx->CSSetShaderResources(0, 1, &m_roiSRV);
        m_d3dCtx->CSSetUnorderedAccessViews(0, 1, &m_countUAV, nullptr);
        m_d3dCtx->CSSetConstantBuffers(0, 1, &m_cbParams);

        UINT groupsX = ((UINT)cfg.w  + 15) / 16;
        UINT groupsY = ((UINT)cfg.h + 15) / 16;
        m_d3dCtx->Dispatch(groupsX, groupsY, 1);

        // Unbind UAV to avoid D3D11 warnings
        ID3D11UnorderedAccessView *nullUAV = nullptr;
        m_d3dCtx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

        // Copy result to staging and read back on CPU
        m_d3dCtx->CopyResource(m_countReadback, m_countBuf);
        D3D11_MAPPED_SUBRESOURCE resultMapped{};
        if (SUCCEEDED(m_d3dCtx->Map(m_countReadback, 0, D3D11_MAP_READ, 0, &resultMapped))) {
          UINT count = *(UINT *)resultMapped.pData;
          m_d3dCtx->Unmap(m_countReadback, 0);
          return (int)count;
        }
        return 0;
      }
    }

    // ---- CPU fallback (DXGI capture, CPU loop) ---------------------------
    // Recreate as staging texture for CPU read
    if (!m_roiTex || m_roiW != cfg.w || m_roiH != cfg.h) {
      if (m_roiSRV) { m_roiSRV->Release(); m_roiSRV = nullptr; }
      if (m_roiTex) { m_roiTex->Release(); m_roiTex = nullptr; }
      D3D11_TEXTURE2D_DESC sd{};
      sd.Width            = (UINT)cfg.w;
      sd.Height           = (UINT)cfg.h;
      sd.MipLevels        = 1;
      sd.ArraySize        = 1;
      sd.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
      sd.SampleDesc.Count = 1;
      sd.Usage            = D3D11_USAGE_STAGING;
      sd.CPUAccessFlags   = D3D11_CPU_ACCESS_READ;
      m_d3dDevice->CreateTexture2D(&sd, nullptr, &m_roiTex);
      m_roiW = cfg.w; m_roiH = cfg.h;
    }

    D3D11_BOX box{ (UINT)cfg.x, (UINT)cfg.y, 0,
                   (UINT)(cfg.x + cfg.w), (UINT)(cfg.y + cfg.h), 1 };
    m_d3dCtx->CopySubresourceRegion(m_roiTex, 0, 0, 0, 0, desktopTex, 0, &box);
    desktopTex->Release();
    m_duplication->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = m_d3dCtx->Map(m_roiTex, 0, D3D11_MAP_READ, 0, &mapped);
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
    m_d3dCtx->Unmap(m_roiTex, 0);
    return match;
  }

  return ScanBitBlt(cfg);
}

// ---------- BitBlt fallback (original logic) -------------------------------

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
