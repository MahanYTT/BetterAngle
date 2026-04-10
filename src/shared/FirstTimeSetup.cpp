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

static int g_setupState    = 2;
static std::wstring g_setupSensX = L"";
static std::wstring g_setupSensY = L"";
static bool g_extractedConfig    = false;
static bool g_isEditingManual    = false; 
static int  g_focusedInput       = 1;

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;

// ── Profile save ─────────────────────────────────────────────────────────
void FinishSetup() {
    Profile p;
    // Set sensible defaults first
    p.name = L"Default"; 
    p.tolerance = 25;
    p.roi_x = 760; p.roi_y = 640; p.roi_w = 400; p.roi_h = 70;
    p.target_color = RGB(150, 150, 150);
    p.fov = 80.0f;
    p.resolutionWidth  = GetSystemMetrics(SM_CXSCREEN);
    p.resolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    p.renderScale = 100.0f;

    // Parse numeric values safely
    double sensX = 0.05, sensY = 0.05;
    try { if (!g_setupSensX.empty()) sensX  = std::stod(g_setupSensX); } catch(...) {}
    try { if (!g_setupSensY.empty()) sensY  = std::stod(g_setupSensY); } catch(...) {}
    
    p.sensitivityX = (std::max)(0.0001, sensX);
    p.sensitivityY = (std::max)(0.0001, sensY);

    if (g_allProfiles.empty()) {
        p.Save(GetAppStoragePath() + L"Default.json");
        g_allProfiles.push_back(p);
        g_selectedProfileIdx = 0;
    } else {
        // Update existing selected profile safely
        if (g_selectedProfileIdx < 0 || g_selectedProfileIdx >= (int)g_allProfiles.size()) {
            g_selectedProfileIdx = 0;
        }
        Profile& e = g_allProfiles[g_selectedProfileIdx];
        e.sensitivityX = p.sensitivityX;
        e.sensitivityY = p.sensitivityY;
        e.Save(GetAppStoragePath() + e.name + L".json");
    }
    g_setupComplete = true; 
    SaveSettings();
}

