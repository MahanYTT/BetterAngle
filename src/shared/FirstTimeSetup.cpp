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
int g_focusedInput = 1;

// ─── Colors (matching mockup) ─────────────────────────────────────────────────
static const Color BG       (255, 13,  15,  20);   // #0D0F14
static const Color CARD     (255, 22,  25,  32);   // card layer
static const Color INPUT_BG (255, 30,  33,  42);   // input unfocused
static const Color ACCENT   (255, 59, 130, 246);   // blue #3B82F6
static const Color ACCENT_H (255, 37, 99,  235);   // hover blue
static const Color WHITE    (255, 255, 255, 255);
static const Color GRAY     (255, 120, 125, 140);
static const Color DIMGRAY  (255, 70,  75,  90);
static const Color CYAN     (255, 0,   210, 200);

void FinishSetup() {
    Profile p;
    p.name = L"Default";
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
    p.resolutionWidth  = GetSystemMetrics(SM_CXSCREEN);
    p.resolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    p.renderScale = 100.0f;

    extern std::vector<Profile> g_allProfiles;
    extern int g_selectedProfileIdx;

    if (g_allProfiles.empty()) {
        p.Save(GetAppStoragePath() + L"Default.json");
        g_allProfiles.push_back(p);
    } else {
        Profile& existing = g_allProfiles[g_selectedProfileIdx];
        p.name          = existing.name;
        p.roi_h         = existing.roi_h;
        p.roi_w         = existing.roi_w;
        p.roi_x         = existing.roi_x;
        p.roi_y         = existing.roi_y;
        p.target_color  = existing.target_color;
        p.tolerance     = existing.tolerance;
        g_allProfiles[g_selectedProfileIdx] = p;
        p.Save(GetAppStoragePath() + p.name + L".json");
    }
}

// ─── Rounded rectangle helper ────────────────────────────────────────────────
static void FillRoundRect(Graphics& g, Brush* br, int x, int y, int w, int h, int r) {
    GraphicsPath path;
    path.AddArc(x, y, r*2, r*2, 180, 90);
    path.AddArc(x+w-r*2, y, r*2, r*2, 270, 90);
    path.AddArc(x+w-r*2, y+h-r*2, r*2, r*2, 0, 90);
    path.AddArc(x, y+h-r*2, r*2, r*2, 90, 90);
    path.CloseFigure();
    g.FillPath(br, &path);
}

static void DrawRoundRect(Graphics& g, Pen* pen, int x, int y, int w, int h, int r) {
    GraphicsPath path;
    path.AddArc(x, y, r*2, r*2, 180, 90);
    path.AddArc(x+w-r*2, y, r*2, r*2, 270, 90);
    path.AddArc(x+w-r*2, y+h-r*2, r*2, r*2, 0, 90);
    path.AddArc(x, y+h-r*2, r*2, r*2, 90, 90);
    path.CloseFigure();
    g.DrawPath(pen, &path);
}

// ─── Draw a clean centered label ─────────────────────────────────────────────
static void DrawCentered(Graphics& g, Font* f, const wchar_t* text, SolidBrush* br,
                         int winW, int y, int maxW = 0) {
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    RectF layout((REAL)((winW - (maxW ? maxW : winW)) / 2),
                 (REAL)y,
                 (maxW ? (REAL)maxW : (REAL)winW),
                 100.0f);
    if (maxW == 0) {
        layout.X = 0;
        layout.Width = (REAL)winW;
    }
    g.DrawString(text, -1, f, layout, &sf, br);
}

