#include "shared/State.h"
#include "shared/Overlay.h"
#include "shared/ControlPanel.h"
#include "shared/Updater.h"
#include "shared/Profile.h"
#include <thread>
#include <d2d1.h>
#include <dwrite.h>
#include <string>

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern Profile g_currentProfile;

#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace D2D1;

ID2D1Factory* g_pD2DFactory = NULL;
IDWriteFactory* g_pDWriteFactory = NULL;
ID2D1HwndRenderTarget* g_pRenderTarget = NULL;

// UI State
// extern declared in State.h

void InitD2D(HWND hWnd) {
    if (!g_pD2DFactory) {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_pDWriteFactory);
    }
    RECT rc; GetClientRect(hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    if (g_pRenderTarget) g_pRenderTarget->Release();
    g_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &g_pRenderTarget);
}

void DrawD2DButton(ID2D1HwndRenderTarget* rt, D2D1_RECT_F rect, const wchar_t* text, D2D1_COLOR_F color) {
    ID2D1SolidColorBrush* pBrush = NULL;
    rt->CreateSolidColorBrush(color, &pBrush);

    ID2D1SolidColorBrush* pStroke = NULL;
    rt->CreateSolidColorBrush(D2D1::ColorF(color.r * 1.5f, color.g * 1.5f, color.b * 1.5f), &pStroke);

    rt->FillRoundedRectangle(D2D1::RoundedRect(rect, 6.0f, 6.0f), pBrush);
    rt->DrawRoundedRectangle(D2D1::RoundedRect(rect, 6.0f, 6.0f), pStroke, 1.0f);

    ID2D1SolidColorBrush* pWhite = NULL;
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);

    IDWriteTextFormat* pTextFormat = NULL;
    g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", &pTextFormat);
    pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    rt->DrawText(text, (UINT32)wcslen(text), pTextFormat, rect, pWhite);

    pTextFormat->Release();
    pWhite->Release();
    if (pStroke) pStroke->Release();
    pBrush->Release();
}

HWND CreateControlPanel(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ControlPanelWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    int w = 540, h = 580;
    HWND hPanel = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"BetterAngleControlPanel", L"BetterAngle Pro | Global Command Center",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}

int g_listeningKey = -1;

std::wstring GetKeyName(UINT mod, UINT vk) {
    if (vk == 0) return L"Unbound";
    std::wstring n = L"";
    if (mod & MOD_CONTROL) n += L"Ctrl + ";
    if (mod & MOD_SHIFT) n += L"Shift + ";
    if (mod & MOD_ALT) n += L"Alt + ";
    
    if (vk >= 'A' && vk <= 'Z') n += (wchar_t)vk;
    else if (vk >= '0' && vk <= '9') n += (wchar_t)vk;
    else if (vk >= VK_F1 && vk <= VK_F12) n += L"F" + std::to_wstring(vk - VK_F1 + 1);
    else n += L"Key(" + std::to_wstring(vk) + L")";
    return n;
}

