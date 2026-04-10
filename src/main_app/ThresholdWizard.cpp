#include <windows.h>
#include <string>
#include "shared/State.h"
#include "shared/Profile.h"
#include "shared/Input.h"
#include <gdiplus.h>
#include <cmath>
#include <algorithm>
#include <vector>

extern float g_detectionRatio;
extern std::vector<Profile> g_allProfiles;
bool IsFortniteFocused();

int g_wizStep = 0;
HWND g_hWiz = NULL;
long long g_wizDxNormal = 0;
long long g_wizDxFreefall = 0;
long long g_wizDxGliding = 0;

LRESULT CALLBACK ThresholdWizProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CREATE) {
        SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        return 0;
    }
    if (message == WM_INPUT) {
        if (!g_fortniteFocusedCache) return 0; // Prevent alt-tab and pre-focus tracking corruption
        int dx = GetRawInputDeltaX(lParam);
        if (g_wizStep == 0) g_wizDxNormal += dx;
        else if (g_wizStep == 1) g_wizDxFreefall += dx;
        else if (g_wizStep == 2) g_wizDxGliding += dx;
        return 0;
    }
    if (message == WM_HOTKEY) {
        if (wParam == 99) { // F10 hotkey
            if (g_wizStep == 0) {
                g_wizStep = 1;
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (g_wizStep == 1) {
                g_freefallThreshold = g_detectionRatio;
                g_wizStep = 2;
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (g_wizStep == 2) {
                g_glideThreshold = g_detectionRatio;
                
                // Complete Wizard! Calculate sensitivities.
                double diff_n = std::abs((double)g_wizDxNormal);
                double diff_f = std::abs((double)g_wizDxFreefall);
                double diff_g = std::abs((double)g_wizDxGliding);
                if (diff_n == 0) diff_n = 1;
                if (diff_f == 0) diff_f = 1;
                if (diff_g == 0) diff_g = 1;

                Profile p;
                std::wstring path = GetAppStoragePath() + L"last_calibrated.json";
                p.Load(path); 
                
                double normalScaleTested = 120.0 / (std::max)(1.0, diff_n);
                p.sensitivityX = normalScaleTested / 0.05555;
                
                p.name = L"last_calibrated";
                
                p.Save(path); 
                
                // Also update the global allProfiles loaded into UI currently
                for (size_t i = 0; i < g_allProfiles.size(); i++) {
                    if (g_allProfiles[i].name == L"last_calibrated") {
                        g_allProfiles[i] = p;
                        break;
                    }
                }

                void SaveSettings();
                SaveSettings(); 
                
                DestroyWindow(hWnd);
            }
        }
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Gdiplus::Graphics graphics(hdc);
        graphics.Clear(Gdiplus::Color(240, 20, 25, 30));

        Gdiplus::FontFamily ff(L"Segoe UI");
        Gdiplus::Font f(&ff, 16, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::Font f2(&ff, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush white(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::SolidBrush amber(Gdiplus::Color(255, 240, 160, 40));

        graphics.DrawString(L"MASTER CALIBRATION HUD", -1, &f, Gdiplus::PointF(10, 10), &white);

        if (g_wizStep == 0) {
            graphics.DrawString(L"1. Look exactly center. Turn 120 degrees.", -1, &f2, Gdiplus::PointF(10, 40), &amber);
            graphics.DrawString(L"2. Press F10.", -1, &f2, Gdiplus::PointF(10, 60), &white);
        } else if (g_wizStep == 1) {
            graphics.DrawString(L"1. Jump out of the bus and FREEFALL.", -1, &f2, Gdiplus::PointF(10, 40), &amber);
            graphics.DrawString(L"2. Turn exactly 120 degrees. Press F10.", -1, &f2, Gdiplus::PointF(10, 60), &white);
        } else if (g_wizStep == 2) {
            graphics.DrawString(L"1. Deploy your GLIDER in the sky.", -1, &f2, Gdiplus::PointF(10, 40), &amber);
            graphics.DrawString(L"2. Turn exactly 120 degrees. Press F10.", -1, &f2, Gdiplus::PointF(10, 60), &white);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    if (message == WM_DESTROY) {
        UnregisterHotKey(hWnd, 99);
        g_hWiz = NULL;
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void StartThresholdWizard(HINSTANCE hInstance) {
    static bool s_wizardActive = false;
    if (s_wizardActive || g_hWiz) return;
    s_wizardActive = true;

    struct GuardReset {
        bool* active;
        ~GuardReset() { *active = false; }
    } resetter{&s_wizardActive};
    g_wizStep = 0;
    g_wizDxNormal = 0;
    g_wizDxFreefall = 0;
    g_wizDxGliding = 0;
    
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ThresholdWizProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"ThresholdWizardWnd";
    RegisterClass(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    
    g_hWiz = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"ThresholdWizardWnd", L"Threshold Wizard",
        WS_POPUP,
        sw / 2 - 175, 50, 350, 100,
        NULL, NULL, hInstance, NULL
    );

    RegisterRawMouse(g_hWiz);
    RegisterHotKey(g_hWiz, 99, 0, VK_F10);
    ShowWindow(g_hWiz, SW_SHOW);
    UpdateWindow(g_hWiz);
}
