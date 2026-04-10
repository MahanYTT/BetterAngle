#include "shared/FirstTimeSetup.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <fstream>
#include <shlobj.h>
#include "shared/Profile.h"
#include "shared/State.h"

// ─── Pure GDI colours (no GDI+, zero GPU cost) ───────────────────────────
static const COLORREF C_BG      = RGB( 13,  15,  20);
static const COLORREF C_INPUT   = RGB( 28,  31,  40);
static const COLORREF C_ACCENT  = RGB( 59, 130, 246);
static const COLORREF C_DIM     = RGB( 55,  60,  75);
static const COLORREF C_GRAY    = RGB(110, 115, 135);
static const COLORREF C_WHITE   = RGB(255, 255, 255);
static const COLORREF C_CYAN    = RGB(  0, 210, 200);
static const COLORREF C_BTNOFF  = RGB( 40,  44,  55);

int g_setupState    = 1;
std::wstring g_setupDPI   = L"";
std::wstring g_setupSensX = L"";
std::wstring g_setupSensY = L"";
bool g_extractedConfig    = false;
int  g_focusedInput       = 1;

// ── GDI helpers ───────────────────────────────────────────────────────────
static void FillR(HDC hdc, int x, int y, int w, int h, COLORREF c) {
    RECT r = { x, y, x+w, y+h };
    HBRUSH br = CreateSolidBrush(c);
    FillRect(hdc, &r, br);
    DeleteObject(br);
}

static void DrawT(HDC hdc, const wchar_t* t, int x, int y, int w, int h,
                  COLORREF c, int sz, bool bold = false, UINT fmt = DT_CENTER|DT_VCENTER|DT_SINGLELINE) {
    HFONT f = CreateFontW(-sz, 0, 0, 0,
                          bold ? FW_BOLD : FW_NORMAL,
                          0, 0, 0, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH,
                          L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, f);
    SetTextColor(hdc, c);
    SetBkMode(hdc, TRANSPARENT);
    RECT r = { x, y, x+w, y+h };
    DrawTextW(hdc, t, -1, &r, fmt);
    SelectObject(hdc, old);
    DeleteObject(f);
}

// ── Profile save ─────────────────────────────────────────────────────────
void FinishSetup() {
    Profile p;
    p.name = L"Default"; p.tolerance = 25;
    p.roi_h = 60; p.roi_w = 400; p.roi_x = 760; p.roi_y = 650;
    p.target_color = RGB(150, 150, 150);

    double dpiVal = 800.0, sensX = 0.05, sensY = 0.05;
    try { dpiVal = std::stod(g_setupDPI); }  catch(...) {}
    try { sensX  = std::stod(g_setupSensX); } catch(...) {}
    try { sensY  = std::stod(g_setupSensY); } catch(...) {}
    if (dpiVal <= 0) dpiVal = 800.0;
    if (sensX  <= 0) sensX  = 0.05;
    if (sensY  <= 0) sensY  = 0.05;

    p.dpi = (int)dpiVal; p.sensitivityX = sensX; p.sensitivityY = sensY;
    p.divingScaleMultiplier = 1.22; p.fov = 80.0f;
    p.resolutionWidth  = GetSystemMetrics(SM_CXSCREEN);
    p.resolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    p.renderScale = 100.0f;

    extern std::vector<Profile> g_allProfiles;
    extern int g_selectedProfileIdx;
    if (g_allProfiles.empty()) {
        p.Save(GetAppStoragePath() + L"Default.json");
        g_allProfiles.push_back(p);
    } else {
        Profile& e = g_allProfiles[g_selectedProfileIdx];
        p.name = e.name; p.roi_h = e.roi_h; p.roi_w = e.roi_w;
        p.roi_x = e.roi_x; p.roi_y = e.roi_y;
        p.target_color = e.target_color; p.tolerance = e.tolerance;
        g_allProfiles[g_selectedProfileIdx] = p;
        p.Save(GetAppStoragePath() + p.name + L".json");
    }
}