// ── Paint ────────────────────────────────────────────────────────────────
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

    // Background Gradient (Matched to ImGui Dashboard)
    LinearGradientBrush bgBrush(Point(0,0), Point(0, H), Color(255, 12, 14, 18), Color(255, 5, 5, 5));
    graphics.FillRectangle(&bgBrush, 0, 0, W, H);

    FontFamily ff(L"Segoe UI");
    Font fTit(&ff, 28, FontStyleBold, UnitPixel);
    Font fSub(&ff, 11, FontStyleRegular, UnitPixel);
    Font fInp(&ff, 20, FontStyleBold, UnitPixel);
    Font fBod(&ff, 13, FontStyleRegular, UnitPixel);

    SolidBrush bWhite(Color(255, 255, 255, 255));
    SolidBrush bAccent(Color(255, 59, 130, 246));
    SolidBrush bDim(Color(255, 80, 85, 105));
    SolidBrush bCyan(Color(255, 34, 211, 238));

    StringFormat fmtC; fmtC.SetAlignment(StringAlignmentCenter); fmtC.SetLineAlignment(StringAlignmentCenter);
    StringFormat fmtL; fmtL.SetAlignment(StringAlignmentNear);   fmtL.SetLineAlignment(StringAlignmentCenter);
    StringFormat fmtR; fmtR.SetAlignment(StringAlignmentFar);    fmtR.SetLineAlignment(StringAlignmentCenter);

    // Header Glow Bar
    LinearGradientBrush glw(Point(0,0), Point(W,0), Color(0,0,0,0), Color(80, 59, 130, 246));
    glw.SetWrapMode(WrapModeTileFlipX);
    graphics.FillRectangle(&glw, 0, 0, W, 48);

    // Progress
    int pilX = (W - 148) / 2;
    SolidBrush bp1(g_setupState >= 1 ? Color(255, 59, 130, 246) : Color(255, 45, 50, 65));
    SolidBrush bp2(g_setupState >= 2 ? Color(255, 59, 130, 246) : Color(255, 45, 50, 65));
    graphics.FillRectangle(&bp1, pilX, 22, 70, 3);
    graphics.FillRectangle(&bp2, pilX + 78, 22, 70, 3);

    if (g_extractedConfig && !g_isEditingManual) {
        // --- STAGE 1: CONFIRMATION ---
        graphics.DrawString(L"CONFIRM SENSITIVITY", -1, &fSub, RectF(0, 36, (float)W, 18), &fmtC, &bAccent);
        graphics.DrawString(L"Is this correct?",      -1, &fTit, RectF(0, 64, (float)W, 34), &fmtC, &bWhite);
        
        std::wstring msg = L"We found your Fortnite settings: " + g_setupSensX + L" x " + g_setupSensY;
        graphics.DrawString(msg.c_str(), -1, &fBod, RectF(0, 108, (float)W, 20), &fmtC, &bCyan);

        // Big Cyan Checkmark
        Font fCheck(&ff, 60, FontStyleBold, UnitPixel);
        graphics.DrawString(L"\x2713", -1, &fCheck, RectF(0, 130, (float)W, 80), &fmtC, &bCyan);

        // Buttons
        int bx = (W-210)/2, by = H-84, bw = 210, bh = 42;
        LinearGradientBrush bB(Point(bx, by), Point(bx, by+bh), Color(255, 60, 140, 250), Color(255, 30, 90, 200));
        graphics.FillRectangle(&bB, bx, by, bw, bh);
        graphics.DrawString(L"YES, LOCK IT IN", -1, &fBod, RectF((float)bx, (float)by, (float)bw, (float)bh), &fmtC, &bWhite);

        graphics.DrawString(L"NO, LET ME EDIT", -1, &fSub, RectF(0, (float)H-38, (float)W, 20), &fmtC, &bDim);
    } else {
        // --- STAGE 2: MANUAL EDITING ---
        graphics.DrawString(L"PRO CALIBRATION WIZARD", -1, &fSub, RectF(0, 36, (float)W, 18), &fmtC, &bAccent);
        graphics.DrawString(L"In-Game Sensitivity",   -1, &fTit, RectF(0, 64, (float)W, 34), &fmtC, &bWhite);

        const wchar_t* sub = g_extractedConfig ? L"Edit your auto-detected sensitivity below." : L"Manually set your mouse sensitivity below.";
        graphics.DrawString(sub, -1, &fBod, RectF(0, 108, (float)W, 20), &fmtC, g_extractedConfig ? &bCyan : &bDim);

        // Input fields
        int fw = (W - 90) / 2, fxA = 40, fxB = fxA + fw + 10, fy = 166, fh = 44;

        auto field = [&](float fx, int foc, const wchar_t* lab, const std::wstring& val) {
            graphics.DrawString(lab, -1, &fSub, RectF(fx, (float)fy-20, (float)fw, 18), &fmtL, &bDim);
            SolidBrush inBg(Color(255, 20, 22, 28));
            graphics.FillRectangle(&inBg, fx, (float)fy, (float)fw, (float)fh);
            Pen border(g_focusedInput == foc ? Color(255, 59, 130, 246) : Color(255, 45, 50, 60), 1.5f);
            graphics.DrawRectangle(&border, fx, (float)fy, (float)fw, (float)fh);
            
            std::wstring d = val.empty() ? L"0.00" : val + (g_focusedInput==foc ? L"_" : L"");
            SolidBrush tx(val.empty() ? Color(255, 50, 55, 75) : Color(255, 255, 255, 255));
            graphics.DrawString(d.c_str(), -1, &fInp, RectF(fx+12, (float)fy, (float)fw-24, (float)fh), &fmtL, &tx);
        };
        field((float)fxA, 1, L"SENSITIVITY X", g_setupSensX);
        field((float)fxB, 2, L"SENSITIVITY Y", g_setupSensY);

        // Button
        int bx = (W-200)/2, by = H-74, bw = 200, bh = 42;
        LinearGradientBrush bB(Point(bx, by), Point(bx, by+bh), Color(255, 30, 90, 200), Color(255, 60, 140, 250));
        graphics.FillRectangle(&bB, bx, by, bw, bh);
        graphics.DrawString(L"SAVE CONFIG  \x2713", -1, &fBod, RectF((float)bx, (float)by, (float)bw, (float)bh), &fmtC, &bWhite);
    }

    graphics.DrawString(L"PRO SUITE ACTIVE", -1, &fSub, RectF(20, (float)H-28, 150, 20), &fmtL, &bAccent);
    graphics.DrawString(L"DRAG HEADER TO MOVE", -1, &fSub, RectF((float)W-170, (float)H-28, 150, 20), &fmtR, &bDim);

    BitBlt(hdc, 0, 0, W, H, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld); DeleteObject(hbmMem); DeleteDC(hdcMem);
    EndPaint(hWnd, &ps);
}

