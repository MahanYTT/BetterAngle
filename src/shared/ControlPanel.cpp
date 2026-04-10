// ControlPanel.cpp - BetterAngle Pro v4.20.4
// Full responsive layout, pixel-perfect hit-testing, resizable window.
#include "shared/State.h"
#include "shared/Overlay.h"
#include "shared/ControlPanel.h"
#include "shared/Updater.h"
#include "shared/Profile.h"
#include <thread>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <algorithm>
#include <vector>

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern Profile g_currentProfile;

#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace D2D1;

ID2D1Factory*          g_pD2DFactory   = NULL;
IDWriteFactory*        g_pDWriteFactory = NULL;
ID2D1HwndRenderTarget* g_pRenderTarget  = NULL;

// ─────────────────────────────────────────────────────────────────────────────
// D2D init / resize
// ─────────────────────────────────────────────────────────────────────────────
void InitD2D(HWND hWnd) {
    if (!g_pD2DFactory) {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_pDWriteFactory);
    }
    RECT rc; GetClientRect(hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    if (g_pRenderTarget) {
        // Attempt resize first — avoids destroying/recreating the target every frame
        HRESULT hr = g_pRenderTarget->Resize(size);
        if (SUCCEEDED(hr)) return;
        g_pRenderTarget->Release();
        g_pRenderTarget = NULL;
    }
    g_pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hWnd, size),
        &g_pRenderTarget
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Shared button helper
// ─────────────────────────────────────────────────────────────────────────────
void DrawD2DButton(ID2D1HwndRenderTarget* rt, D2D1_RECT_F rect, const wchar_t* text, D2D1_COLOR_F color, float fontSize = 13.0f) {
    ID2D1SolidColorBrush* pBrush  = NULL;
    ID2D1SolidColorBrush* pStroke = NULL;
    ID2D1SolidColorBrush* pWhite  = NULL;

    rt->CreateSolidColorBrush(color, &pBrush);
    rt->CreateSolidColorBrush(D2D1::ColorF((std::min)(1.0f, color.r * 1.6f), (std::min)(1.0f, color.g * 1.6f), (std::min)(1.0f, color.b * 1.6f)), &pStroke);
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);

    rt->FillRoundedRectangle(D2D1::RoundedRect(rect, 6.0f, 6.0f), pBrush);
    rt->DrawRoundedRectangle(D2D1::RoundedRect(rect, 6.0f, 6.0f), pStroke, 1.0f);

    IDWriteTextFormat* pFmt = NULL;
    g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display", NULL,
        DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        fontSize, L"en-us", &pFmt
    );
    if (pFmt) {
        pFmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        pFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        rt->DrawText(text, (UINT32)wcslen(text), pFmt, rect, pWhite);
        pFmt->Release();
    }

    if (pWhite)  pWhite->Release();
    if (pStroke) pStroke->Release();
    if (pBrush)  pBrush->Release();
}

// ─────────────────────────────────────────────────────────────────────────────
// Window creation — WS_OVERLAPPEDWINDOW gives full resize/drag support
// ─────────────────────────────────────────────────────────────────────────────
HWND CreateControlPanel(HINSTANCE hInst) {
    WNDCLASS wc = {};
    wc.lpfnWndProc   = ControlPanelWndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    HWND hPanel = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"BetterAngleControlPanel",
        L"BetterAngle Pro | Global Command Center",
        WS_OVERLAPPEDWINDOW,          // ← this gives drag-resize handles
        CW_USEDEFAULT, CW_USEDEFAULT,
        580, 620,
        NULL, NULL, hInst, NULL
    );
    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}

int g_listeningKey = -1;

std::wstring GetKeyName(UINT mod, UINT vk) {
    if (vk == 0) return L"Unbound";
    std::wstring n;
    if (mod & MOD_CONTROL) n += L"Ctrl+";
    if (mod & MOD_SHIFT)   n += L"Shift+";
    if (mod & MOD_ALT)     n += L"Alt+";

    if (vk >= 'A' && vk <= 'Z')     n += (wchar_t)vk;
    else if (vk >= '0' && vk <= '9') n += (wchar_t)vk;
    else if (vk >= VK_F1 && vk <= VK_F12) n += L"F" + std::to_wstring(vk - VK_F1 + 1);
    else n += L"Key(" + std::to_wstring(vk) + L")";
    return n;
}