// ── Paint (pure GDI — no GPU, no GDI+) ───────────────────────────────────
static void PaintSetup(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    // Off-screen buffer to avoid flicker
    HDC     buf = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, W, H);
    HGDIOBJ old = SelectObject(buf, bmp);

    // Background
    FillR(buf, 0, 0, W, H, C_BG);

    // Progress pills (2px flat rectangles)
    COLORREF p1 = (g_setupState >= 1) ? C_ACCENT : C_DIM;
    COLORREF p2 = (g_setupState >= 2) ? C_ACCENT : C_DIM;
    int pilX = (W - 148) / 2;
    FillR(buf, pilX,      28, 70, 2, p1);
    FillR(buf, pilX + 78, 28, 70, 2, p2);

    // Step label
    DrawT(buf, (g_setupState == 1) ? L"Step 1 of 2" : L"Step 2 of 2",
          0, 36, W, 18, C_GRAY, 11);

    // Heading
    const wchar_t* hd = (g_setupState == 1) ? L"Mouse DPI" : L"Sensitivity";
    DrawT(buf, hd, 0, 60, W, 34, C_WHITE, 26, true);

    // Thin separator
    FillR(buf, 30, 103, W - 60, 1, C_DIM);

    if (g_setupState == 1) {
        // Subtitle
        DrawT(buf, L"Enter the DPI from your mouse software or sensor label",
              30, 113, W-60, 20, C_GRAY, 11, false, DT_CENTER | DT_SINGLELINE);

        // Input box
        int ix = 30, iw = W-60, iy = 142, ih = 40;
        FillR(buf, ix, iy, iw, ih, C_INPUT);
        // 1px accent border
        HPEN pen = CreatePen(PS_SOLID, 1, C_ACCENT);
        HPEN op  = (HPEN)SelectObject(buf, pen);
        HBRUSH bn = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob = (HBRUSH)SelectObject(buf, bn);
        RECT br = { ix, iy, ix+iw, iy+ih };
        Rectangle(buf, ix, iy, ix+iw, iy+ih);
        SelectObject(buf, op); SelectObject(buf, ob);
        DeleteObject(pen);

        std::wstring val = g_setupDPI.empty() ? L"e.g. 800" : g_setupDPI + L"|";
        COLORREF tc = g_setupDPI.empty() ? C_DIM : C_WHITE;
        DrawT(buf, val.c_str(), ix+12, iy, iw-24, ih, tc, 14, false, DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        // Next button
        bool rdy = !g_setupDPI.empty();
        int bx = (W-180)/2, by = H-74, bw = 180, bh = 40;
        FillR(buf, bx, by, bw, bh, rdy ? C_ACCENT : C_BTNOFF);
        DrawT(buf, L"Next  →", bx, by, bw, bh, C_WHITE, 13, true);

    } else {
        // Subtitle
        const wchar_t* sub = g_extractedConfig
            ? L"Pre-filled from GameUserSettings.ini"
            : L"Enter your in-game sensitivity values";
        COLORREF sc = g_extractedConfig ? C_CYAN : C_GRAY;
        DrawT(buf, sub, 30, 113, W-60, 20, sc, 11, false, DT_CENTER|DT_SINGLELINE);

        // Two side-by-side inputs
        int fw = (W - 75) / 2;
        int fxA = 30,      fxB = fxA + fw + 15;
        int fy = 170, fh = 40;

        // Labels
        DrawT(buf, L"Sensitivity X", fxA, 154, fw, 16, C_GRAY, 10, false, DT_LEFT|DT_SINGLELINE);
        DrawT(buf, L"Sensitivity Y", fxB, 154, fw, 16, C_GRAY, 10, false, DT_LEFT|DT_SINGLELINE);

        auto drawField = [&](int fx, int focusIdx, const std::wstring& val) {
            FillR(buf, fx, fy, fw, fh, C_INPUT);
            HPEN pen = CreatePen(PS_SOLID, 1, (g_focusedInput == focusIdx) ? C_ACCENT : C_DIM);
            HPEN op  = (HPEN)SelectObject(buf, pen);
            HBRUSH bn = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH ob = (HBRUSH)SelectObject(buf, bn);
            Rectangle(buf, fx, fy, fx+fw, fy+fh);
            SelectObject(buf, op); SelectObject(buf, ob); DeleteObject(pen);
            std::wstring d = val.empty() ? L"0.05" : val + (g_focusedInput==focusIdx ? L"|" : L"");
            COLORREF tc = val.empty() ? C_DIM : C_WHITE;
            DrawT(buf, d.c_str(), fx+10, fy, fw-20, fh, tc, 13, false, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        };
        drawField(fxA, 1, g_setupSensX);
        drawField(fxB, 2, g_setupSensY);

        // Tab hint
        DrawT(buf, L"Tab to switch  ·  Enter to confirm",
              30, fy+fh+8, W-60, 16, C_DIM, 10, false, DT_CENTER|DT_SINGLELINE);

        // Confirm button
        int bx = (W-180)/2, by = H-74, bw = 180, bh = 40;
        FillR(buf, bx, by, bw, bh, C_ACCENT);
        DrawT(buf, L"Confirm  ✓", bx, by, bw, bh, C_WHITE, 13, true);
    }

    // Brand
    DrawT(buf, L"BetterAngle Pro", 14, H-20, 160, 16, C_CYAN, 10, false, DT_LEFT|DT_SINGLELINE);
    // Drag hint
    DrawT(buf, L"drag ↑", W-52, 6, 46, 14, C_DIM, 9, false, DT_LEFT|DT_SINGLELINE);

    BitBlt(hdc, 0, 0, W, H, buf, 0, 0, SRCCOPY);
    SelectObject(buf, old);
    DeleteObject(bmp);
    DeleteDC(buf);
    EndPaint(hWnd, &ps);
}

// ── Window Procedure ──────────────────────────────────────────────────────
LRESULT CALLBACK FirstTimeSetupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    if (msg == WM_ACTIVATE) {
        if (LOWORD(wParam) == WA_INACTIVE)
            SetWindowPos(hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        return 0;
    }
    if (msg == WM_NCACTIVATE)   return TRUE;
    if (msg == WM_MOUSEACTIVATE) return MA_ACTIVATE;

    if (msg == WM_NCHITTEST) {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y < 52) return HTCAPTION;
        }
        return hit;
    }

    if (msg == WM_KEYDOWN && wParam == VK_TAB && g_setupState == 2) {
        g_focusedInput = (g_focusedInput == 1) ? 2 : 1;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    if (msg == WM_CHAR) {
        if (wParam == VK_RETURN) {
            if (g_setupState == 1 && !g_setupDPI.empty()) {
                g_setupState = 2; g_focusedInput = 1;
            } else if (g_setupState == 2) {
                FinishSetup(); DestroyWindow(hWnd);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        std::wstring* cur = (g_setupState == 1) ? &g_setupDPI
                          : (g_focusedInput == 1) ? &g_setupSensX : &g_setupSensY;
        if (wParam == VK_BACK) {
            if (!cur->empty()) cur->pop_back();
        } else if (wParam >= 32 && wParam <= 126) {
            cur->push_back((wchar_t)wParam);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    if (msg == WM_LBUTTONDOWN) {
        int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right, H = rc.bottom;

        if (g_setupState == 1) {
            int bx = (W-180)/2, by = H-74;
            if (mx>=bx && mx<=bx+180 && my>=by && my<=by+40 && !g_setupDPI.empty()) {
                g_setupState = 2; g_focusedInput = 1;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else {
            int fw = (W-75)/2, fxA = 30, fxB = fxA+fw+15, fy = 170, fh = 40;
            if (my >= fy && my <= fy+fh) {
                if      (mx>=fxA && mx<=fxA+fw) { g_focusedInput=1; InvalidateRect(hWnd,NULL,FALSE); }
                else if (mx>=fxB && mx<=fxB+fw) { g_focusedInput=2; InvalidateRect(hWnd,NULL,FALSE); }
            }
            int bx=(W-180)/2, by=H-74;
            if (mx>=bx && mx<=bx+180 && my>=by && my<=by+40) {
                FinishSetup(); DestroyWindow(hWnd);
            }
        }
        SetFocus(hWnd);
    }

    if (msg == WM_PAINT) { PaintSetup(hWnd); return 0; }
    if (msg == WM_DESTROY) return 0;
    return DefWindowProc(hWnd, msg, wParam, lParam);
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
    g_setupState = 1; g_setupDPI = L""; g_setupSensX = L"";
    g_setupSensY = L""; g_focusedInput = 1; g_extractedConfig = false;

    std::string ini;
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        std::wstring p = std::wstring(appdata) +
            L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";
        std::ifstream ifs(p.c_str());
        if (ifs.good())
            ini.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }
    auto extract = [&](const std::string& key) -> std::wstring {
        size_t pos = ini.find(key + "=");
        if (pos == std::string::npos) return L"";
        size_t end = ini.find_first_of("\r\n", pos);
        std::string val = ini.substr(pos + key.size() + 1, end - (pos + key.size() + 1));
        return std::wstring(val.begin(), val.end());
    };
    g_setupSensX = extract("MouseX");
    g_setupSensY = extract("MouseY");
    if (!g_setupSensX.empty() || !g_setupSensY.empty()) g_extractedConfig = true;

    WNDCLASS wc     = { 0 };
    wc.style        = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc  = FirstTimeSetupProc;
    wc.hInstance    = hInstance;
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground= (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName= L"FTSWindowClass";
    RegisterClass(&wc);

    int W = 500, H = 310;
    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"FTSWindowClass", L"BetterAngle — Setup",
        WS_POPUP | WS_SYSMENU,
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