LRESULT CALLBACK FirstTimeSetupProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    if (message == WM_MOUSEACTIVATE) return MA_ACTIVATE;

    if (message == WM_NCHITTEST) {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y < 50) return HTCAPTION;
        }
        return hit;
    }

    if (message == WM_CHAR) {
        if (wParam == VK_RETURN) {
            if (g_setupState == 1 && !g_setupDPI.empty()) {
                g_setupState = 2;
                g_focusedInput = 1;
            } else if (g_setupState == 2) {
                FinishSetup();
                DestroyWindow(hWnd);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        std::wstring* active = nullptr;
        if (g_setupState == 1) {
            active = &g_setupDPI;
        } else {
            active = (g_focusedInput == 1) ? &g_setupSensX : &g_setupSensY;
        }
        if (wParam == VK_BACK) {
            if (active && !active->empty()) active->pop_back();
        } else if (wParam >= 32 && wParam <= 126) {
            if (active) active->push_back((wchar_t)wParam);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    if (message == WM_LBUTTONDOWN) {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right;

        if (g_setupState == 1) {
            // Next button: centered, y=230, w=180, h=44
            int bx = (W - 180) / 2, by = 230;
            if (mx >= bx && mx <= bx+180 && my >= by && my <= by+44 && !g_setupDPI.empty()) {
                g_setupState = 2;
                g_focusedInput = 1;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else {
            // SensX field: left half, SensY: right half
            int fieldY = 175, fieldH = 42;
            int fieldXa = 30, fieldW2 = (W - 75) / 2;
            int fieldXb = fieldXa + fieldW2 + 15;

            if (my >= fieldY && my <= fieldY + fieldH) {
                if (mx >= fieldXa && mx <= fieldXa + fieldW2) { g_focusedInput = 1; InvalidateRect(hWnd, NULL, FALSE); }
                else if (mx >= fieldXb && mx <= fieldXb + fieldW2) { g_focusedInput = 2; InvalidateRect(hWnd, NULL, FALSE); }
            }
            // Confirm button: centered, y=245, w=180, h=44
            int bx = (W - 180) / 2, by = 250;
            if (mx >= bx && mx <= bx+180 && my >= by && my <= by+44) {
                FinishSetup();
                DestroyWindow(hWnd);
            }
        }
        SetFocus(hWnd);
        // Fall through for activation
    }

    if (message == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right, H = rc.bottom;

        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        // ── Background ────────────────────────────────────────────────
        g.Clear(BG);

        // ── Fonts ─────────────────────────────────────────────────────
        FontFamily ff(L"Segoe UI");
        Font fStep  (&ff, 11, FontStyleRegular, UnitPixel);
        Font fTitle (&ff, 30, FontStyleBold,    UnitPixel);
        Font fSub   (&ff, 13, FontStyleRegular, UnitPixel);
        Font fLabel (&ff, 12, FontStyleRegular, UnitPixel);
        Font fInput (&ff, 15, FontStyleRegular, UnitPixel);
        Font fBrand (&ff, 11, FontStyleRegular, UnitPixel);
        Font fBtn   (&ff, 14, FontStyleBold,    UnitPixel);

        SolidBrush brWhite  (WHITE);
        SolidBrush brGray   (GRAY);
        SolidBrush brDimGray(DIMGRAY);
        SolidBrush brAccent (ACCENT);
        SolidBrush brCyan   (CYAN);
        SolidBrush brInput  (INPUT_BG);

        // ── Step indicator ────────────────────────────────────────────
        const wchar_t* stepStr = (g_setupState == 1) ? L"Step 1 of 2" : L"Step 2 of 2";
        DrawCentered(g, &fStep, stepStr, &brGray, W, 50);

        // ── Step progress bar (thin, 2 segments) ──────────────────────
        int barW = 80, barH = 3, barY = 72;
        int barX = (W - barW * 2 - 6) / 2;
        SolidBrush brBarActive (ACCENT);
        SolidBrush brBarInact  (DIMGRAY);
        FillRoundRect(g, (g_setupState >= 1) ? &brBarActive : &brBarInact, barX, barY, barW, barH, 1);
        FillRoundRect(g, (g_setupState >= 2) ? &brBarActive : &brBarInact, barX + barW + 6, barY, barW, barH, 1);

        if (g_setupState == 1) {
            // ── Heading ───────────────────────────────────────────────
            DrawCentered(g, &fTitle, L"Mouse DPI", &brWhite, W, 90);

            // ── Subtitle ──────────────────────────────────────────────
            DrawCentered(g, &fSub, L"Enter the DPI from your mouse software or sensor label", &brGray, W, 135);

            // ── Input field ───────────────────────────────────────────
            int fx = 60, fw = W - 120, fy = 170, fh = 44;
            Pen borderPen(ACCENT, 1.5f);
            Pen dimBorderPen(DIMGRAY, 1.0f);
            FillRoundRect(g, &brInput, fx, fy, fw, fh, 8);
            DrawRoundRect(g, &borderPen, fx, fy, fw, fh, 8); // always focused on step 1

            std::wstring disp = g_setupDPI.empty() ? L"e.g. 800" : g_setupDPI + L"│";
            SolidBrush* textBr = g_setupDPI.empty() ? &brDimGray : &brWhite;

            StringFormat sfLeft;
            sfLeft.SetAlignment(StringAlignmentNear);
            sfLeft.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(disp.c_str(), -1, &fInput, RectF((REAL)(fx+14),(REAL)fy,(REAL)(fw-28),(REAL)fh), &sfLeft, textBr);

            // ── Next button ───────────────────────────────────────────
            int bw = 180, bh = 44, bx = (W-bw)/2, by = 230;
            SolidBrush brBtn(!g_setupDPI.empty() ? ACCENT : DIMGRAY);
            FillRoundRect(g, &brBtn, bx, by, bw, bh, 8);

            StringFormat sfC;
            sfC.SetAlignment(StringAlignmentCenter);
            sfC.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(L"Next  →", -1, &fBtn, RectF((REAL)bx,(REAL)by,(REAL)bw,(REAL)bh), &sfC, &brWhite);

        } else {
            // ── Heading ───────────────────────────────────────────────
            DrawCentered(g, &fTitle, L"Fortnite Sensitivity", &brWhite, W, 90);

            // ── Subtitle ──────────────────────────────────────────────
            const wchar_t* sub = g_extractedConfig
                ? L"We pre-filled your values from GameUserSettings.ini"
                : L"Enter your in-game sensitivity values below";
            DrawCentered(g, &fSub, sub, (g_extractedConfig ? &brCyan : &brGray), W, 135);

            // ── Two side-by-side input fields ─────────────────────────
            int fieldW2 = (W - 75) / 2;
            int fxA = 30, fxB = fxA + fieldW2 + 15;
            int fya = 175, fh = 42;

            Pen borderActive(ACCENT, 1.5f);
            Pen borderInact (DIMGRAY, 1.0f);

            // Label X
            g.DrawString(L"Sensitivity X", -1, &fLabel, PointF((REAL)fxA, 160.0f), &brGray);
            FillRoundRect(g, &brInput, fxA, fya, fieldW2, fh, 8);
            DrawRoundRect(g, (g_focusedInput == 1) ? &borderActive : &borderInact, fxA, fya, fieldW2, fh, 8);
            {
                std::wstring dx = g_setupSensX.empty() ? L"0.05" : g_setupSensX + (g_focusedInput==1 ? L"│" : L"");
                SolidBrush* tb = g_setupSensX.empty() ? &brDimGray : &brWhite;
                StringFormat sfL; sfL.SetAlignment(StringAlignmentNear); sfL.SetLineAlignment(StringAlignmentCenter);
                g.DrawString(dx.c_str(), -1, &fInput, RectF((REAL)(fxA+10),(REAL)fya,(REAL)(fieldW2-20),(REAL)fh), &sfL, tb);
            }

            // Label Y
            g.DrawString(L"Sensitivity Y", -1, &fLabel, PointF((REAL)fxB, 160.0f), &brGray);
            FillRoundRect(g, &brInput, fxB, fya, fieldW2, fh, 8);
            DrawRoundRect(g, (g_focusedInput == 2) ? &borderActive : &borderInact, fxB, fya, fieldW2, fh, 8);
            {
                std::wstring dy = g_setupSensY.empty() ? L"0.05" : g_setupSensY + (g_focusedInput==2 ? L"│" : L"");
                SolidBrush* tb = g_setupSensY.empty() ? &brDimGray : &brWhite;
                StringFormat sfL; sfL.SetAlignment(StringAlignmentNear); sfL.SetLineAlignment(StringAlignmentCenter);
                g.DrawString(dy.c_str(), -1, &fInput, RectF((REAL)(fxB+10),(REAL)fya,(REAL)(fieldW2-20),(REAL)fh), &sfL, tb);
            }

            // Tab hint
            g.DrawString(L"Press Tab to switch fields  ·  Enter to confirm", -1, &fLabel,
                         PointF(30.0f, 225.0f), &brDimGray);

            // ── Confirm button ────────────────────────────────────────
            int bw = 180, bh = 44, bx = (W-bw)/2, by = 250;
            FillRoundRect(g, &brAccent, bx, by, bw, bh, 8);
            StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter); sfC.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(L"Confirm  ✓", -1, &fBtn, RectF((REAL)bx,(REAL)by,(REAL)bw,(REAL)bh), &sfC, &brWhite);
        }

        // ── Brand watermark bottom-left ───────────────────────────────
        g.DrawString(L"BetterAngle Pro", -1, &fBrand, PointF(20.0f, (REAL)(H-26)), &brCyan);

        // ── Drag hint top-right ───────────────────────────────────────
        g.DrawString(L"⠿ drag", -1, &fBrand, PointF((REAL)(W-56), 10.0f), &brDimGray);

        EndPaint(hWnd, &ps);
        return 0;
    }

    if (message == WM_KEYDOWN && wParam == VK_TAB) {
        if (g_setupState == 2) {
            g_focusedInput = (g_focusedInput == 1) ? 2 : 1;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    if (message == WM_DESTROY) {
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
    g_setupState   = 1;
    g_setupDPI     = L"";
    g_setupSensX   = L"";
    g_setupSensY   = L"";
    g_focusedInput = 1;
    g_extractedConfig = false;

    // Auto-detect Fortnite sensitivity
    std::string iniContent;
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::wstring p = std::wstring(path) + L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";
        std::ifstream ifs(p.c_str());
        if (ifs.good())
            iniContent.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }
    auto extractValue = [&](const std::string& key) -> std::wstring {
        size_t pos = iniContent.find(key + "=");
        if (pos != std::string::npos) {
            size_t end = iniContent.find_first_of("\r\n", pos);
            std::string val = iniContent.substr(pos + key.length() + 1,
                                                 end - (pos + key.length() + 1));
            return std::wstring(val.begin(), val.end());
        }
        return L"";
    };
    g_setupSensX = extractValue("MouseX");
    g_setupSensY = extractValue("MouseY");
    if (!g_setupSensX.empty() || !g_setupSensY.empty())
        g_extractedConfig = true;

    WNDCLASS wc = { 0 };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = FirstTimeSetupProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(13, 15, 20));
    wc.lpszClassName = L"FTSWindowClass";
    RegisterClass(&wc);

    int W = 500, H = 330;
    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"FTSWindowClass", L"BetterAngle — First Time Setup",
        WS_POPUP,
        GetSystemMetrics(SM_CXSCREEN)/2 - W/2,
        GetSystemMetrics(SM_CYSCREEN)/2 - H/2,
        W, H, NULL, NULL, hInstance, NULL
    );

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    BringWindowToTop(hWnd);
    SetFocus(hWnd);

    StartModalSetupLoop(hWnd);
}
