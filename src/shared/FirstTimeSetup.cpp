#include "shared/FirstTimeSetup.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <fstream>
#include <shlobj.h>
#include <vector>
#include <algorithm>
#include <cwctype>
#include "shared/Profile.h"
#include "shared/State.h"
#include <gdiplus.h>

using namespace Gdiplus;

// Use persistent state from shared/State.h
static std::wstring g_setupSensX = L"0.05";
static std::wstring g_setupSensY = L"0.05";
static bool g_isEditingManual    = true; 
static int  g_focusedInput       = 1;

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;

void FinishSetup() {
    Profile p;
    p.name = L"Default"; 
    p.tolerance = 25;
    p.roi_x = 760; p.roi_y = 640; p.roi_w = 400; p.roi_h = 70;
    p.target_color = RGB(150, 150, 150);
    p.fov = 80.0f;
    p.resolutionWidth  = GetSystemMetrics(SM_CXSCREEN);
    p.resolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    p.renderScale = 100.0f;

    double sensX = 0.05, sensY = 0.05;
    try { if (!g_setupSensX.empty()) sensX  = std::stod(g_setupSensX); } catch(...) {}
    try { if (!g_setupSensY.empty()) sensY  = std::stod(g_setupSensY); } catch(...) {}
    
    p.sensitivityX = (std::max)(0.0001, sensX);
    p.sensitivityY = (std::max)(0.0001, sensY);

    if (g_allProfiles.empty()) {
        p.Save(GetProfilesPath() + L"Default.json");
        g_allProfiles.push_back(p);
        g_selectedProfileIdx = 0;
    } else {
        if (g_selectedProfileIdx < 0 || g_selectedProfileIdx >= (int)g_allProfiles.size()) {
            g_selectedProfileIdx = 0;
        }
        Profile& e = g_allProfiles[g_selectedProfileIdx];
        e.sensitivityX = p.sensitivityX;
        e.sensitivityY = p.sensitivityY;
        e.Save(GetProfilesPath() + e.name + L".json");
    }

    g_setupComplete = true; 
    SaveSettings(); // Force atomic save now

    // Create a hidden marker file for absolute persistence (even if JSON is lost)
    std::wstring marker = GetAppRootPath() + L".setup_done";
    HANDLE h = CreateFileW(marker.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
}

static void PaintSetup(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, W, H);
    HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Modern Background
    LinearGradientBrush bgBrush(Point(0,0), Point(0, H), Color(255, 10, 12, 20), Color(255, 5, 5, 10));
    graphics.FillRectangle(&bgBrush, 0, 0, W, H);

    FontFamily ff(L"Segoe UI");
    Font fTit(&ff, 24, FontStyleBold, UnitPixel);
    Font fSub(&ff, 11, FontStyleRegular, UnitPixel);
    Font fInp(&ff, 18, FontStyleBold, UnitPixel);
    Font fBod(&ff, 13, FontStyleRegular, UnitPixel);

    SolidBrush bWhite(Color(255, 255, 255, 255));
    SolidBrush bAccent(Color(255, 0, 204, 204)); // Cyan
    SolidBrush bDim(Color(255, 100, 110, 130));

    StringFormat fmtC; fmtC.SetAlignment(StringAlignmentCenter); fmtC.SetLineAlignment(StringAlignmentCenter);
    StringFormat fmtL; fmtL.SetAlignment(StringAlignmentNear);   fmtL.SetLineAlignment(StringAlignmentCenter);

    graphics.DrawString(L"CALIBRATION WIZARD", -1, &fSub, RectF(0, 30, (float)W, 18), &fmtC, &bAccent);
    graphics.DrawString(L"Set In-Game Sensitivity", -1, &fTit, RectF(0, 60, (float)W, 34), &fmtC, &bWhite);
    graphics.DrawString(L"Enter your Fortnite sensitivity below to ensure accuracy.", -1, &fBod, RectF(0, 100, (float)W, 20), &fmtC, &bDim);

    // Inputs
    int fw = 180, fxA = (W/2) - fw - 10, fxB = (W/2) + 10, fy = 150, fh = 40;

    auto field = [&](float fx, int foc, const wchar_t* lab, const std::wstring& val) {
        graphics.DrawString(lab, -1, &fSub, RectF(fx, (float)fy-20, (float)fw, 18), &fmtL, &bDim);
        SolidBrush inBg(Color(255, 20, 25, 35));
        graphics.FillRectangle(&inBg, fx, (float)fy, (float)fw, (float)fh);
        Pen border(g_focusedInput == foc ? Color(255, 0, 204, 204) : Color(255, 50, 60, 80), 1.0f);
        graphics.DrawRectangle(&border, fx, (float)fy, (float)fw, (float)fh);
        
        std::wstring d = val + (g_focusedInput==foc ? L"|" : L"");
        graphics.DrawString(d.c_str(), -1, &fInp, RectF(fx+10, (float)fy, (float)fw-20, (float)fh), &fmtL, &bWhite);
    };
    field((float)fxA, 1, L"SENSITIVITY X", g_setupSensX);
    field((float)fxB, 2, L"SENSITIVITY Y", g_setupSensY);

    // Save Button
    int bx = (W-180)/2, by = H-70, bw = 180, bh = 40;
    SolidBrush btnBg(Color(255, 0, 163, 163));
    graphics.FillRectangle(&btnBg, bx, by, bw, bh);
    graphics.DrawString(L"FINISH SETUP", -1, &fBod, RectF((float)bx, (float)by, (float)bw, (float)bh), &fmtC, &bWhite);

    BitBlt(hdc, 0, 0, W, H, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld); DeleteObject(hbmMem); DeleteDC(hdcMem);
    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK FirstTimeSetupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y < 50) return HTCAPTION;
        }
        return hit;
    }
    case WM_KEYDOWN:
        if (wParam == VK_TAB) {
            g_focusedInput = (g_focusedInput == 1) ? 2 : 1;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        if (wParam == VK_RETURN) {
            FinishSetup(); 
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_CHAR: {
        std::wstring* cur = (g_focusedInput == 1) ? &g_setupSensX : &g_setupSensY;
        if (wParam == VK_BACK) {
            if (!cur->empty()) cur->pop_back();
        } else if (iswdigit((wchar_t)wParam) || (wchar_t)wParam == L'.') {
             if ((wchar_t)wParam == L'.' && cur->find(L'.') != std::wstring::npos) return 0;
             if (cur->length() < 10) cur->push_back((wchar_t)wParam);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right, H = rc.bottom;
        int fw = 180, fxA = (W/2) - fw - 10, fxB = (W/2) + 10, fy = 150, fh = 40;
        if (my >= fy && my <= fy+fh) {
            if      (mx>=fxA && mx<=fxA+fw) g_focusedInput=1;
            else if (mx>=fxB && mx<=fxB+fw) g_focusedInput=2;
        }
        int bx=(W-180)/2, by=H-70;
        if (mx>=bx && mx<=bx+180 && my>=by && my<=by+40) {
            FinishSetup(); DestroyWindow(hWnd);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_PAINT: PaintSetup(hWnd); return 0;
    case WM_CLOSE: DestroyWindow(hWnd); return 0;
    case WM_DESTROY: return 0; // Don't call PostQuitMessage(0) here as it kills the main app thread
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ShowFirstTimeSetup(HINSTANCE hInstance) {
    WNDCLASS wc = { 0 };
    if (!GetClassInfo(hInstance, L"FTSWindowClass", &wc)) {
        wc.lpfnWndProc = FirstTimeSetupProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"FTSWindowClass";
        RegisterClass(&wc);
    }

    int W = 500, H = 320;
    HWND hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"FTSWindowClass", L"BetterAngle Setup", WS_POPUP,
        GetSystemMetrics(SM_CXSCREEN)/2 - W/2, GetSystemMetrics(SM_CYSCREEN)/2 - H/2, W, H, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (IsWindow(hWnd) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
