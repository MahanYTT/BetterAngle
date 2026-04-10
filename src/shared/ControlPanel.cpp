#include "shared/State.h"
#include "shared/Overlay.h"
#include "shared/Config.h"
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

    int w = 420, h = 580;
    HWND hPanel = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"BetterAngleControlPanel", L"BetterAngle Pro | Global Command Center",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
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

            // Tab Navigation (Y: 90-120)
            if (y >= 90 && y <= 120) {
                if (x >= 40 && x <= 115) { g_currentTab = 0; g_listeningKey = -1; }
                else if (x >= 125 && x <= 200) { g_currentTab = 1; g_listeningKey = -1; }
                else if (x >= 210 && x <= 295) { g_currentTab = 2; g_listeningKey = -1; }
                else if (x >= 305 && x <= 380) { g_currentTab = 3; g_listeningKey = -1; }
            }

            if (g_currentTab == 0 && g_listeningKey == -1) {
                // Keybinds Selection (160 - 280)
                if (x >= 200 && x <= 380) {
                    if (y >= 160 && y <= 180) g_listeningKey = 1;
                    else if (y >= 185 && y <= 205) g_listeningKey = 2;
                    else if (y >= 210 && y <= 230) g_listeningKey = 3;
                    else if (y >= 235 && y <= 255) g_listeningKey = 4;
                    else if (y >= 260 && y <= 280) g_listeningKey = 5;
                }
                
                // Cloud Sync
                if (g_cloudProfileNames.empty() && !g_isCheckingCloud && x >= 40 && x <= 380 && y >= 330 && y <= 370) {
                    std::thread(FetchCloudProfiles).detach();
                } else if (!g_cloudProfileNames.empty() && x >= 40 && x <= 380) {
                    for (int i=0; i<min(5, (int)g_cloudProfileNames.size()); i++) {
                        float cy = 330.0f + (i * 35);
                        if (y >= cy && y <= cy + 30) {
                            DownloadCloudProfile(i);
                            g_lastLoadedProfileName = g_cloudProfileNames[i];
                            // Remove .json for display/internal
                            if (g_lastLoadedProfileName.find(L".json") != std::wstring::npos) {
                                g_lastLoadedProfileName = g_lastLoadedProfileName.substr(0, g_lastLoadedProfileName.length() - 5);
                            }
                            SaveSettings();
                            
                            // Reload Profiles Array
                            g_allProfiles = GetProfiles(GetAppStoragePath());
                            g_selectedProfileIdx = 0;
                            for (size_t pidx = 0; pidx < g_allProfiles.size(); pidx++) {
                                if (g_allProfiles[pidx].name == g_lastLoadedProfileName) {
                                    g_selectedProfileIdx = pidx; break;
                                }
                            }
                            g_currentProfile = g_allProfiles[g_selectedProfileIdx];
                            g_logic.SetScale(g_currentProfile.scale_normal);
                            break;
                        }
                    }
                }
            }

            if (g_currentTab == 3) {
                if (x >= 40 && x <= 380 && y >= 150 && y <= 180) g_debugMode = !g_debugMode;
                else if (x >= 40 && x <= 380 && y >= 190 && y <= 220) g_forceDiving = !g_forceDiving;
                else if (x >= 40 && x <= 380 && y >= 230 && y <= 260) g_forceDetection = !g_forceDetection;
                else if (x >= 40 && x <= 380 && y >= 270 && y <= 300) {
                    g_currentAngle = 0.0f;
                    g_logic.SetZero();
                }
                else if (x >= 40 && x <= 200 && y >= 350 && y <= 380) {
                    if (!g_allProfiles.empty()) {
                        g_allProfiles[g_selectedProfileIdx].tolerance = max(0, g_allProfiles[g_selectedProfileIdx].tolerance - 2);
                        g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                    }
                }
                else if (x >= 220 && x <= 380 && y >= 350 && y <= 380) {
                    if (!g_allProfiles.empty()) {
                        g_allProfiles[g_selectedProfileIdx].tolerance += 2;
                        g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                    }
                }
                else if (x >= 40 && x <= 380 && y >= 450 && y <= 480) {
                    void StartThresholdWizard(HINSTANCE hInstance);
                    StartThresholdWizard((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE));
                }
            }

            if (g_currentTab == 1) {
                if (x >= 40 && x <= 380 && y >= 320 && y <= 370) {
                    if (g_updateAvailable) {
                        g_updateAvailable = false; // Lock immediately to prevent thread spam
                        std::thread([]() {
                            // Use AUTO to trigger the new dynamic GitHub URL logic
                            if (DownloadUpdate(L"AUTO", L"update_tmp.exe")) {
                                ApplyUpdateAndRestart();
                            }
                        }).detach();
                    } else {
                        g_isCheckingForUpdates = true;
                        std::thread(CheckForUpdates).detach();
                    }
                }
                if (g_updateAvailable && x >= 40 && x <= 380 && y >= 380 && y <= 430) {
                    ShellExecuteW(0, L"open", L"https://github.com/MahanYTT/BetterAngle/releases/latest", 0, 0, SW_SHOW);
                }
                if (g_latestVersion > 4.92f && x >= 40 && x <= 380 && y >= 270 && y <= 290) {
                    ShellExecuteW(0, L"open", L"https://github.com/MahanYTT/BetterAngle/releases", 0, 0, SW_SHOW);
                }
            }

            if (x >= 40 && x <= 380 && y >= 520 && y <= 560) {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_PAINT: {
            if (!g_pRenderTarget) return 0;
            g_pRenderTarget->BeginDraw();
            g_pRenderTarget->Clear(D2D1::ColorF(0.02f, 0.03f, 0.04f));

            ID2D1SolidColorBrush* pWhite = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);
            ID2D1SolidColorBrush* pGrey = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &pGrey);
            ID2D1SolidColorBrush* pBlue = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.7f, 1.0f), &pBlue);

            IDWriteTextFormat* pTitleFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"en-us", &pTitleFormat);
            IDWriteTextFormat* pHeaderFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &pHeaderFormat);
            IDWriteTextFormat* pVerFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", &pVerFormat);

            g_pRenderTarget->DrawText(L"Pro Command Center", 18, pTitleFormat, D2D1::RectF(40, 40, 380, 80), pWhite);

            // Draw 4 Tabs
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 90, 115, 120), L"GENERAL", g_currentTab == 0 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(125, 90, 200, 120), L"UPDATES", g_currentTab == 1 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(210, 90, 295, 120), L"COLORS", g_currentTab == 2 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(305, 90, 380, 120), L"DEBUG", g_currentTab == 3 ? D2D1::ColorF(0.2f, 0.25f, 0.3f) : D2D1::ColorF(0.1f, 0.12f, 0.15f));

            if (g_currentTab == 0) {
                g_pRenderTarget->DrawText(L"HOTKEYS (Click to Rebind)", 25, pHeaderFormat, D2D1::RectF(40, 130, 380, 150), pWhite);

                auto drawBind = [&](int id, std::wstring name, UINT mod, UINT vk, float y) {
                    g_pRenderTarget->DrawText(name.c_str(), name.length(), pVerFormat, D2D1::RectF(40, y, 200, y+20), pGrey);
                    std::wstring bindText = (g_listeningKey == id) ? L"[ Press Key... ]" : (L"[ " + GetKeyName(mod, vk) + L" ]");
                    g_pRenderTarget->DrawText(bindText.c_str(), bindText.length(), pVerFormat, D2D1::RectF(210, y, 380, y+20), (g_listeningKey == id) ? pBlue : pWhite);
                };
                drawBind(1, L"Toggle Dashboard:", g_keybinds.toggleMod, g_keybinds.toggleKey, 160);
                drawBind(2, L"Visual ROI Selector:", g_keybinds.roiMod, g_keybinds.roiKey, 185);
                drawBind(3, L"Precision Crosshair:", g_keybinds.crossMod, g_keybinds.crossKey, 210);
                drawBind(4, L"Zero Angle Reset:", g_keybinds.zeroMod, g_keybinds.zeroKey, 235);
                drawBind(5, L"Secret Debug Tab:", g_keybinds.debugMod, g_keybinds.debugKey, 260);

                g_pRenderTarget->DrawText(L"CLOUD PROFILES REPOSITORY", 25, pHeaderFormat, D2D1::RectF(40, 300, 380, 320), pWhite);

                if (g_cloudProfileNames.empty() && !g_isCheckingCloud) {
                    DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 330, 380, 370), L"FETCH GITHUB PROFILES", D2D1::ColorF(0.2f, 0.4f, 0.8f));
                } else if (g_isCheckingCloud) {
                    g_pRenderTarget->DrawText(L"Connecting to GitHub API...", 25, pVerFormat, D2D1::RectF(40, 340, 380, 360), pGrey);
                } else {
                    for (int i=0; i<min(5, (int)g_cloudProfileNames.size()); i++) {
                        float cy = 330.0f + (i * 35);
                        DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, cy, 380, cy+30), g_cloudProfileNames[i].c_str(), D2D1::ColorF(0.2f, 0.2f, 0.25f));
                    }
                }

                std::wstring act = L"Active: " + g_currentProfile.name;
                g_pRenderTarget->DrawText(act.c_str(), act.length(), pVerFormat, D2D1::RectF(40, 495, 380, 515), pBlue);

            } else if (g_currentTab == 1) {
                g_pRenderTarget->DrawText(L"SOFTWARE DASHBOARD", 18, pHeaderFormat, D2D1::RectF(40, 140, 380, 170), pWhite);

                if (g_isCheckingForUpdates) {
                    D2D1_POINT_2F center = D2D1::Point2F(365.0f, 155.0f);
                    g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(g_updateSpinAngle, center));
                    g_pRenderTarget->DrawEllipse(D2D1::Ellipse(center, 6.0f, 6.0f), pBlue, 2.0f);
                    g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
                }

                // Use the VERSION_WSTR macro correctly
                std::wstring curVer = L"Current Version: v" VERSION_WSTR L" (Live Pro Build)";

                std::wstring latestVerStr = std::wstring(g_latestVersionOnline.begin(), g_latestVersionOnline.end());
                std::wstring latestVer = L"Latest Found: " + latestVerStr + L" (" + g_latestName + L")";

                g_pRenderTarget->DrawText(curVer.c_str(), (UINT32)curVer.length(), pVerFormat, D2D1::RectF(40, 170, 380, 190), pGrey);
                g_pRenderTarget->DrawText(latestVer.c_str(), (UINT32)latestVer.length(), pVerFormat, D2D1::RectF(40, 195, 380, 215), pGrey);

                if (g_updateAvailable) {
                    std::wstring changelog = L"BetterAngle v" VERSION_WSTR L" is now available online!\nNewest Stable Version.";
                    g_pRenderTarget->DrawText(changelog.c_str(), (UINT32)changelog.length(), pVerFormat, D2D1::RectF(40, 230, 380, 260), pWhite);
                    std::wstring viewFull = L"View Full Changelog ->";
                    g_pRenderTarget->DrawText(viewFull.c_str(), (UINT32)viewFull.length(), pVerFormat, D2D1::RectF(40, 270, 380, 290), pBlue);
                    DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 380, 380, 430), L"DOWNLOAD UPDATE", D2D1::ColorF(0.0f, 0.5f, 0.8f));
                }
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 320, 380, 370), g_updateAvailable ? L"INSTALL UPDATE NOW" : L"DOWNLOAD AND INSTALL NOW", D2D1::ColorF(0.15f, 0.17f, 0.2f));

            } else if (g_currentTab == 2) {
                g_pRenderTarget->DrawText(L"ALGORITHM COLOR CONFIG", 22, pHeaderFormat, D2D1::RectF(40, 140, 380, 170), pWhite);

                std::wstring statusText;
                D2D1_COLOR_F statusColor;

                if (g_currentSelection == SELECTING_COLOR) {
                    statusText = L"STATUS: CLICK A PIXEL ON YOUR SCREEN";
                    statusColor = D2D1::ColorF(1.0f, 0.8f, 0.0f); // Amber
                } else {
                    statusText = L"STATUS: ACTIVE AND MONITORING";
                    statusColor = D2D1::ColorF(0.0f, 1.0f, 0.5f); // Green
                }

                ID2D1SolidColorBrush* pStatusBrush = NULL;
                g_pRenderTarget->CreateSolidColorBrush(statusColor, &pStatusBrush);
                g_pRenderTarget->DrawText(statusText.c_str(), (UINT32)statusText.length(), pVerFormat, D2D1::RectF(40, 180, 380, 200), pStatusBrush);
                if (pStatusBrush) pStatusBrush->Release();

                g_pRenderTarget->DrawText(L"Current Target Value:", 21, pVerFormat, D2D1::RectF(40, 220, 250, 240), pGrey);

                D2D1_RECT_F swatchRect = D2D1::RectF(40, 250, 380, 310);
                ID2D1SolidColorBrush* pSwatchBrush = NULL;
                g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(GetRValue(g_targetColor)/255.0f, GetGValue(g_targetColor)/255.0f, GetBValue(g_targetColor)/255.0f), &pSwatchBrush);
                g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(swatchRect, 6.0f, 6.0f), pSwatchBrush);
                g_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(swatchRect, 6.0f, 6.0f), pGrey, 1.0f);
                if (pSwatchBrush) pSwatchBrush->Release();

                wchar_t colorInfo[64];
                swprintf_s(colorInfo, L"RGB: %d, %d, %d", GetRValue(g_targetColor), GetGValue(g_targetColor), GetBValue(g_targetColor));
                g_pRenderTarget->DrawText(colorInfo, (UINT32)wcslen(colorInfo), pHeaderFormat, D2D1::RectF(40, 320, 380, 350), pWhite);

                g_pRenderTarget->DrawText(L"WORKFLOW GUIDE:", 16, pVerFormat, D2D1::RectF(40, 380, 380, 400), pWhite);
                std::wstring instructions = L"1. Press CTRL+R to start ROI selection.\n"
                                            L"2. Drag a box over your target area.\n"
                                            L"3. Release the mouse and click the exact color you want the app to track.";
                g_pRenderTarget->DrawText(instructions.c_str(), (UINT32)instructions.length(), pVerFormat, D2D1::RectF(40, 410, 380, 500), pGrey);
            } else if (g_currentTab == 3) {
                g_pRenderTarget->DrawText(L"DEBUG & SIMULATION", 18, pHeaderFormat, D2D1::RectF(40, 130, 380, 150), pWhite);

                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 150, 380, 180), g_debugMode ? L"Simulation [ ON ]" : L"Simulation [ OFF ]", g_debugMode ? D2D1::ColorF(0.0f, 0.6f, 0.2f) : D2D1::ColorF(0.3f, 0.3f, 0.3f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 190, 380, 220), g_forceDiving ? L"Force Diving [ ON ]" : L"Force Diving [ OFF ]", g_forceDiving ? D2D1::ColorF(0.0f, 0.6f, 0.2f) : D2D1::ColorF(0.3f, 0.3f, 0.3f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 230, 380, 260), g_forceDetection ? L"Force Color Match [ ON ]" : L"Force Color Match [ OFF ]", g_forceDetection ? D2D1::ColorF(0.0f, 0.6f, 0.2f) : D2D1::ColorF(0.3f, 0.3f, 0.3f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 270, 380, 300), L"Reset Angle to Zero", D2D1::ColorF(0.8f, 0.4f, 0.0f));

                g_pRenderTarget->DrawText(L"COLOR TOLERANCE", 16, pHeaderFormat, D2D1::RectF(40, 320, 380, 340), pWhite);
                
                int currentTolerance = 0;
                if (!g_allProfiles.empty()) currentTolerance = g_allProfiles[g_selectedProfileIdx].tolerance;

                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 350, 200, 380), L"- DECREASE", D2D1::ColorF(0.2f, 0.2f, 0.2f));
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(220, 350, 380, 380), L"+ INCREASE", D2D1::ColorF(0.2f, 0.2f, 0.2f));

                wchar_t diagText[128];
                swprintf_s(diagText, L"Diag: Angle=%.1f, Match=%.0f%%", g_currentAngle, g_detectionRatio * 100.0f);
                g_pRenderTarget->DrawText(diagText, (UINT32)wcslen(diagText), pVerFormat, D2D1::RectF(40, 390, 380, 410), pGrey);

                g_pRenderTarget->DrawText(L"AUTO THRESHOLD CALIBRATOR", 16, pHeaderFormat, D2D1::RectF(40, 420, 380, 440), pWhite);
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 450, 380, 480), L"LAUNCH HUD WIZARD", D2D1::ColorF(0.7f, 0.5f, 0.0f));
            }

            DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 520, 380, 560), L"QUIT SUITE", D2D1::ColorF(0.7f, 0.1f, 0.15f));

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