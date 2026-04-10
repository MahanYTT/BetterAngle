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

// Forward declare message handler from imgui_impl_win32.cpp
// Forward declare from Logic.cpp
extern double FetchFortniteSensitivity();
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

// 360 Calibration State
static bool      g_calibrating = false;
static long long g_calStartDx  = 0;
static float     g_calDetectedSens = 0;

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
    if (s_inFrame) return; // Prevent re-entrancy during modal loops (e.g. Setup)
    s_inFrame = true;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("BetterAngle Pro Command Center", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.00f), "BetterAngle Pro | Command Center");
    ImGui::Spacing();

    if (g_allProfiles.empty()) {
        ImGui::TextDisabled("No profiles found. Please run the setup wizard.");
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::Render();
        s_inFrame = false;
        return;
    }
    Profile& p = g_allProfiles[g_selectedProfileIdx];

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
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                    ImGui::Button((btn + "##" + std::to_string(id)).c_str(), ImVec2(150, 0));
                    ImGui::PopStyleColor();
                } else {
                    if (ImGui::Button((btn + "##" + std::to_string(id)).c_str(), ImVec2(150, 0))) {
                        g_listeningKey = id;
                    }
                }
            };
            
            BindRow("Toggle Dashboard", 1, p.keybinds.toggleMod, p.keybinds.toggleKey);
            BindRow("Visual ROI Selector", 2, p.keybinds.roiMod, p.keybinds.roiKey);
            BindRow("Precision Crosshair", 3, p.keybinds.crossMod, p.keybinds.crossKey);
            BindRow("Zero Angle Reset", 4, p.keybinds.zeroMod, p.keybinds.zeroKey);
            BindRow("Secret Debug Tab", 5, p.keybinds.debugMod, p.keybinds.debugKey);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "MANUAL SENSITIVITY");
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
                        syncResult = "SYNC SUCCESSFUL! (sens=" + std::to_string(synced).substr(0,6) + ")";
                        syncColor = ImVec4(0.2f, 0.8f, 0.5f, 1.0f);
                    } else {
                        syncResult = "CONFIG NOT FOUND! Check: %LOCALAPPDATA%\\FortniteGame\\Saved\\Config\\WindowsClient\\";
                        syncColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                    }
                }
                if (!syncResult.empty()) {
                    ImGui::TextWrapped("%s", syncResult.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(syncColor, " ");
                }
                
            // v4.20.37: 360 Calibration Assistant Removed (Per User Request)

            ImGui::Spacing();
            if (ImGui::Button("RE-RUN INITIAL CALIBRATION", ImVec2(-1, 35))) {
                extern void ShowFirstTimeSetup(HINSTANCE);
                ShowFirstTimeSetup(GetModuleHandle(NULL));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("TERMINATE BetterAngle PRO", ImVec2(-1, 40))) {
                PostQuitMessage(0);
            }
            ImGui::PopStyleColor(2);

            ImGui::EndTabItem();
        }

        // TAB: DEBUG
        if (ImGui::BeginTabItem("DEBUG")) {
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
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("DEBUG & SIMULATION");
            ImGui::Checkbox("SIMULATION MODE", &g_debugMode);
            ImGui::Checkbox("FORCE DIVING", &g_forceDiving);
            ImGui::Checkbox("FORCE MATCH", &g_forceDetection);
            if (ImGui::Button("RESET ANGLE TO ZERO", ImVec2(200, 30))) { g_currentAngle = 0.0f; g_logic.SetZero(); }
            
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "COLOR TOLERANCE");
            if (!g_allProfiles.empty()) {
                int tol = g_allProfiles[g_selectedProfileIdx].tolerance;
                if (ImGui::SliderInt("Tolerance", &tol, 0, 100)) {
                    g_allProfiles[g_selectedProfileIdx].tolerance = tol;
                    g_allProfiles[g_selectedProfileIdx].Save(GetAppStoragePath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
                }
            }
            ImGui::Spacing();
            ImGui::Text("Diag | Angle = %.2f° | Match = %.0f%%", g_currentAngle, g_detectionRatio * 100.0f);
            ImGui::EndTabItem();
        }

        // TAB: CROSSHAIR
        if (ImGui::BeginTabItem("CROSSHAIR")) {
            ImGui::Spacing();
            ImGui::TextDisabled("PRECISION CROSSHAIR CONFIG");
            ImGui::Spacing();

            // Large Toggle Button (Java-style visibility control)
            ImVec4 toggleCol = g_showCrosshair ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.7f, 0.2f, 0.2f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, toggleCol);
            if (ImGui::Button("TOGGLE MANUAL CROSSHAIR", ImVec2(-1, 40))) {
                g_showCrosshair = !g_showCrosshair;
                SaveSettings();
                // v4.20.35: Immediate Refresh
                if (g_hHUD) {
                    InvalidateRect(g_hHUD, NULL, FALSE);
                    UpdateWindow(g_hHUD);
                }
            }
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float cCol[3] = { GetRValue(g_crossColor)/255.f, GetGValue(g_crossColor)/255.f, GetBValue(g_crossColor)/255.f };
            if (ImGui::ColorEdit3("Crosshair Color", cCol)) {
                g_crossColor = RGB((BYTE)(cCol[0]*255), (BYTE)(cCol[1]*255), (BYTE)(cCol[2]*255)); SaveSettings();
            }
            if (ImGui::Checkbox("Pulse Animation", &g_crossPulse)) SaveSettings();
            
            ImGui::Spacing();

            // Precision Control Helper (v4.20.37: With Immediate HUD Refresh)
            auto PrecisionSlider = [](const char* label, float* val, float minV, float maxV, float step) {
                ImGui::Text("%s", label);
                if (ImGui::Button(("-##" + std::string(label)).c_str(), ImVec2(25, 25))) { 
                    *val -= step; SaveSettings(); 
                    if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
                if (ImGui::SliderFloat(("##" + std::string(label)).c_str(), val, minV, maxV, "%.2f")) {
                    SaveSettings();
                    if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
                }
                ImGui::SameLine();
                if (ImGui::Button(("+##" + std::string(label)).c_str(), ImVec2(25, 25))) { 
                    *val += step; SaveSettings(); 
                    if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
                }
            };

            PrecisionSlider("Thickness", &g_crossThickness, 1.0f, 10.0f, 0.5f);
            PrecisionSlider("Rotation", &g_crossAngle, -180.0f, 180.0f, 5.0f);
            PrecisionSlider("Offset X", &g_crossOffsetX, -500.0f, 500.0f, 0.5f);
            PrecisionSlider("Offset Y", &g_crossOffsetY, -500.0f, 500.0f, 0.5f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("DIRECTIONAL NUDGE (PIXEL-PERFECT)");
            float nudgeW = 70.0f;
            float centerOffset = (ImGui::GetContentRegionAvail().x - nudgeW) * 0.5f;

            ImGui::SetCursorPosX(centerOffset);
            if (ImGui::Button("UP", ImVec2(nudgeW, 35))) { g_crossOffsetY -= 1.0f; }

            ImGui::SetCursorPosX(centerOffset - nudgeW - 8);
            if (ImGui::Button("LEFT", ImVec2(nudgeW, 35))) { g_crossOffsetX -= 1.0f; }
            ImGui::SameLine();
            if (ImGui::Button("DOWN", ImVec2(nudgeW, 35))) { g_crossOffsetY += 1.0f; }
            ImGui::SameLine();
            if (ImGui::Button("RIGHT", ImVec2(nudgeW, 35))) { g_crossOffsetX += 1.0f; }
            
            // Auto-save helper
            auto SyncAndSaveCurrentProfile = [&]() {
                if (!g_allProfiles.empty()) {
                    Profile& p = g_allProfiles[g_selectedProfileIdx];
                    p.crossThickness = g_crossThickness; p.crossColor = g_crossColor;
                    p.crossOffsetX   = g_crossOffsetX;   p.crossOffsetY = g_crossOffsetY;
                    p.crossAngle     = g_crossAngle;     p.crossPulse   = g_crossPulse;
                    p.Save(GetAppStoragePath() + p.name + L".json");
                }
                SaveSettings();
            };

            // Hook sliders to auto-save
            static float lastX = 0, lastY = 0, lastT = 0, lastA = 0;
            if (g_crossOffsetX != lastX || g_crossOffsetY != lastY || g_crossThickness != lastT || g_crossAngle != lastA) {
                SyncAndSaveCurrentProfile();
                lastX = g_crossOffsetX; lastY = g_crossOffsetY; lastT = g_crossThickness; lastA = g_crossAngle;
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("RESET", ImVec2(-1, 35))) {
                g_crossThickness = 2.0f; g_crossOffsetX = 0.0f; g_crossOffsetY = 0.0f;
                g_crossAngle = 0.0f; g_crossPulse = false; g_crossColor = RGB(255,0,0);
                SyncAndSaveCurrentProfile();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("SAVED POSITIONS (PRESETS)");
            static char presetName[64] = "";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 110);
            ImGui::InputTextWithHint("##presetNm", "Preset Name...", presetName, 64);
            ImGui::SameLine();
            if (ImGui::Button("SAVE AS", ImVec2(100, 0))) {
                if (strlen(presetName) > 0) {
                    std::string nStr(presetName);
                    CrosshairPreset cp;
                    cp.name = std::wstring(nStr.begin(), nStr.end());
                    cp.offsetX = g_crossOffsetX;
                    cp.offsetY = g_crossOffsetY;
                    cp.angle   = g_crossAngle;
                    p.crosshairPresets.push_back(cp);
                    p.Save(GetAppStoragePath() + p.name + L".json");
                    presetName[0] = '\0'; // Clear input
                }
            }

            ImGui::Spacing();
            ImGui::BeginChild("PresetsList", ImVec2(0, 150), true);
            for (size_t i = 0; i < p.crosshairPresets.size(); i++) {
                auto& cp = p.crosshairPresets[i];
                std::string label;
                for (wchar_t c : cp.name) label += (char)c;
                
                std::string fullLabel = "[" + std::to_string(i+1) + "] " + label;
                
                if (ImGui::Button(fullLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 30))) {
                    g_crossOffsetX = cp.offsetX;
                    g_crossOffsetY = cp.offsetY;
                    g_crossAngle   = cp.angle;
                    SaveSettings();
                    if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.1f, 0.1f, 1.0f));
                if (ImGui::Button(("X##" + std::to_string(i)).c_str(), ImVec2(25, 30))) {
                    p.crosshairPresets.erase(p.crosshairPresets.begin() + i);
                    p.Save(GetAppStoragePath() + p.name + L".json");
                }
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // TAB: UPDATES
        if (ImGui::BeginTabItem("UPDATES")) {
            ImGui::Spacing();
            ImGui::Text("BetterAngle Pro | Precision Suite");
            ImGui::Spacing();
            ImGui::Text("Current Version: v%s", VERSION_STR);
            
            if (g_hasCheckedForUpdates) {
                ImGui::Text("Latest Version:  %s", g_latestVersionOnline.c_str());
            } else {
                ImGui::TextDisabled("Latest Version:  (Checking...)");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (g_updateAvailable) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
                ImGui::TextWrapped("A new update is available for BetterAngle Pro!");
                ImGui::PopStyleColor();
                
                std::string btnText = "DOWNLOAD & INSTALL " + g_latestVersionOnline + " NOW";
                if (g_isDownloadingUpdate) btnText = "DOWNLOADING... PLEASE WAIT";
                
                if (ImGui::Button(btnText.c_str(), ImVec2(-1, 40))) {
                    UpdateApp();
                }
            } else {
                if (g_hasCheckedForUpdates) {
                    ImGui::Text("Your software is fully up to date.");
                } else {
                    ImGui::TextDisabled("Click below to check for software updates.");
                }
                
                if (ImGui::Button("CHECK FOR UPDATES", ImVec2(-1, 35))) {
                    std::thread([]() { CheckForUpdates(); }).detach();
                }
            }
            ImGui::EndTabItem();
        }


        ImGui::EndTabBar();
    }
    
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0); // Present with vsync
    s_inFrame = false;
}

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN) {
        // Keyboard Nudge Support for Crosshair (Tab Index 2)
        if (g_currentTab == 2) {
            float step = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 5.0f : 1.0f;
            bool changed = false;
            if (wParam == VK_LEFT)  { g_crossOffsetX -= step; changed = true; }
            if (wParam == VK_RIGHT) { g_crossOffsetX += step; changed = true; }
            if (wParam == VK_UP)    { g_crossOffsetY -= step; changed = true; }
            if (wParam == VK_DOWN)  { g_crossOffsetY += step; changed = true; }
            
            if (changed) {
                SaveSettings();
                if (g_hHUD) { InvalidateRect(g_hHUD, NULL, FALSE); UpdateWindow(g_hHUD); }
                return 0;
            }
        }

        if (g_listeningKey != -1) {
            if (wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU || wParam == VK_ESCAPE) {
                if (wParam == VK_ESCAPE) g_listeningKey = -1;
                return 0;
            }
            UINT mod = 0;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
            if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mod |= MOD_SHIFT;
            if (GetAsyncKeyState(VK_MENU)    & 0x8000) mod |= MOD_ALT;
            
            if (g_allProfiles.empty()) return 0;
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            
            switch (g_listeningKey) {
                case 1: p.keybinds.toggleMod = mod; p.keybinds.toggleKey = (UINT)wParam; break;
                case 2: p.keybinds.roiMod    = mod; p.keybinds.roiKey    = (UINT)wParam; break;
                case 3: p.keybinds.crossMod  = mod; p.keybinds.crossKey  = (UINT)wParam; break;
                case 4: p.keybinds.zeroMod   = mod; p.keybinds.zeroKey   = (UINT)wParam; break;
                case 5: p.keybinds.debugMod  = mod; p.keybinds.debugKey  = (UINT)wParam; break;
            }
            g_listeningKey = -1;
            p.Save(GetAppStoragePath() + p.name + L".json");

            if (g_hHUD) {
                UnregisterHotKey(g_hHUD, 1); RegisterHotKey(g_hHUD, 1, p.keybinds.toggleMod, p.keybinds.toggleKey);
                UnregisterHotKey(g_hHUD, 2); RegisterHotKey(g_hHUD, 2, p.keybinds.roiMod,    p.keybinds.roiKey);
                UnregisterHotKey(g_hHUD, 3); RegisterHotKey(g_hHUD, 3, p.keybinds.crossMod,  p.keybinds.crossKey);
                UnregisterHotKey(g_hHUD, 4); RegisterHotKey(g_hHUD, 4, p.keybinds.zeroMod,   p.keybinds.zeroKey);
                UnregisterHotKey(g_hHUD, 5); RegisterHotKey(g_hHUD, 5, p.keybinds.debugMod,  p.keybinds.debugKey);
            }
            return 0;
        }
    }

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
            ImGui::StyleColorsDark();
            
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = NULL;
            io.FontGlobalScale = 1.30f; // Scale up the default font for better readability
            
            ImGuiStyle& style = ImGui::GetStyle();
            
            // 2D Flat Style (No rounding, no borders, no 3D effects)
            style.WindowRounding    = 0.0f;
            style.ChildRounding     = 0.0f;
            style.FrameRounding     = 0.0f;
            style.PopupRounding     = 0.0f;
            style.ScrollbarRounding = 0.0f;
            style.GrabRounding      = 0.0f;
            style.TabRounding       = 0.0f;
            
            style.WindowBorderSize  = 0.0f;
            style.FrameBorderSize   = 0.0f;
            style.PopupBorderSize   = 0.0f;
            style.ChildBorderSize   = 0.0f;
            style.TabBorderSize     = 0.0f;

            // Padding & Sizing
            style.WindowPadding     = ImVec2(20.0f, 20.0f);
            style.FramePadding      = ImVec2(12.0f, 8.0f);
            style.ItemSpacing       = ImVec2(12.0f, 12.0f);
            style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
            style.ScrollbarSize     = 14.0f;
            
            // Premium Stealth Dark Theme (No blues, zero borders)
            style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
            style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
            style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
            style.Colors[ImGuiCol_Border]                = ImVec4(0.15f, 0.15f, 0.15f, 0.30f);
            style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
            style.Colors[ImGuiCol_Button]                = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.25f, 0.26f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_Header]                = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.25f, 0.26f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_Tab]                   = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
            style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_TabActive]             = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
            style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_Separator]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 450,
        nullptr, nullptr, hInst, nullptr
    );
    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}