#include "shared/FirstTimeSetup.h"
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <string>
#include <fstream>
#include <shlobj.h>
#include "shared/Profile.h"
#include "shared/State.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

int g_setupState = 1;

std::wstring g_setupDPI = L"";
std::wstring g_setupSensX = L"";
std::wstring g_setupSensY = L"";

bool g_extractedConfig = false;

void FinishSetup() {
    Profile p;
    p.name = L"Fallback_Default";
    p.roi_h = 60; p.roi_w = 400; p.roi_x = 760; p.roi_y = 650;
    p.target_color = RGB(150, 150, 150);
    p.tolerance = 25;

    double dpiVal = 800.0;
    try { dpiVal = std::stod(g_setupDPI); } catch(...) {}
    if (dpiVal <= 0) dpiVal = 800.0;

    double sensXVal = 0.05;
    try { sensXVal = std::stod(g_setupSensX); } catch(...) {}
    if (sensXVal <= 0) sensXVal = 0.05;

    double sensYVal = 0.05;
    try { sensYVal = std::stod(g_setupSensY); } catch(...) {}
    if (sensYVal <= 0) sensYVal = 0.05;

    p.dpi = (int)dpiVal;
    p.sensitivityX = sensXVal;
    p.sensitivityY = sensYVal;
    p.divingScaleMultiplier = 1.22;

    p.fov = 80.0f;
    p.resolutionWidth = GetSystemMetrics(SM_CXSCREEN);
    p.resolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    p.renderScale = 100.0f;

    extern std::vector<Profile> g_allProfiles;
    extern int g_selectedProfileIdx;
    
    if (g_allProfiles.empty()) {
        p.name = L"Fallback_Default";
        p.Save(GetAppStoragePath() + L"Fallback_Default.json");
        g_allProfiles.push_back(p);
    } else {
        p.name = g_allProfiles[g_selectedProfileIdx].name;
        // Keep existing ROI parameters
        p.roi_h = g_allProfiles[g_selectedProfileIdx].roi_h;
        p.roi_w = g_allProfiles[g_selectedProfileIdx].roi_w;
        p.roi_x = g_allProfiles[g_selectedProfileIdx].roi_x;
        p.roi_y = g_allProfiles[g_selectedProfileIdx].roi_y;
        p.target_color = g_allProfiles[g_selectedProfileIdx].target_color;
        p.tolerance = g_allProfiles[g_selectedProfileIdx].tolerance;
        g_allProfiles[g_selectedProfileIdx] = p;
        p.Save(GetAppStoragePath() + p.name + L".json");
    }
}

void DrawSetupButton(Graphics& g, Font* f, const wchar_t* text, int x, int y, int w, int h, Color fill) {
    SolidBrush brush(fill);
    g.FillRectangle(&brush, x, y, w, h);
    SolidBrush white(Color(255,255,255));
    g.DrawString(text, -1, f, PointF(x + 10, y + 10), &white);
}

// 0 for neither, 1 for SensX, 2 for SensY
int g_focusedInput = 1;