// ── Window Procedure ──────────────────────────────────────────────────────
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
    case WM_ERASEBKGND: return 1;

    case WM_KEYDOWN:
        if (wParam == VK_TAB && g_setupState == 2) {
            g_focusedInput = (g_focusedInput == 1) ? 2 : 1;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (g_setupState == 2) {
                FinishSetup(); DestroyWindow(hWnd);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        return 0;

    case WM_CHAR: {
        std::wstring* cur = (g_focusedInput == 1) ? &g_setupSensX : &g_setupSensY;
        if (wParam == VK_BACK) {
            if (!cur->empty()) cur->pop_back();
        } else if (iswdigit((wchar_t)wParam) || (wchar_t)wParam == L'.') {
            // Basic validation: only allow one dot
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

        if (g_extractedConfig && !g_isEditingManual) {
            // Confirm stage buttons
            int bx = (W-210)/2, by = H-84, bw = 210, bh = 42;
            if (mx>=bx && mx<=bx+bw && my>=by && my<=by+bh) {
                FinishSetup(); DestroyWindow(hWnd);
            }
            // "NO, LET ME EDIT" link area
            if (my >= H-40 && my <= H-10) {
                g_isEditingManual = true;
            }
        } else {
            // Manual stage buttons
            int fw = (W-90)/2, fxA = 40, fxB = fxA+fw+10, fy = 166, fh = 44;
            if (my >= fy && my <= fy+fh) {
                if      (mx>=fxA && mx<=fxA+fw) g_focusedInput=1;
                else if (mx>=fxB && mx<=fxB+fw) g_focusedInput=2;
            }
            int bx=(W-200)/2, by=H-74;
            if (mx>=bx && mx<=bx+200 && my>=by && my<=by+42) {
                FinishSetup(); DestroyWindow(hWnd);
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: PaintSetup(hWnd); return 0;
    case WM_CLOSE: DestroyWindow(hWnd); return 0;
    case WM_DESTROY: return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ShowFirstTimeSetup(HINSTANCE hInstance) {
    static HWND s_hFTS = NULL;
    if (s_hFTS && IsWindow(s_hFTS)) {
        SetForegroundWindow(s_hFTS);
        return;
    }

    // Reset state locally
    g_setupState = 2; g_setupSensX = L""; g_setupSensY = L"";
    g_focusedInput = 1; g_extractedConfig = false;

    // Fast GameUserSettings.ini extraction (Robust v4.20.34)
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        std::wstring folders[] = { L"\\WindowsClient\\", L"\\WindowsNoEditor\\" };
        std::wstring basePath = std::wstring(appdata) + L"\\FortniteGame\\Saved\\Config";
        
        for (const auto& f : folders) {
            std::wstring pPath = basePath + f + L"GameUserSettings.ini";
            std::ifstream ifs(pPath.c_str());
            if (ifs.good()) {
                std::string line;
                while (std::getline(ifs, line)) {
                    if (line.find("MouseSensitivityX=") != std::string::npos || line.find("MouseX=") != std::string::npos) {
                        size_t eq = line.find("=");
                        if (eq != std::string::npos) {
                            std::string val = line.substr(eq + 1);
                            if (g_setupSensX.empty()) g_setupSensX = std::wstring(val.begin(), val.end());
                        }
                    } else if (line.find("MouseSensitivityY=") != std::string::npos || line.find("MouseY=") != std::string::npos) {
                        size_t eq = line.find("=");
                        if (eq != std::string::npos) {
                            std::string val = line.substr(eq + 1);
                            if (g_setupSensY.empty()) g_setupSensY = std::wstring(val.begin(), val.end());
                        }
                    }
                }
                if (!g_setupSensX.empty() || !g_setupSensY.empty()) {
                    g_extractedConfig = true;
                    break;
                } else {
                    g_setupSensX = L"0.05";
                    g_setupSensY = L"0.05";
                }
            }
        }
    }

    WNDCLASS wc = { 0 };
    if (!GetClassInfo(hInstance, L"FTSWindowClass", &wc)) {
        wc.lpfnWndProc = FirstTimeSetupProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"FTSWindowClass";
        RegisterClass(&wc);
    }

    int W = 500, H = 310;
    s_hFTS = CreateWindowEx(WS_EX_TOPMOST | WS_EX_APPWINDOW, L"FTSWindowClass", L"BetterAngle Setup", WS_POPUP | WS_SYSMENU,
        GetSystemMetrics(SM_CXSCREEN)/2 - W/2, GetSystemMetrics(SM_CYSCREEN)/2 - H/2, W, H, NULL, NULL, hInstance, NULL);

    ShowWindow(s_hFTS, SW_SHOW);
    UpdateWindow(s_hFTS);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!IsWindow(s_hFTS)) break;
    }
}