extern HWND g_hHUD;

// ─────────────────────────────────────────────────────────────────────────────
// Layout helpers — returns consistent rects for both PAINT and HITTEST
// ─────────────────────────────────────────────────────────────────────────────
struct Layout {
    float W, H;
    float margin;
    float contentW;
    float tY1, tY2;        // Tab row
    float tabW;
    // 5 tab rects
    float tabX[5], tabX2[5];
    float cY;              // Content area top
    float footerY1, footerY2;

    Layout(float w, float h) : W(w), H(h) {
        margin   = 0.07f * W;
        contentW = W - margin * 2.0f;
        tY1      = 0.155f * H;
        tY2      = 0.225f * H;

        // Evenly distribute 5 tabs across contentW with 8px gaps
        float gap   = 8.0f;
        float totalGap = gap * 4.0f;
        tabW = (contentW - totalGap) / 5.0f;

        for (int i = 0; i < 5; i++) {
            tabX[i]  = margin + i * (tabW + gap);
            tabX2[i] = tabX[i] + tabW;
        }

        cY       = 0.26f * H;
        footerY1 = 0.88f * H;
        footerY2 = 0.96f * H;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Main WndProc
// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE:
        InitD2D(hWnd);
        SetTimer(hWnd, 1, 16, NULL);
        return 0;

    case WM_SIZE:
        InitD2D(hWnd);       // Resize the D2D render target to match new client size
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_GETMINMAXINFO: {
        // Enforce a sensible minimum so UI never completely collapses
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 420;
        mmi->ptMinTrackSize.y = 440;
        return 0;
    }

    case WM_KEYDOWN:
        if (g_listeningKey != -1) {
            if (wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU) return 0;
            UINT mod = 0;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
            if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mod |= MOD_SHIFT;
            if (GetAsyncKeyState(VK_MENU)    & 0x8000) mod |= MOD_ALT;

            switch (g_listeningKey) {
                case 1: g_keybinds.toggleMod = mod; g_keybinds.toggleKey = (UINT)wParam; break;
                case 2: g_keybinds.roiMod    = mod; g_keybinds.roiKey    = (UINT)wParam; break;
                case 3: g_keybinds.crossMod  = mod; g_keybinds.crossKey  = (UINT)wParam; break;
                case 4: g_keybinds.zeroMod   = mod; g_keybinds.zeroKey   = (UINT)wParam; break;
                case 5: g_keybinds.debugMod  = mod; g_keybinds.debugKey  = (UINT)wParam; break;
            }
            g_listeningKey = -1;
            SaveSettings();
            if (g_hHUD) {
                for (int i = 1; i <= 5; i++) UnregisterHotKey(g_hHUD, i);
                RegisterHotKey(g_hHUD, 1, g_keybinds.toggleMod, g_keybinds.toggleKey);
                RegisterHotKey(g_hHUD, 2, g_keybinds.roiMod,    g_keybinds.roiKey);
                RegisterHotKey(g_hHUD, 3, g_keybinds.crossMod,  g_keybinds.crossKey);
                RegisterHotKey(g_hHUD, 4, g_keybinds.zeroMod,   g_keybinds.zeroKey);
                RegisterHotKey(g_hHUD, 5, g_keybinds.debugMod,  g_keybinds.debugKey);
            }
        }
        return 0;

    // ─────────────────────────────────────────────────────────────────────────
    // HIT TESTING — coordinates match Layout exactly
    // ─────────────────────────────────────────────────────────────────────────
    case WM_LBUTTONDOWN: {
        RECT rc; GetClientRect(hWnd, &rc);
        float fx = (float)LOWORD(lParam);
        float fy = (float)HIWORD(lParam);
        Layout L((float)(rc.right - rc.left), (float)(rc.bottom - rc.top));

        // ── Tab bar ──────────────────────────────────────────────────────────
        if (fy >= L.tY1 && fy <= L.tY2) {
            for (int i = 0; i < 5; i++) {
                if (fx >= L.tabX[i] && fx <= L.tabX2[i]) {
                    g_currentTab   = i;
                    g_listeningKey = -1;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            return 0;
        }

        // ── Quit footer ──────────────────────────────────────────────────────
        if (fx >= L.margin && fx <= L.W - L.margin && fy >= L.footerY1 && fy <= L.footerY2) {
            PostQuitMessage(0);
            return 0;
        }

        float cY = L.cY;

        // ── TAB 0 — GENERAL ───────────────────────────────────────────────
        if (g_currentTab == 0 && g_listeningKey == -1) {
            // Keybind click areas (right 40% of width)
            float bindX = L.margin + (L.contentW * 0.6f);
            float rowH  = 0.04f * L.H;
            float rowGap = 0.005f * L.H;
            struct { int id; float y; } binds[5] = {
                {1, cY + 0.05f * L.H},
                {2, cY + 0.10f * L.H},
                {3, cY + 0.15f * L.H},
                {4, cY + 0.20f * L.H},
                {5, cY + 0.25f * L.H},
            };
            for (auto& b : binds) {
                if (fx >= bindX && fx <= L.W - L.margin && fy >= b.y && fy <= b.y + rowH + rowGap)
                    g_listeningKey = b.id;
            }
        }

        // ── TAB 1 — UPDATES ───────────────────────────────────────────────
        else if (g_currentTab == 1) {
            // Check / Install button
            if (fx >= L.margin && fx <= L.W - L.margin && fy >= 0.45f * L.H && fy <= 0.56f * L.H) {
                if (g_updateAvailable) {
                    std::thread([]() {
                        // Resolve absolute path next to the running exe
                        wchar_t exeBuf[MAX_PATH] = {};
                        GetModuleFileNameW(NULL, exeBuf, MAX_PATH);
                        std::wstring exePath = exeBuf;
                        size_t slash = exePath.find_last_of(L"\\/");
                        std::wstring tmpPath = exePath.substr(0, slash + 1) + L"update_tmp.exe";

                        if (DownloadUpdate(L"AUTO", tmpPath))
                            ApplyUpdateAndRestart();
                        else
                            g_updateAvailable = true; // restore flag so user can retry
                    }).detach();
                } else {
                    g_isCheckingForUpdates = true;
                    std::thread(CheckForUpdates).detach();
                }
            }
        }

        // ── TAB 2 — COLORS ────────────────────────────────────────────────
        else if (g_currentTab == 2) {
            // Target Color swatch area
            if (fx >= L.margin && fx <= L.W - L.margin && fy >= cY + 0.15f * L.H && fy <= cY + 0.32f * L.H) {
                CHOOSECOLOR cc = {};
                static COLORREF acrCustClr[16] = {};
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hWnd;
                cc.lpCustColors = (LPDWORD)acrCustClr;
                cc.rgbResult = g_targetColor;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                if (ChooseColor(&cc)) {
                    g_targetColor = cc.rgbResult;
                    SaveSettings();
                }
            }
        }

        // ── TAB 3 — DEBUG ─────────────────────────────────────────────────
        else if (g_currentTab == 3) {
            float bH = 0.06f * L.H;
            if (fy >= cY + 0.05f * L.H && fy <= cY + 0.05f * L.H + bH)           g_debugMode        = !g_debugMode;
            else if (fy >= cY + 0.13f * L.H && fy <= cY + 0.13f * L.H + bH)       g_forceDiving      = !g_forceDiving;
            else if (fy >= cY + 0.21f * L.H && fy <= cY + 0.21f * L.H + bH)       g_forceDetection   = !g_forceDetection;
            else if (fy >= cY + 0.29f * L.H && fy <= cY + 0.29f * L.H + bH) {     g_currentAngle = 0.0f; g_logic.SetZero(); }
            // Tolerance buttons
            else if (fy >= 0.60f * L.H && fy <= 0.67f * L.H) {
                if (fx >= L.margin && fx <= L.margin + L.contentW * 0.45f) {
                    if (!g_allProfiles.empty()) {
                        g_allProfiles[g_selectedProfileIdx].tolerance = (std::max)(0, g_allProfiles[g_selectedProfileIdx].tolerance - 2);
                        g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                    }
                } else if (fx >= L.W - L.margin - L.contentW * 0.45f && fx <= L.W - L.margin) {
                    if (!g_allProfiles.empty()) {
                        g_allProfiles[g_selectedProfileIdx].tolerance += 2;
                        g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                    }
                }
            }
        }

        // ── TAB 4 — CROSSHAIR ─────────────────────────────────────────────
        else if (g_currentTab == 4) {
            float rowH = 0.06f * L.H;
            
            // Layout constants for 2-column
            float col1X = L.margin;
            float col1W = L.contentW * 0.45f;
            float col2X = L.margin + L.contentW * 0.55f;
            float col2W = L.contentW * 0.45f;

            // Color picker (left) & Pulse toggle (right)
            if (fy >= cY + 0.06f * L.H && fy <= cY + 0.06f * L.H + rowH) {
                if (fx >= col1X && fx <= col1X + col1W) {
                    CHOOSECOLOR cc = {};
                    static COLORREF acrCustClr[16] = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner   = hWnd;
                    cc.lpCustColors = (LPDWORD)acrCustClr;
                    cc.rgbResult   = g_crossColor;
                    cc.Flags       = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColor(&cc)) { g_crossColor = cc.rgbResult; SaveSettings(); }
                }
                else if (fx >= col2X && fx <= col2X + col2W) {
                    g_crossPulse = !g_crossPulse;
                    SaveSettings();
                }
            }

            // ─ / + buttons 2 columns ─────────────────────────
            float row1Y = cY + 0.16f * L.H;
            float row2Y = cY + 0.28f * L.H;
            float btnW  = col1W * 0.25f;

            auto hitTestPlusMinus = [&](float rowY, float colX, int idx) {
                if (fy >= rowY && fy <= rowY + rowH) {
                    float minX = colX + col1W * 0.4f;
                    float plusX = colX + col1W * 0.7f;
                    if (fx >= minX && fx <= minX + btnW) {
                        switch(idx) {
                            case 0: g_crossThickness = (std::max)(1.0f, g_crossThickness - 1.0f); break;
                            case 1: g_crossOffsetX -= 1.0f; break;
                            case 2: g_crossOffsetY -= 1.0f; break;
                            case 3: g_crossAngle -= 5.0f; break;
                        }
                        SaveSettings();
                    } else if (fx >= plusX && fx <= plusX + btnW) {
                        switch(idx) {
                            case 0: g_crossThickness += 1.0f; break;
                            case 1: g_crossOffsetX += 1.0f; break;
                            case 2: g_crossOffsetY += 1.0f; break;
                            case 3: g_crossAngle += 5.0f; break;
                        }
                        SaveSettings();
                    }
                }
            };
            
            hitTestPlusMinus(row1Y, col1X, 0); // Thickness (Col 1, Row 1)
            hitTestPlusMinus(row1Y, col2X, 3); // Rotation (Col 2, Row 1)
            hitTestPlusMinus(row2Y, col1X, 1); // Offset X (Col 1, Row 2)
            hitTestPlusMinus(row2Y, col2X, 2); // Offset Y (Col 2, Row 2)

            // Action Buttons: RESET, SAVE POS, SAVE ALL
            float btnRowY = cY + 0.42f * L.H;
            float abW = (L.contentW - 30) / 3;
            if (fy >= btnRowY && fy <= btnRowY + rowH) {
                if (fx >= L.margin && fx <= L.margin + abW) {
                    g_crossThickness = 2.0f; g_crossOffsetX = 0.0f; g_crossOffsetY = 0.0f;
                    g_crossAngle = 0.0f; g_crossPulse = false; g_crossColor = RGB(255,0,0);
                } else if (fx >= L.margin + abW + 15 && fx <= L.margin + 2 * abW + 15) {
                    if (!g_allProfiles.empty()) {
                        Profile& p = g_allProfiles[g_selectedProfileIdx];
                        p.crossOffsetX = g_crossOffsetX; p.crossOffsetY = g_crossOffsetY;
                        p.Save(GetAppStoragePath() + p.name + L".json");
                    }
                } else if (fx >= L.margin + 2 * abW + 30 && fx <= L.margin + 3 * abW + 30) {
                    if (!g_allProfiles.empty()) {
                        Profile& p = g_allProfiles[g_selectedProfileIdx];
                        p.crossThickness = g_crossThickness; p.crossColor = g_crossColor;
                        p.crossOffsetX = g_crossOffsetX; p.crossOffsetY = g_crossOffsetY;
                        p.crossAngle = g_crossAngle; p.crossPulse = g_crossPulse;
                        p.Save(GetAppStoragePath() + p.name + L".json");
                    }
                }
            }
        }

        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PAINTING — must exactly mirror hit-test layout above
    // ─────────────────────────────────────────────────────────────────────────
    case WM_PAINT: {
        if (!g_pRenderTarget) return 0;

        g_pRenderTarget->BeginDraw();

        g_pRenderTarget->Clear(D2D1::ColorF(0.02f, 0.03f, 0.05f));

        D2D1_SIZE_F sz = g_pRenderTarget->GetSize();
        Layout L(sz.width, sz.height);

        // Base scale for fonts
        float baseScale = (std::min)(L.W / 560.0f, L.H / 600.0f);
        if (baseScale < 0.65f) baseScale = 0.65f;

        // ── Brushes ──────────────────────────────────────────────────────────
        ID2D1SolidColorBrush *pWhite = NULL, *pGrey = NULL, *pBlue = NULL, *pDark = NULL;
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &pWhite);
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.55f, 0.55f, 0.55f), &pGrey);
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.7f, 1.0f), &pBlue);
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.10f, 0.13f), &pDark);

        // ── Text formats ─────────────────────────────────────────────────────
        IDWriteTextFormat *pTitle = NULL, *pHeader = NULL, *pBody = NULL;
        g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD,    DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 22.0f * baseScale, L"en-us", &pTitle);
        g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD,    DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f * baseScale, L"en-us", &pHeader);
        g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f * baseScale, L"en-us", &pBody);

        // ── Title bar ────────────────────────────────────────────────────────
        g_pRenderTarget->DrawText(
            L"BetterAngle Pro | Command Center", 33,
            pTitle,
            D2D1::RectF(L.margin, 0.03f * L.H, L.W - L.margin, 0.13f * L.H),
            pWhite
        );

        // ── Tab buttons ──────────────────────────────────────────────────────
        const wchar_t* tabLabels[5] = { L"GENERAL", L"UPDATES", L"COLORS", L"DEBUG", L"CROSSHAIR" };
        for (int i = 0; i < 5; i++) {
            bool active = (g_currentTab == i);
            D2D1_COLOR_F col = active
                ? D2D1::ColorF(0.18f, 0.45f, 0.72f)
                : D2D1::ColorF(0.08f, 0.10f, 0.14f);
            DrawD2DButton(g_pRenderTarget,
                D2D1::RectF(L.tabX[i], L.tY1, L.tabX2[i], L.tY2),
                tabLabels[i], col, 11.0f * baseScale
            );
        }

        float cY = L.cY;

        // ─────────────────────────────────────────────────────────────────────
        // Tab content
        // ─────────────────────────────────────────────────────────────────────
        if (g_currentTab == 0) {
            // ─ GENERAL ──────────────────────────────────────────────────────
            g_pRenderTarget->DrawText(L"HOTKEYS - Click to rebind", 25, pHeader,
                D2D1::RectF(L.margin, cY, L.W - L.margin, cY + 0.04f * L.H), pWhite);

            float bindX1 = L.margin;
            float bindX2 = L.margin + L.contentW * 0.6f;
            float bindX3 = L.margin + L.contentW * 0.62f;
            float rowH   = 0.04f * L.H;

            struct BindRow { int id; const wchar_t* label; UINT mod; UINT vk; };
            BindRow rows[5] = {
                {1, L"Toggle Dashboard:",   g_keybinds.toggleMod, g_keybinds.toggleKey},
                {2, L"Visual ROI Selector:", g_keybinds.roiMod,   g_keybinds.roiKey},
                {3, L"Precision Crosshair:", g_keybinds.crossMod, g_keybinds.crossKey},
                {4, L"Zero Angle Reset:",   g_keybinds.zeroMod,  g_keybinds.zeroKey},
                {5, L"Secret Debug Tab:",   g_keybinds.debugMod, g_keybinds.debugKey},
            };
            for (int i = 0; i < 5; i++) {
                float y = cY + (0.05f + i * 0.05f) * L.H;
                g_pRenderTarget->DrawText(rows[i].label, (UINT32)wcslen(rows[i].label), pBody,
                    D2D1::RectF(bindX1, y, bindX2, y + rowH), pGrey);
                std::wstring bindText = (g_listeningKey == rows[i].id)
                    ? L"[ Press Key... ]"
                    : (L"[ " + GetKeyName(rows[i].mod, rows[i].vk) + L" ]");
                g_pRenderTarget->DrawText(bindText.c_str(), (UINT32)bindText.length(), pBody,
                    D2D1::RectF(bindX3, y, L.W - L.margin, y + rowH),
                    (g_listeningKey == rows[i].id) ? pBlue : pWhite);
            }

        } else if (g_currentTab == 1) {
            // ─ UPDATES ──────────────────────────────────────────────────────
            g_pRenderTarget->DrawText(L"SOFTWARE DASHBOARD", 18, pHeader,
                D2D1::RectF(L.margin, cY, L.W - L.margin, cY + 0.05f * L.H), pWhite);

            wchar_t cur[64], lat[64];
            swprintf_s(cur, L"Installed: v%s", VERSION_WSTR);
            if (g_latestVersionOnline.empty()) swprintf_s(lat, L"Latest:    (not checked)");
            else swprintf_s(lat, L"Latest:    %S", g_latestVersionOnline.c_str());

            g_pRenderTarget->DrawText(cur, (UINT32)wcslen(cur), pBody,
                D2D1::RectF(L.margin, cY + 0.08f * L.H, L.W - L.margin, cY + 0.13f * L.H), pGrey);
            g_pRenderTarget->DrawText(lat, (UINT32)wcslen(lat), pBody,
                D2D1::RectF(L.margin, cY + 0.14f * L.H, L.W - L.margin, cY + 0.19f * L.H), pGrey);

            D2D1_RECT_F btnRect = D2D1::RectF(L.margin, 0.45f * L.H, L.W - L.margin, 0.56f * L.H);
            ColorF btnCol = g_updateAvailable ? ColorF(0.0f, 0.5f, 0.85f) : ColorF(0.12f, 0.15f, 0.20f);

            if (g_isCheckingForUpdates) {
                float pulse = (sinf((float)GetTickCount64() * 0.008f) + 1.0f) * 0.5f;
                btnCol = ColorF(0.12f + pulse * 0.1f, 0.15f + pulse * 0.45f, 0.2f + pulse * 0.65f);
                g_pRenderTarget->DrawText(L"SYNCING WITH GITHUB...", 21, pBody,
                    D2D1::RectF(L.margin, cY + 0.22f * L.H, L.W - L.margin, cY + 0.27f * L.H), pBlue);
            } else if (g_updateAvailable) {
                g_pRenderTarget->DrawText(L"Update available! Click below to install.", 40, pBody,
                    D2D1::RectF(L.margin, cY + 0.22f * L.H, L.W - L.margin, cY + 0.27f * L.H), pBlue);
            }

            DrawD2DButton(g_pRenderTarget, btnRect, g_updateAvailable ? L"INSTALL UPDATE NOW" : L"CHECK / DOWNLOAD LATEST", btnCol, 13.0f * baseScale);

        } else if (g_currentTab == 2) {
            // ─ COLORS ───────────────────────────────────────────────────────
            g_pRenderTarget->DrawText(L"ALGORITHM COLOR CONFIG", 22, pHeader,
                D2D1::RectF(L.margin, cY, L.W - L.margin, cY + 0.05f * L.H), pWhite);

            g_pRenderTarget->DrawText(L"Current Target Color:", 21, pBody,
                D2D1::RectF(L.margin, cY + 0.09f * L.H, L.W - L.margin, cY + 0.14f * L.H), pGrey);

            D2D1_RECT_F swatch = D2D1::RectF(L.margin, cY + 0.15f * L.H, L.W - L.margin, cY + 0.32f * L.H);
            ID2D1SolidColorBrush* pSwatch = NULL;
            g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(GetRValue(g_targetColor)/255.0f,
                             GetGValue(g_targetColor)/255.0f,
                             GetBValue(g_targetColor)/255.0f), &pSwatch);
            g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(swatch, 8.0f, 8.0f), pSwatch);
            g_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(swatch, 8.0f, 8.0f), pGrey, 1.0f);
            if (pSwatch) pSwatch->Release();

            wchar_t rgb[48];
            swprintf_s(rgb, L"RGB(%d, %d, %d)", GetRValue(g_targetColor), GetGValue(g_targetColor), GetBValue(g_targetColor));
            g_pRenderTarget->DrawText(rgb, (UINT32)wcslen(rgb), pHeader,
                D2D1::RectF(L.margin, cY + 0.34f * L.H, L.W - L.margin, cY + 0.41f * L.H), pWhite);

            g_pRenderTarget->DrawText(
                L"Press Ctrl+R in-game to open the ROI selector,\nthen click the pixel you want to track.",
                87, pBody,
                D2D1::RectF(L.margin, cY + 0.44f * L.H, L.W - L.margin, cY + 0.55f * L.H), pGrey);

        } else if (g_currentTab == 3) {
            // ─ DEBUG ────────────────────────────────────────────────────────
            g_pRenderTarget->DrawText(L"DEBUG & SIMULATION", 18, pHeader,
                D2D1::RectF(L.margin, cY, L.W - L.margin, cY + 0.04f * L.H), pWhite);

            float bH = 0.06f * L.H;
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin, cY+0.05f*L.H, L.W-L.margin, cY+0.05f*L.H+bH),
                g_debugMode      ? L"SIMULATION [ ON ]"  : L"SIMULATION [ OFF ]",
                g_debugMode      ? D2D1::ColorF(0.0f,0.55f,0.2f) : D2D1::ColorF(0.22f,0.22f,0.22f), 12.0f*baseScale);
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin, cY+0.13f*L.H, L.W-L.margin, cY+0.13f*L.H+bH),
                g_forceDiving    ? L"FORCE DIVING [ ON ]": L"FORCE DIVING [ OFF ]",
                g_forceDiving    ? D2D1::ColorF(0.0f,0.55f,0.2f) : D2D1::ColorF(0.22f,0.22f,0.22f), 12.0f*baseScale);
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin, cY+0.21f*L.H, L.W-L.margin, cY+0.21f*L.H+bH),
                g_forceDetection ? L"FORCE MATCH [ ON ]" : L"FORCE MATCH [ OFF ]",
                g_forceDetection ? D2D1::ColorF(0.0f,0.55f,0.2f) : D2D1::ColorF(0.22f,0.22f,0.22f), 12.0f*baseScale);
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin, cY+0.29f*L.H, L.W-L.margin, cY+0.29f*L.H+bH),
                L"RESET ANGLE TO ZERO", D2D1::ColorF(0.75f,0.35f,0.0f), 12.0f*baseScale);

            g_pRenderTarget->DrawText(L"COLOUR TOLERANCE", 16, pHeader,
                D2D1::RectF(L.margin, 0.55f*L.H, L.W-L.margin, 0.60f*L.H), pWhite);
            DrawD2DButton(g_pRenderTarget,
                D2D1::RectF(L.margin,                          0.60f * L.H, L.margin + L.contentW * 0.45f, 0.67f * L.H),
                L"-  DECREASE", D2D1::ColorF(0.18f, 0.18f, 0.22f), 12.0f * baseScale);
            DrawD2DButton(g_pRenderTarget,
                D2D1::RectF(L.W-L.margin-L.contentW*0.45f, 0.60f*L.H, L.W-L.margin, 0.67f*L.H),
                L"+  INCREASE", D2D1::ColorF(0.18f,0.18f,0.22f), 12.0f*baseScale);

            wchar_t diag[128];
            swprintf_s(diag, L"Diag | Angle = %.2f [deg] | Match = %.0f%%", g_currentAngle, g_detectionRatio * 100.0f);
            g_pRenderTarget->DrawText(diag, (UINT32)wcslen(diag), pBody,
                D2D1::RectF(L.margin, 0.70f*L.H, L.W-L.margin, 0.76f*L.H), pGrey);

        } else if (g_currentTab == 4) {
            // ─ CROSSHAIR ────────────────────────────────────────────────────
            g_pRenderTarget->DrawText(L"PRECISION CROSSHAIR CONFIG", 26, pHeader,
                D2D1::RectF(L.margin, cY, L.W - L.margin, cY + 0.05f * L.H), pWhite);

            float bH  = 0.06f * L.H;
            float bY0 = cY + 0.06f * L.H;
            
            float col1X = L.margin;
            float col1W = L.contentW * 0.45f;
            float col2X = L.margin + L.contentW * 0.55f;
            float col2W = L.contentW * 0.45f;
            
            // Top two control buttons
            DrawD2DButton(g_pRenderTarget,
                D2D1::RectF(col1X, bY0, col1X + col1W, bY0 + bH),
                L"CHOOSE COLOR", D2D1::ColorF(0.12f, 0.15f, 0.22f), 12.0f * baseScale);
            DrawD2DButton(g_pRenderTarget,
                D2D1::RectF(col2X, bY0, col2X + col2W, bY0 + bH),
                g_crossPulse ? L"PULSE  [ ON ]" : L"PULSE  [ OFF ]",
                g_crossPulse  ? D2D1::ColorF(0.45f, 0.1f, 0.55f) : D2D1::ColorF(0.15f, 0.15f, 0.20f),
                12.0f * baseScale);

            // ─ / + setting rows layout ─────────────────────────────────────────────
            float row1Y = cY + 0.16f * L.H;
            float row2Y = cY + 0.28f * L.H;
            float btnW  = col1W * 0.25f;

            auto drawRow = [&](float rowY, float colX, const wchar_t* label, float value) {
                float minX = colX + col1W * 0.4f;
                float plusX = colX + col1W * 0.7f;
                float valX = colX + col1W * 0.98f;
                
                g_pRenderTarget->DrawText(label, (UINT32)wcslen(label), pBody,
                    D2D1::RectF(colX, rowY + 0.01f * L.H, minX, rowY + bH), pGrey);
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(minX, rowY, minX + btnW, rowY + bH),
                    L"–", D2D1::ColorF(0.15f, 0.15f, 0.20f), 14.0f * baseScale);
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(plusX, rowY, plusX + btnW, rowY + bH),
                    L"+", D2D1::ColorF(0.15f, 0.15f, 0.20f), 14.0f * baseScale);
                
                std::wstring val = std::to_wstring((int)value);
                g_pRenderTarget->DrawText(val.c_str(), (UINT32)val.length(), pHeader,
                    D2D1::RectF(valX, rowY + 0.005f * L.H, L.W - L.margin, rowY + bH), pBlue);
            };

            drawRow(row1Y, col1X, L"Thickness:", g_crossThickness);
            drawRow(row1Y, col2X, L"Rotation:", g_crossAngle);
            drawRow(row2Y, col1X, L"Offset X:", g_crossOffsetX);
            drawRow(row2Y, col2X, L"Offset Y:", g_crossOffsetY);

            // Action Buttons (Reset/Save)
            float btnRowY = cY + 0.42f * L.H;
            float actionBtnH = 32.0f * baseScale;
            float abW = (L.contentW - 30) / 3;
            
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin, btnRowY, L.margin + abW, btnRowY + actionBtnH), 
                L"RESET", D2D1::ColorF(0.4f, 0.4f, 0.45f), 11.0f * baseScale);
            
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin + abW + 15, btnRowY, L.margin + 2 * abW + 15, btnRowY + actionBtnH), 
                L"SAVE POS", D2D1::ColorF(0.15f, 0.35f, 0.25f), 11.0f * baseScale);
            
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(L.margin + 2 * abW + 30, btnRowY, L.margin + 3 * abW + 30, btnRowY + actionBtnH), 
                L"SAVE ALL", D2D1::ColorF(0.15f, 0.25f, 0.55f), 11.0f * baseScale);

            g_pRenderTarget->DrawText(L"Press F10 in-game to toggle the crosshair overlay.", 50, pBody,
                D2D1::RectF(L.margin, cY + 0.52f * L.H, L.W - L.margin, cY + 0.58f * L.H), pGrey);
        }

        // ── Footer Quit button ────────────────────────────────────────────────
        DrawD2DButton(g_pRenderTarget,
            D2D1::RectF(L.margin, L.footerY1, L.W - L.margin, L.footerY2),
            L"QUIT SUITE", D2D1::ColorF(0.65f, 0.08f, 0.12f), 13.0f * baseScale);

        // ── Release resources ─────────────────────────────────────────────────
        if (pBody)   pBody->Release();
        if (pHeader) pHeader->Release();
        if (pTitle)  pTitle->Release();
        if (pBlue)   pBlue->Release();
        if (pGrey)   pGrey->Release();
        if (pDark)   pDark->Release();
        if (pWhite)  pWhite->Release();

        HRESULT hr = g_pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            g_pRenderTarget->Release();
            g_pRenderTarget = NULL;
            InitD2D(hWnd);
        }
        ValidateRect(hWnd, NULL);
        return 0;
    }

    case WM_TIMER:
        if (g_isCheckingForUpdates) {
            g_updateSpinAngle += 10.0f;
            if (g_updateSpinAngle >= 360.0f) g_updateSpinAngle = 0.0f;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_MINIMIZE);
        return 0;

    case WM_DESTROY:
        if (g_pRenderTarget) { g_pRenderTarget->Release(); g_pRenderTarget = NULL; }
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}