LRESULT CALLBACK FirstTimeSetupProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CREATE) {
        return 0;
    }
    if (message == WM_MOUSEACTIVATE) {
        return MA_ACTIVATE;
    }
    if (message == WM_CHAR) {
        if (wParam == VK_RETURN) { // Enter confirms the step
            if (g_setupState == 1) {
                g_setupState = 2; // Fixed state jump
            } else if (g_setupState == 2) {
                FinishSetup();
                DestroyWindow(hWnd);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        if (wParam == VK_BACK) {
            std::wstring* active = nullptr;
            if (g_setupState == 1) active = &g_setupDPI;
            else if (g_setupState == 2) {
                if (g_focusedInput == 1) active = &g_setupSensX;
                else if (g_focusedInput == 2) active = &g_setupSensY;
            }
            if (active && !active->empty()) active->pop_back();
        } else if (wParam >= 32 && wParam <= 126) {
            std::wstring* active = nullptr;
            if (g_setupState == 1) active = &g_setupDPI;
            else if (g_setupState == 2) {
                if (g_focusedInput == 1) active = &g_setupSensX;
                else if (g_focusedInput == 2) active = &g_setupSensY;
            }
            if (active) active->push_back((wchar_t)wParam);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    if (message == WM_LBUTTONDOWN) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        
        if (g_setupState == 1) {
            if (y > 150 && y < 190 && x > 20 && x < 200) {
                g_setupState = 2;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else if (g_setupState == 2) {
            if (y > 220 && y < 260 && x > 20 && x < 200) {
                FinishSetup();
                DestroyWindow(hWnd);
            } else if (y > 140 && y < 170) {
                if (x > 20 && x < 150) { g_focusedInput = 1; InvalidateRect(hWnd, NULL, FALSE); }
                else if (x > 180 && x < 310) { g_focusedInput = 2; InvalidateRect(hWnd, NULL, FALSE); }
            }
        }
        SetFocus(hWnd); // Ensure window gets focus on any click
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Graphics g(hdc);
        g.Clear(Color(255, 15, 17, 22)); // Match Startup Charcoal

        // Neon Border
        Pen neonPen(Color(255, 0, 255, 255), 1);
        RECT rc; GetClientRect(hWnd, &rc);
        g.DrawRectangle(&neonPen, 0, 0, rc.right - 1, rc.bottom - 1);

        FontFamily ff(L"Segoe UI");
        Font title(&ff, 20, FontStyleBold, UnitPixel);
        Font text(&ff, 15, FontStyleRegular, UnitPixel);
        Font smallText(&ff, 12, FontStyleRegular, UnitPixel);
        SolidBrush white(Color(255, 255, 255));
        SolidBrush gray(Color(180, 180, 180));
        SolidBrush inputBg(Color(40, 45, 50));
        SolidBrush inputFocusedBg(Color(60, 65, 70));

        g.DrawString(L"First Time Integration Setup", -1, &title, PointF(20, 20), &white);

        if (g_setupState == 1) {
            g.DrawString(L"Step 1: Mouse DPI", -1, &text, PointF(20, 60), &gray);
            g.DrawString(L"Enter your mouse DPI", -1, &text, PointF(20, 85), &white);
            g.FillRectangle(&inputBg, 20, 110, 300, 30);
            
            std::wstring disp = g_setupDPI;
            if (disp.empty()) {
                g.DrawString(L"e.g. 800", -1, &text, PointF(25, 115), &gray);
            } else {
                disp += L"_";
                g.DrawString(disp.c_str(), -1, &text, PointF(25, 115), &white);
            }
            
            DrawSetupButton(g, &text, L"Next", 20, 150, 180, 40, Color(0, 120, 215));
        } else if (g_setupState == 2) {
            g.DrawString(L"Step 2: Fortnite Sensitivity", -1, &text, PointF(20, 60), &gray);
            
            if (g_extractedConfig) {
                g.DrawString(L"We found your Fortnite sensitivity from your game files.\nIf this looks correct just press Enter to confirm.", -1, &smallText, PointF(20, 90), &white);
            } else {
                g.DrawString(L"We couldn't detect your config. Please enter manually.", -1, &smallText, PointF(20, 90), &white);
            }
            
            g.DrawString(L"Sensitivity X", -1, &text, PointF(20, 120), &gray);
            g.FillRectangle(g_focusedInput == 1 ? &inputFocusedBg : &inputBg, 20, 140, 130, 30);
            std::wstring dispX = g_setupSensX;
            if (dispX.empty()) {
                g.DrawString(L"Enter manually", -1, &text, PointF(25, 145), &gray);
            } else {
                if (g_focusedInput == 1) dispX += L"_";
                g.DrawString(dispX.c_str(), -1, &text, PointF(25, 145), &white);
            }

            g.DrawString(L"Sensitivity Y", -1, &text, PointF(180, 120), &gray);
            g.FillRectangle(g_focusedInput == 2 ? &inputFocusedBg : &inputBg, 180, 140, 130, 30);
            std::wstring dispY = g_setupSensY;
            if (dispY.empty()) {
                g.DrawString(L"Enter manually", -1, &text, PointF(185, 145), &gray);
            } else {
                if (g_focusedInput == 2) dispY += L"_";
                g.DrawString(dispY.c_str(), -1, &text, PointF(185, 145), &white);
            }
            
            DrawSetupButton(g, &text, L"Confirm", 20, 220, 180, 40, Color(0, 120, 215));
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void StartModalSetupLoop(HWND hwnd) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!IsWindow(hwnd)) break;
    }
}

void ShowFirstTimeSetup(HINSTANCE hInstance) {
    g_setupState = 1;
    g_setupDPI = L""; g_setupSensX = L""; g_setupSensY = L"";
    g_focusedInput = 1;
    g_extractedConfig = false;

    // Parse Phase
    std::string iniContent = "";
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::wstring p = std::wstring(path) + L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";
        std::ifstream ifs(p.c_str());
        if (ifs.good()) {
            iniContent.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        }
    }
    auto extractValue = [&](const std::string& key) -> std::wstring {
        size_t pos = iniContent.find(key + "=");
        if (pos != std::string::npos) {
            size_t end = iniContent.find_first_of("\r\n", pos);
            std::string val = iniContent.substr(pos + key.length() + 1, end - (pos + key.length() + 1));
            return std::wstring(val.begin(), val.end());
        }
        return L"";
    };

    g_setupSensX = extractValue("MouseX");
    g_setupSensY = extractValue("MouseY");
    
    if (!g_setupSensX.empty() || !g_setupSensY.empty()) {
        g_extractedConfig = true;
    }

    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = FirstTimeSetupProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FTSWindowClass";
    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST, L"FTSWindowClass", L"BetterAngle Setup", WS_POPUP,
        GetSystemMetrics(SM_CXSCREEN) / 2 - 250, GetSystemMetrics(SM_CYSCREEN) / 2 - 175,
        500, 350, NULL, NULL, hInstance, NULL
    );

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

    StartModalSetupLoop(hWnd);
}