extern HWND g_hHUD;

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            InitD2D(hWnd);
            SetTimer(hWnd, 1, 16, NULL); // 60FPS Refresh Rate
            return 0;
        case WM_SIZE:
            InitD2D(hWnd);
            return 0;
        case WM_KEYDOWN:
            if (g_listeningKey != -1) {
                if (wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU) return 0;
                
                UINT mod = 0;
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000) mod |= MOD_SHIFT;
                if (GetAsyncKeyState(VK_MENU) & 0x8000) mod |= MOD_ALT;
                
                if (g_listeningKey == 1) { g_keybinds.toggleMod = mod; g_keybinds.toggleKey = wParam; }
                if (g_listeningKey == 2) { g_keybinds.roiMod = mod; g_keybinds.roiKey = wParam; }
                if (g_listeningKey == 3) { g_keybinds.crossMod = mod; g_keybinds.crossKey = wParam; }
                if (g_listeningKey == 4) { g_keybinds.zeroMod = mod; g_keybinds.zeroKey = wParam; }
                if (g_listeningKey == 5) { g_keybinds.debugMod = mod; g_keybinds.debugKey = wParam; }
                
                g_listeningKey = -1;
                SaveSettings();
                
                if (g_hHUD) {
                    UnregisterHotKey(g_hHUD, 1); UnregisterHotKey(g_hHUD, 2);
                    UnregisterHotKey(g_hHUD, 3); UnregisterHotKey(g_hHUD, 4);
                    UnregisterHotKey(g_hHUD, 5);
                    RegisterHotKey(g_hHUD, 1, g_keybinds.toggleMod, g_keybinds.toggleKey);
                    RegisterHotKey(g_hHUD, 2, g_keybinds.roiMod, g_keybinds.roiKey);
                    RegisterHotKey(g_hHUD, 3, g_keybinds.crossMod, g_keybinds.crossKey);
                    RegisterHotKey(g_hHUD, 4, g_keybinds.zeroMod, g_keybinds.zeroKey);
                    RegisterHotKey(g_hHUD, 5, g_keybinds.debugMod, g_keybinds.debugKey);
                }
            }
            return 0;
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            RECT rc; GetClientRect(hWnd, &rc);
            float W = (float)(rc.right - rc.left);
            float H = (float)(rc.bottom - rc.top);
            float margin = 0.07f * W;
            float contentW = W - (margin * 2.0f);

            // Tab Navigation (Y: 15% to 22% of H)
            float tY1 = 0.15f * H, tY2 = 0.22f * H;
            if (y >= tY1 && y <= tY2) {
                float tabW = (contentW - 40.0f) / 5.0f;
                if (x >= margin && x <= margin + tabW) { g_currentTab = 0; g_listeningKey = -1; }
                else if (x >= margin + tabW + 10 && x <= margin + 2 * tabW + 10) { g_currentTab = 1; g_listeningKey = -1; }
                else if (x >= margin + 2 * tabW + 20 && x <= margin + 3 * tabW + 20) { g_currentTab = 2; g_listeningKey = -1; }
                else if (x >= margin + 3 * tabW + 30 && x <= margin + 4 * tabW + 30) { g_currentTab = 3; g_listeningKey = -1; }
                else if (x >= margin + 4 * tabW + 40 && x <= W - margin) { g_currentTab = 4; g_listeningKey = -1; }
            }

            float cY = 0.25f * H; // Content Start
            if (g_currentTab == 0 && g_listeningKey == -1) {
                if (x >= margin + (contentW * 0.6f) && x <= W - margin) {
                    if (y >= cY + 0.05f * H && y <= cY + 0.08f * H) g_listeningKey = 1;
                    else if (y >= cY + 0.09f * H && y <= cY + 0.12f * H) g_listeningKey = 2;
                    else if (y >= cY + 0.13f * H && y <= cY + 0.16f * H) g_listeningKey = 3;
                    else if (y >= cY + 0.17f * H && y <= cY + 0.20f * H) g_listeningKey = 4;
                    else if (y >= cY + 0.21f * H && y <= cY + 0.24f * H) g_listeningKey = 5;
                }
                
                if (x >= margin && x <= W - margin && y >= 0.65f * H && y <= 0.72f * H) {
                    void ShowFirstTimeSetup(HINSTANCE hInstance);
                    ShowFirstTimeSetup((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE));
                }
                if (x >= margin && x <= W - margin && y >= 0.74f * H && y <= 0.81f * H) {
                    void StartThresholdWizard(HINSTANCE hInstance);
                    StartThresholdWizard((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE));
                }
            }

            if (g_currentTab == 3) {
                if (x >= margin && x <= W - margin && y >= cY + 0.05f * H && y <= cY + 0.10f * H) g_debugMode = !g_debugMode;
                else if (x >= margin && x <= W - margin && y >= cY + 0.12f * H && y <= cY + 0.17f * H) g_forceDiving = !g_forceDiving;
                else if (x >= margin && x <= W - margin && y >= cY + 0.19f * H && y <= cY + 0.24f * H) g_forceDetection = !g_forceDetection;
                else if (x >= margin && x <= W - margin && y >= cY + 0.26f * H && y <= cY + 0.31f * H) {
                    g_currentAngle = 0.0f;
                    g_logic.SetZero();
                }
                else if (x >= margin && x <= margin + (contentW * 0.45f) && y >= 0.60f * H && y <= 0.65f * H) {
                    if (!g_allProfiles.empty()) {
                        g_allProfiles[g_selectedProfileIdx].tolerance = max(0, g_allProfiles[g_selectedProfileIdx].tolerance - 2);
                        g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                    }
                }
                else if (x >= margin + (contentW * 0.55f) && x <= W - margin && y >= 0.60f * H && y <= 0.65f * H) {
                    if (!g_allProfiles.empty()) {
                        g_allProfiles[g_selectedProfileIdx].tolerance += 2;
                        g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                    }
                }
            }

            if (g_currentTab == 4) {
                if (x >= margin && x <= margin + (contentW * 0.45f) && y >= cY + 0.05f * H && y <= cY + 0.10f * H) { // Color Picker
                    CHOOSECOLOR cc;
                    static COLORREF acrCustClr[16];
                    ZeroMemory(&cc, sizeof(cc));
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hWnd;
                    cc.lpCustColors = (LPDWORD)acrCustClr;
                    cc.rgbResult = g_crossColor;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColor(&cc)) {
                        g_crossColor = cc.rgbResult;
                    }
                }
                else if (x >= margin + (contentW * 0.55f) && x <= W - margin && y >= cY + 0.05f * H && y <= cY + 0.10f * H) { // Pulse Toggle
                    g_crossPulse = !g_crossPulse;
                }
                else if (x >= margin + 160 && x <= margin + 210 && y >= cY + 0.12f * H && y <= cY + 0.17f * H) g_crossThickness = max(1.0f, g_crossThickness - 1.0f);
                else if (x >= margin + 220 && x <= margin + 270 && y >= cY + 0.12f * H && y <= cY + 0.17f * H) g_crossThickness += 1.0f;
                
                else if (x >= margin + 160 && x <= margin + 210 && y >= cY + 0.19f * H && y <= cY + 0.24f * H) g_crossOffsetX -= 1.0f;
                else if (x >= margin + 220 && x <= margin + 270 && y >= cY + 0.19f * H && y <= cY + 0.24f * H) g_crossOffsetX += 1.0f;
                
                else if (x >= margin + 160 && x <= margin + 210 && y >= cY + 0.26f * H && y <= cY + 0.31f * H) g_crossOffsetY -= 1.0f;
                else if (x >= margin + 220 && x <= margin + 270 && y >= cY + 0.26f * H && y <= cY + 0.31f * H) g_crossOffsetY += 1.0f;
                
                else if (x >= margin + 160 && x <= margin + 210 && y >= cY + 0.33f * H && y <= cY + 0.38f * H) g_crossAngle -= 5.0f;
                else if (x >= margin + 220 && x <= margin + 270 && y >= cY + 0.33f * H && y <= cY + 0.38f * H) g_crossAngle += 5.0f;
            }

            if (g_currentTab == 1) {
                if (x >= margin && x <= W - margin && y >= 0.55f * H && y <= 0.65f * H) {
                    if (g_updateAvailable) {
                        g_updateAvailable = false;
                        std::thread([]() {
                            if (DownloadUpdate(L"AUTO", L"update_tmp.exe")) {
                                ApplyUpdateAndRestart();
                            }
                        }).detach();
                    } else {
                        g_isCheckingForUpdates = true;
                        std::thread(CheckForUpdates).detach();
                    }
                }
            }

            if (x >= margin && x <= W - margin && y >= 0.88f * H && y <= 0.96f * H) {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_PAINT: {
            if (!g_pRenderTarget) return 0;
            g_pRenderTarget->BeginDraw();
            g_pRenderTarget->Clear(D2D1::ColorF(0.02f, 0.03f, 0.04f));

            D2D1_SIZE_F rtSize = g_pRenderTarget->GetSize();
            float W = rtSize.width;
            float H = rtSize.height;
            float margin = 0.07f * W;
            float contentW = W - (margin * 2.0f);
            float baseScale = min(W / 540.0f, H / 580.0f);
            if (baseScale < 0.6f) baseScale = 0.6f;

            ID2D1SolidColorBrush* pWhite = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);
            ID2D1SolidColorBrush* pGrey = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &pGrey);
            ID2D1SolidColorBrush* pBlue = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.7f, 1.0f), &pBlue);

            IDWriteTextFormat* pTitleFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f * baseScale, L"en-us", &pTitleFormat);
            IDWriteTextFormat* pHeaderFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f * baseScale, L"en-us", &pHeaderFormat);
            IDWriteTextFormat* pVerFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f * baseScale, L"en-us", &pVerFormat);

            g_pRenderTarget->DrawText(L"Pro Command Center", 18, pTitleFormat, D2D1::RectF(margin, 0.07f * H, W - margin, 0.14f * H), pWhite);

            // Draw 5 Tabs
            float tY1 = 0.15f * H, tY2 = 0.22f * H;
            float tabW = (contentW - 40.0f) / 5.0f;
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, tY1, margin + tabW, tY2), L"GENERAL", g_currentTab == 0 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin + tabW + 10, tY1, margin + 2 * tabW + 10, tY2), L"UPDATES", g_currentTab == 1 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin + 2 * tabW + 20, tY1, margin + 3 * tabW + 20, tY2), L"COLORS", g_currentTab == 2 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin + 3 * tabW + 30, tY1, margin + 4 * tabW + 30, tY2), L"DEBUG", g_currentTab == 3 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin + 4 * tabW + 40, tY1, W - margin, tY2), L"CROSSHAIR", g_currentTab == 4 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));

            float cY = 0.25f * H; // Content Start
            if (g_currentTab == 0) {
                g_pRenderTarget->DrawText(L"HOTKEYS (Click to Rebind)", 25, pHeaderFormat, D2D1::RectF(margin, cY, W - margin, cY + 0.04f * H), pWhite);

                auto drawBind = [&](int id, std::wstring name, UINT mod, UINT vk, float y) {
                    g_pRenderTarget->DrawText(name.c_str(), name.length(), pVerFormat, D2D1::RectF(margin, y, margin + (contentW * 0.6f), y + 0.035f * H), pGrey);
                    std::wstring bindText = (g_listeningKey == id) ? L"[ Press Key... ]" : (L"[ " + GetKeyName(mod, vk) + L" ]");
                    g_pRenderTarget->DrawText(bindText.c_str(), bindText.length(), pVerFormat, D2D1::RectF(margin + (contentW * 0.62f), y, W - margin, y + 0.035f * H), (g_listeningKey == id) ? pBlue : pWhite);
                };
                drawBind(1, L"Toggle Dashboard:", g_keybinds.toggleMod, g_keybinds.toggleKey, cY + 0.05f * H);
                drawBind(2, L"Visual ROI Selector:", g_keybinds.roiMod, g_keybinds.roiKey, cY + 0.09f * H);
                drawBind(3, L"Precision Crosshair:", g_keybinds.crossMod, g_keybinds.crossKey, cY + 0.13f * H);
                drawBind(4, L"Zero Angle Reset:", g_keybinds.zeroMod, g_keybinds.zeroKey, cY + 0.17f * H);
                drawBind(5, L"Secret Debug Tab:", g_keybinds.debugMod, g_keybinds.debugKey, cY + 0.21f * H);

                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, 0.65f * H, W - margin, 0.72f * H), L"RECALIBRATE BASE SETTINGS", D2D1::ColorF(0.6f, 0.2f, 0.2f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, 0.74f * H, W - margin, 0.81f * H), L"MASTER CALIBRATION WIZARD", D2D1::ColorF(0.8f, 0.4f, 0.0f));

            } else if (g_currentTab == 1) {
                g_pRenderTarget->DrawText(L"SOFTWARE DASHBOARD", 18, pHeaderFormat, D2D1::RectF(margin, cY, W - margin, cY + 0.05f * H), pWhite);
                std::wstring curVer = L"Current Version: v" VERSION_WSTR;
                g_pRenderTarget->DrawText(curVer.c_str(), (UINT32)curVer.length(), pVerFormat, D2D1::RectF(margin, cY + 0.1f * H, W - margin, cY + 0.14f * H), pGrey);
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, 0.45f * H, W - margin, 0.55f * H), L"CHECK FOR UPDATES", D2D1::ColorF(0.15f, 0.17f, 0.2f));

            } else if (g_currentTab == 2) {
                g_pRenderTarget->DrawText(L"ALGORITHM COLOR CONFIG", 22, pHeaderFormat, D2D1::RectF(margin, cY, W - margin, cY + 0.05f * H), pWhite);
                D2D1_RECT_F swatchRect = D2D1::RectF(margin, 0.45f * H, W - margin, 0.55f * H);
                ID2D1SolidColorBrush* pSwatchBrush = NULL;
                g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(g_targetColor)/255.0f, GetGValue(g_targetColor)/255.0f, GetBValue(g_targetColor)/255.0f), &pSwatchBrush);
                g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(swatchRect, 6.0f, 6.0f), pSwatchBrush);
                if (pSwatchBrush) pSwatchBrush->Release();

            } else if (g_currentTab == 3) {
                g_pRenderTarget->DrawText(L"DEBUG & SIMULATION", 18, pHeaderFormat, D2D1::RectF(margin, cY, W - margin, cY + 0.04f * H), pWhite);
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, cY + 0.05f * H, W - margin, cY + 0.10f * H), g_debugMode ? L"Simulation [ ON ]" : L"Simulation [ OFF ]", g_debugMode ? D2D1::ColorF(0.0f, 0.6f, 0.2f) : D2D1::ColorF(0.3f, 0.3f, 0.3f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, cY + 0.12f * H, W - margin, cY + 0.17f * H), g_forceDiving ? L"Force Diving [ ON ]" : L"Force Diving [ OFF ]", g_forceDiving ? D2D1::ColorF(0.0f, 0.6f, 0.2f) : D2D1::ColorF(0.3f, 0.3f, 0.3f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, cY + 0.19f * H, W - margin, cY + 0.24f * H), g_forceDetection ? L"Force Match [ ON ]" : L"Force Match [ OFF ]", g_forceDetection ? D2D1::ColorF(0.0f, 0.6f, 0.2f) : D2D1::ColorF(0.3f, 0.3f, 0.3f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, 0.60f * H, margin + (contentW * 0.45f), 0.65f * H), L"- TOLERANCE", D2D1::ColorF(0.2f, 0.2f, 0.2f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(W - margin - (contentW * 0.45f), 0.60f * H, W - margin, 0.65f * H), L"+ TOLERANCE", D2D1::ColorF(0.2f, 0.2f, 0.2f));

            } else if (g_currentTab == 4) {
                g_pRenderTarget->DrawText(L"PRECISION CROSSHAIR CONFIG", 26, pHeaderFormat, D2D1::RectF(margin, cY, W - margin, cY + 0.04f * H), pWhite);
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, cY + 0.05f * H, margin + (contentW * 0.45f), cY + 0.10f * H), L"CHOOSE COLOR", D2D1::ColorF(0.15f, 0.17f, 0.2f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(W - margin - (contentW * 0.45f), cY + 0.05f * H, W - margin, cY + 0.10f * H), g_crossPulse ? L"PULSE: ON" : L"PULSE: OFF", g_crossPulse ? D2D1::ColorF(0.5f, 0.1f, 0.5f) : D2D1::ColorF(0.15f, 0.17f, 0.2f));
                
                auto drawSetting = [&](std::wstring label, std::wstring val, float y) {
                    g_pRenderTarget->DrawText(label.c_str(), label.length(), pVerFormat, D2D1::RectF(margin, y, margin + 160 * baseScale, y + 0.05f * H), pWhite);
                    DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin + 170 * baseScale, y, margin + 220 * baseScale, y + 0.05f * H), L"-", D2D1::ColorF(0.15f, 0.17f, 0.2f));
                    DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin + 230 * baseScale, y, margin + 280 * baseScale, y + 0.05f * H), L"+", D2D1::ColorF(0.15f, 0.17f, 0.2f));
                    g_pRenderTarget->DrawText(val.c_str(), val.length(), pVerFormat, D2D1::RectF(margin + 290 * baseScale, y, W - margin, y + 0.05f * H), pBlue);
                };
                drawSetting(L"Thickness:", std::to_wstring((int)g_crossThickness), cY + 0.12f * H);
                drawSetting(L"Offset X:", std::to_wstring((int)g_crossOffsetX), cY + 0.19f * H);
                drawSetting(L"Offset Y:", std::to_wstring((int)g_crossOffsetY), cY + 0.26f * H);
                drawSetting(L"Rotation:", std::to_wstring((int)g_crossAngle), cY + 0.33f * H);
            }

            DrawD2DButton(g_pRenderTarget, D2D1::RectF(margin, 0.88f * H, W - margin, 0.96f * H), L"QUIT SUITE", D2D1::ColorF(0.7f, 0.1f, 0.15f));

            pVerFormat->Release();
            pHeaderFormat->Release();
            pTitleFormat->Release();
            pBlue->Release();
            pGrey->Release();
            pWhite->Release();
            g_pRenderTarget->EndDraw();
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
            if (g_pRenderTarget) g_pRenderTarget->Release();
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}