#include "shared/State.h"
#include "shared/Overlay.h"
#include "shared/ControlPanel.h"
#include "shared/Updater.h"
#include "shared/Profile.h"
#include <thread>
#include <string>
#include <algorithm>
#include <vector>

#include <d3d11.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern Profile g_currentProfile;
extern HWND g_hHUD;
extern AngleLogic g_logic;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern double FetchFortniteSensitivity();

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

void CleanupDeviceD3D()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
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

std::string GetKeyNameStr(UINT mod, UINT vk) {
    std::wstring w = GetKeyName(mod, vk);
    if (w.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string res(sz - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &res[0], sz, NULL, NULL);
    return res;
}

void RenderImGuiFrame() {
    static bool s_inFrame = false;
    if (s_inFrame) return;
    s_inFrame = true;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("BetterAngle Pro Command Center", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    if (g_allProfiles.empty()) {
        ImGui::TextDisabled("No profiles found.");
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::Render();
        s_inFrame = false;
        return;
    }
    Profile& p = g_allProfiles[g_selectedProfileIdx];

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("GENERAL")) {
            ImGui::Spacing();
            ImGui::TextDisabled("HOTKEYS - Click to rebind");
            
            auto BindRow = [](const char* label, int id, UINT& mod, UINT& vk) {
                ImGui::Text("%s", label);
                ImGui::SameLine(200);
                std::string btn = (g_listeningKey == id) ? "[ Press Key... ]" : "[ " + GetKeyNameStr(mod, vk) + " ]";
                if (g_listeningKey == id) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.10f, 1.0f));
                    ImGui::Button((btn + "##" + std::to_string(id)).c_str(), ImVec2(150, 0));
                    ImGui::PopStyleColor();
                } else {
                    if (ImGui::Button((btn + "##" + std::to_string(id)).c_str(), ImVec2(150, 0)))
                        g_listeningKey = id;
                }
            };
            
            BindRow("Toggle Dashboard",   1, p.keybinds.toggleMod, p.keybinds.toggleKey);
            BindRow("ROI Selector",       2, p.keybinds.roiMod,    p.keybinds.roiKey);
            BindRow("Crosshair",          3, p.keybinds.crossMod,  p.keybinds.crossKey);
            BindRow("Zero Angle Reset",   4, p.keybinds.zeroMod,   p.keybinds.zeroKey);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("MANUAL SENSITIVITY");
            ImGui::Spacing();

            double sX = p.sensitivityX;
            double sY = p.sensitivityY;
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputDouble("Fortnite Sens X##sensX", &sX, 0.0, 0.0, "%.4f")) {
                p.sensitivityX = (std::max)(0.001, sX);
                g_logic.LoadProfile(p.sensitivityX);
                p.Save(GetAppStoragePath() + p.name + L".json");
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputDouble("Fortnite Sens Y##sensY", &sY, 0.0, 0.0, "%.4f")) {
                p.sensitivityY = (std::max)(0.001, sY);
                p.Save(GetAppStoragePath() + p.name + L".json");
            }

            ImGui::Spacing();
            static std::string syncResult = "";
            static ImVec4 syncColor = ImVec4(1,1,1,1);
            if (ImGui::Button("SYNC SENSITIVITY WITH FORTNITE", ImVec2(-1, 35))) {
                double synced = FetchFortniteSensitivity();
                if (synced > 0.0) {
                    p.sensitivityX = synced;
                    p.sensitivityY = synced;
                    g_logic.LoadProfile(synced);
                    p.Save(GetAppStoragePath() + p.name + L".json");
                    syncResult = "SYNC OK! sens=" + std::to_string(synced).substr(0,6);
                    syncColor = ImVec4(0.2f, 0.8f, 0.5f, 1.0f);
                } else {
                    syncResult = "CONFIG NOT FOUND! Check: %LOCALAPPDATA%\\FortniteGame\\Saved\\Config\\WindowsClient\\";
                    syncColor = ImVec4(0.85f, 0.2f, 0.2f, 1.0f);
                }
            }
            if (!syncResult.empty())
                ImGui::TextColored(syncColor, "%s", syncResult.c_str());

            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("TERMINATE BetterAngle PRO", ImVec2(-1, 35))) PostQuitMessage(0);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("CROSSHAIR")) {
            ImGui::Spacing();
            if (ImGui::Button(g_showCrosshair ? "CROSSHAIR: ON  (click to toggle)" : "CROSSHAIR: OFF (click to toggle)", ImVec2(-1, 35))) {
                g_showCrosshair = !g_showCrosshair;
                SaveSettings();
                if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
            }
            ImGui::Spacing();
            
            auto PrecisionSlider = [](const char* label, float* val, float minV, float maxV, float step) {
                ImGui::Text("%s", label);
                if (ImGui::Button(("-##" + std::string(label)).c_str(), ImVec2(25, 25))) { *val -= step; SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE); }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
                if (ImGui::SliderFloat(("##" + std::string(label)).c_str(), val, minV, maxV, "%.2f")) {
                    SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE);
                }
                ImGui::SameLine();
                if (ImGui::Button(("+##" + std::string(label)).c_str(), ImVec2(25, 25))) { *val += step; SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE); }
            };

            PrecisionSlider("Thickness", &g_crossThickness, 1.0f, 10.0f, 0.5f);
            PrecisionSlider("Offset X",  &g_crossOffsetX,  -500.0f, 500.0f, 1.0f);
            PrecisionSlider("Offset Y",  &g_crossOffsetY,  -500.0f, 500.0f, 1.0f);

            ImGui::Spacing();
            ImGui::TextDisabled("DIRECTIONAL NUDGE (PIXEL-PERFECT)");
            float nudgeW = 70.0f;
            float centerOffset = (ImGui::GetContentRegionAvail().x - nudgeW) * 0.5f;
            ImGui::SetCursorPosX(centerOffset);
            if (ImGui::Button("UP",    ImVec2(nudgeW, 30))) { g_crossOffsetY -= 1.0f; SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE); }
            ImGui::SetCursorPosX(centerOffset - nudgeW - 8);
            if (ImGui::Button("LEFT",  ImVec2(nudgeW, 30))) { g_crossOffsetX -= 1.0f; SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE); }
            ImGui::SameLine();
            if (ImGui::Button("DOWN",  ImVec2(nudgeW, 30))) { g_crossOffsetY += 1.0f; SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE); }
            ImGui::SameLine();
            if (ImGui::Button("RIGHT", ImVec2(nudgeW, 30))) { g_crossOffsetX += 1.0f; SaveSettings(); if (g_hHUD) InvalidateRect(g_hHUD, NULL, FALSE); }

            ImGui::Spacing();
            ImGui::Checkbox("Pulse Animation", &g_crossPulse);
            ImGui::SameLine();
            float col[3] = { GetRValue(g_crossColor)/255.f, GetGValue(g_crossColor)/255.f, GetBValue(g_crossColor)/255.f };
            if (ImGui::ColorEdit3("Colour", col, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs))
                g_crossColor = RGB(int(col[0]*255), int(col[1]*255), int(col[2]*255));

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("COLORS")) {
            ImGui::Spacing();
            ImGui::TextDisabled("TARGET COLOR (detection color)");
            ImGui::Spacing();
            float tc[3] = { GetRValue(g_targetColor)/255.f, GetGValue(g_targetColor)/255.f, GetBValue(g_targetColor)/255.f };
            if (ImGui::ColorEdit3("Target Color##tc", tc)) {
                g_targetColor = RGB(int(tc[0]*255), int(tc[1]*255), int(tc[2]*255));
                if (!g_allProfiles.empty()) {
                    p.target_color = g_targetColor;
                    p.Save(GetAppStoragePath() + p.name + L".json");
                }
            }
            ImGui::Spacing();
            ImGui::TextDisabled("Tolerance (color match ±)");
            if (!g_allProfiles.empty()) {
                ImGui::SetNextItemWidth(-1);
                if (ImGui::SliderInt("##tolerance", &p.tolerance, 0, 120))
                    p.Save(GetAppStoragePath() + p.name + L".json");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("DEBUG")) {
            ImGui::Spacing();
            ImGui::Checkbox("Debug Overlay (Ctrl+9)", &g_debugMode);
            ImGui::Checkbox("Force Diving State",     &g_forceDiving);
            ImGui::Checkbox("Force Detection Active", &g_forceDetection);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("DIVE THRESHOLDS");
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Glide Threshold##gt",    &g_glideThreshold,    0.01f, 0.5f, "%.3f");
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Freefall Threshold##ft", &g_freefallThreshold, 0.01f, 0.5f, "%.3f");
            ImGui::Spacing();
            if (ImGui::Button("SAVE THRESHOLDS", ImVec2(-1, 35))) SaveSettings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UPDATES")) {
            ImGui::Spacing();
            ImGui::Text("Version: %s", VERSION_STR);
            if (g_hasCheckedForUpdates) {
                ImGui::Text("Latest:  %s", g_latestVersionOnline.c_str());
                if (g_updateAvailable && ImGui::Button("DOWNLOAD UPDATE NOW", ImVec2(-1, 40))) UpdateApp();
                else if (!g_updateAvailable) ImGui::TextDisabled("You are up to date!");
            } else {
                if (ImGui::Button("CHECK FOR UPDATES", ImVec2(-1, 35))) { std::thread([]() { CheckForUpdates(); }).detach(); }
            }
            if (g_isDownloadingUpdate) ImGui::TextColored(ImVec4(0.3f,0.8f,1.0f,1.0f), "Downloading update...");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::Render();
    const float clear_color[4] = { 0.05f, 0.05f, 0.06f, 1.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
    s_inFrame = false;
}

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && g_listeningKey != -1) {
        if (wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU || wParam == VK_ESCAPE) {
            if (wParam == VK_ESCAPE) g_listeningKey = -1;
            return 0;
        }
        UINT mod = 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
        if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mod |= MOD_SHIFT;
        if (GetAsyncKeyState(VK_MENU)    & 0x8000) mod |= MOD_ALT;
        
        Profile& p = g_allProfiles[g_selectedProfileIdx];
        switch (g_listeningKey) {
            case 1: p.keybinds.toggleMod = mod; p.keybinds.toggleKey = (UINT)wParam; break;
            case 2: p.keybinds.roiMod    = mod; p.keybinds.roiKey    = (UINT)wParam; break;
            case 3: p.keybinds.crossMod  = mod; p.keybinds.crossKey  = (UINT)wParam; break;
            case 4: p.keybinds.zeroMod   = mod; p.keybinds.zeroKey   = (UINT)wParam; break;
        }
        g_listeningKey = -1;
        p.Save(GetAppStoragePath() + p.name + L".json");
        return 0;
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
        case WM_CREATE: {
            if (!CreateDeviceD3D(hWnd)) return -1;
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
            style.Colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            ImGui_ImplWin32_Init(hWnd);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
            SetTimer(hWnd, 1, 16, NULL);
            return 0;
        }
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED && lParam != 0) {
                if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                ID3D11Texture2D* pBackBuffer;
                g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
                g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
                pBackBuffer->Release();
            }
            return 0;
        case WM_TIMER: RenderImGuiFrame(); return 0;
        case WM_CLOSE: ShowWindow(hWnd, SW_MINIMIZE); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND CreateControlPanel(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ControlPanelWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    HWND hPanel = CreateWindowEx(0, L"BetterAngleControlPanel", L"BetterAngle Pro | Command Center",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 450, nullptr, nullptr, hInst, nullptr);
    ShowWindow(hPanel, SW_SHOW);
    return hPanel;
}