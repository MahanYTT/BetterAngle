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

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
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
    return std::string(w.begin(), w.end());
}

void RenderImGuiFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::Begin("BetterAngle Pro Command Center", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "BetterAngle Pro | Command Center");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("MainTabs")) {
        
        // TAB: GENERAL
        if (ImGui::BeginTabItem("GENERAL")) {
            ImGui::Spacing();
            ImGui::TextDisabled("HOTKEYS - Click to rebind");
            ImGui::Spacing();
            
            auto BindRow = [](const char* label, int id, UINT& mod, UINT& vk) {
                ImGui::Text("%s", label);
                ImGui::SameLine(200);
                std::string btn = (g_listeningKey == id) ? "[ Press Key... ]" : "[ " + GetKeyNameStr(mod, vk) + " ]";
                if (g_listeningKey == id) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                    ImGui::Button((btn + "##" + std::to_string(id)).c_str(), ImVec2(150, 0));
                    ImGui::PopStyleColor();
                } else {
                    if (ImGui::Button((btn + "##" + std::to_string(id)).c_str(), ImVec2(150, 0))) {
                        g_listeningKey = id;
                    }
                }
            };
            
            BindRow("Toggle Dashboard", 1, g_keybinds.toggleMod, g_keybinds.toggleKey);
            BindRow("Visual ROI Selector", 2, g_keybinds.roiMod, g_keybinds.roiKey);
            BindRow("Precision Crosshair", 3, g_keybinds.crossMod, g_keybinds.crossKey);
            BindRow("Zero Angle Reset", 4, g_keybinds.zeroMod, g_keybinds.zeroKey);
            BindRow("Secret Debug Tab", 5, g_keybinds.debugMod, g_keybinds.debugKey);
            ImGui::EndTabItem();
        }

        // TAB: UPDATES
        if (ImGui::BeginTabItem("UPDATES")) {
            ImGui::Spacing();
            ImGui::TextDisabled("SOFTWARE DASHBOARD");
            ImGui::Spacing();
            std::wstring wsCur = VERSION_WSTR;
            ImGui::Text("Installed: v%s", std::string(wsCur.begin(), wsCur.end()).c_str());
            ImGui::Text("Latest: %s", g_latestVersionOnline.empty() ? "(not checked)" : g_latestVersionOnline.c_str());
            
            ImGui::Spacing();
            if (g_isCheckingForUpdates) {
                ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "SYNCING WITH GITHUB...");
            } else if (g_updateAvailable) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Update available! Click below to install.");
                if (ImGui::Button("INSTALL UPDATE NOW", ImVec2(250, 40))) {
                    std::thread([]() {
                        wchar_t exeBuf[MAX_PATH] = {}; GetModuleFileNameW(NULL, exeBuf, MAX_PATH);
                        std::wstring exePath = exeBuf; size_t slash = exePath.find_last_of(L"\\/");
                        std::wstring tmpPath = exePath.substr(0, slash + 1) + L"update_tmp.exe";
                        if (DownloadUpdate(L"AUTO", tmpPath)) ApplyUpdateAndRestart();
                        else g_updateAvailable = true;
                    }).detach();
                }
            } else {
                if (ImGui::Button("CHECK / DOWNLOAD LATEST", ImVec2(250, 40))) {
                    g_isCheckingForUpdates = true;
                    std::thread(CheckForUpdates).detach();
                }
            }
            ImGui::EndTabItem();
        }

        // TAB: COLORS
        if (ImGui::BeginTabItem("COLORS")) {
            ImGui::Spacing();
            ImGui::TextDisabled("ALGORITHM COLOR CONFIG");
            ImGui::Spacing();
            float col[3] = { GetRValue(g_targetColor)/255.f, GetGValue(g_targetColor)/255.f, GetBValue(g_targetColor)/255.f };
            if (ImGui::ColorEdit3("Target Color", col)) {
                g_targetColor = RGB((BYTE)(col[0]*255), (BYTE)(col[1]*255), (BYTE)(col[2]*255));
                SaveSettings();
            }
            ImGui::Spacing();
            ImGui::TextWrapped("Press Ctrl+R in-game to open the ROI selector, then click the pixel you want to track.");
            ImGui::EndTabItem();
        }

        // TAB: DEBUG
        if (ImGui::BeginTabItem("DEBUG")) {
            ImGui::Spacing();
            ImGui::TextDisabled("DEBUG & SIMULATION");
            ImGui::Checkbox("SIMULATION MODE", &g_debugMode);
            ImGui::Checkbox("FORCE DIVING", &g_forceDiving);
            ImGui::Checkbox("FORCE MATCH", &g_forceDetection);
            if (ImGui::Button("RESET ANGLE TO ZERO", ImVec2(200, 30))) { g_currentAngle = 0.0f; g_logic.SetZero(); }
            
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextDisabled("COLOR TOLERANCE");
            if (!g_allProfiles.empty()) {
                int tol = g_allProfiles[g_selectedProfileIdx].tolerance;
                if (ImGui::SliderInt("Tolerance", &tol, 0, 100)) {
                    g_allProfiles[g_selectedProfileIdx].tolerance = tol;
                    g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                }
            }
            ImGui::Spacing();
            ImGui::Text("Diag | Angle = %.2f [deg] | Match = %.0f%%", g_currentAngle, g_detectionRatio * 100.0f);
            ImGui::EndTabItem();
        }

        // TAB: CROSSHAIR
        if (ImGui::BeginTabItem("CROSSHAIR")) {
            ImGui::Spacing();
            ImGui::TextDisabled("PRECISION CROSSHAIR CONFIG");
            ImGui::Spacing();
            float cCol[3] = { GetRValue(g_crossColor)/255.f, GetGValue(g_crossColor)/255.f, GetBValue(g_crossColor)/255.f };
            if (ImGui::ColorEdit3("Crosshair Color", cCol)) {
                g_crossColor = RGB((BYTE)(cCol[0]*255), (BYTE)(cCol[1]*255), (BYTE)(cCol[2]*255)); SaveSettings();
            }
            if (ImGui::Checkbox("Pulse Animation", &g_crossPulse)) SaveSettings();
            
            ImGui::Spacing();
            if (ImGui::SliderFloat("Thickness", &g_crossThickness, 1.0f, 10.0f)) SaveSettings();
            if (ImGui::SliderFloat("Rotation", &g_crossAngle, -180.0f, 180.0f)) SaveSettings();
            if (ImGui::SliderFloat("Offset X", &g_crossOffsetX, -500.0f, 500.0f)) SaveSettings();
            if (ImGui::SliderFloat("Offset Y", &g_crossOffsetY, -500.0f, 500.0f)) SaveSettings();
            
            ImGui::Spacing();
            if (ImGui::Button("RESET", ImVec2(100, 30))) {
                g_crossThickness = 2.0f; g_crossOffsetX = 0.0f; g_crossOffsetY = 0.0f;
                g_crossAngle = 0.0f; g_crossPulse = false; g_crossColor = RGB(255,0,0);
            }
            ImGui::SameLine();
            if (ImGui::Button("SAVE POS", ImVec2(100, 30))) {
                if (!g_allProfiles.empty()) {
                    g_allProfiles[g_selectedProfileIdx].crossOffsetX = g_crossOffsetX;
                    g_allProfiles[g_selectedProfileIdx].crossOffsetY = g_crossOffsetY;
                    g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("SAVE ALL", ImVec2(100, 30))) {
                if (!g_allProfiles.empty()) {
                    Profile& p = g_allProfiles[g_selectedProfileIdx];
                    p.crossThickness = g_crossThickness; p.crossColor = g_crossColor;
                    p.crossOffsetX = g_crossOffsetX; p.crossOffsetY = g_crossOffsetY;
                    p.crossAngle = g_crossAngle; p.crossPulse = g_crossPulse;
                    p.Save(GetAppStoragePath() + p.name + L".json");
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    
    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0); // Present with vsync
}

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_CREATE: {
            if (!CreateDeviceD3D(hWnd)) {
                CleanupDeviceD3D();
                return -1;
            }
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.IniFilename = NULL; // Disable ini settings
            ImGui::StyleColorsDark();
            
            // Tweak the style to look professional
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.FrameRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.08f, 1.0f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.22f, 0.28f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.28f, 0.35f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.18f, 0.22f, 1.0f);

            ImGui_ImplWin32_Init(hWnd);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
            SetTimer(hWnd, 1, 16, NULL); // Render loop
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
        case WM_TIMER:
            RenderImGuiFrame();
            return 0;
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
                return 0;
            }
            break;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_CLOSE:
            ShowWindow(hWnd, SW_MINIMIZE);
            return 0;
        case WM_DESTROY:
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            CleanupDeviceD3D();
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND CreateControlPanel(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ControlPanelWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    HWND hPanel = CreateWindowEx(
        0, L"BetterAngleControlPanel", L"BetterAngle Pro | Global Command Center",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInst, nullptr
    );
    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}