#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <dwmapi.h>
#include <gdiplus.h>

#include "shared/Input.h"
#include "shared/Overlay.h"
#include "shared/Logic.h"
#include "shared/Detector.h"
#include "shared/Profile.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// State
int g_step = 1; 
long long g_dxA = 0, g_dxB = 0;
Profile g_result;
std::wstring g_sensInput = L"";
std::wstring g_dpiInput = L"";
std::wstring g_wizardMsg = L"STEP 1: Type your IN-GAME SENSITIVITY (e.g., 2.2) and press 'ENTER'.";

// Message-Only Window for Bullet-Proof Raw Input
LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INPUT) {
        int dx = GetRawInputDeltaX(lParam);
        if (g_step == 3) g_dxA += dx;
        if (g_step == 4) g_dxB += dx;
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Window Procedure
LRESULT CALLBACK ConfigWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            return 0;

        case WM_CHAR:
            if (g_step == 1) {
                if (wParam == VK_BACK && !g_sensInput.empty()) g_sensInput.pop_back();
                else if ((wParam >= '0' && wParam <= '9') || wParam == '.') g_sensInput += (wchar_t)wParam;
            } else if (g_step == 2) {
                if (wParam == VK_BACK && !g_dpiInput.empty()) g_dpiInput.pop_back();
                else if (wParam >= '0' && wParam <= '9') g_dpiInput += (wchar_t)wParam;
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                if (g_step == 1 && !g_sensInput.empty()) {
                    g_step = 2;
                    g_wizardMsg = L"STEP 2: Type your MOUSE DPI and press 'ENTER'.";
                } else if (g_step == 2 && !g_dpiInput.empty()) {
                    g_step = 3;
                    g_wizardMsg = L"STEP 3: Point your camera exactly at 0 degrees and press 'ENTER'.";
                } else if (g_step == 3) {
                    g_step = 4;
                    g_wizardMsg = L"STEP 4: Turn exactly 120 degrees and press 'ENTER'.";
                } else if (g_step == 4) {
                    g_step = 5;
                    double diff = (double)g_dxB - (double)g_dxA;
                    g_result.scale_normal = 120.0 / diff;
                    g_result.scale_diving = g_result.scale_normal * 1.5; 
                    g_result.name = L"SENS:" + g_sensInput + L" DPI:" + g_dpiInput;
                    g_result.roi_x = 700; g_result.roi_y = 800; g_result.roi_w = 500; g_result.roi_h = 60;
                    g_result.target_color = RGB(150, 150, 150); g_result.tolerance = 25;
                    g_wizardMsg = L"STEP 5: Calibration Complete! Press 'S' to Save.";
                }
            } else if (wParam == 'S' && g_step == 5) {
                CreateDirectoryW(L"profiles", NULL);
                std::wstring safeFilename = L"SENS_" + g_sensInput + L"_DPI_" + g_dpiInput + L".json";
                std::wstring path = L"profiles/" + safeFilename;
                g_result.Save(path);
                g_wizardMsg = L"DONE! Profile saved to 'profiles/" + safeFilename + L"'.";
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Graphics graphics(hdc);
            graphics.Clear(Color(200, 30, 34, 40));

            FontFamily ff(L"Segoe UI");
            Font f(&ff, 24, FontStyleBold, UnitPixel);
            SolidBrush b(Color(255, 255, 255, 255));
            graphics.DrawString(L"BETTERANGLE CONFIG TOOL (V3.5 PRO)", -1, &f, PointF(50, 50), &b);
            
            Font f2(&ff, 18, FontStyleRegular, UnitPixel);
            graphics.DrawString(g_wizardMsg.c_str(), -1, &f2, PointF(50, 100), &b);

            if (g_step == 1) {
                std::wstring txt = L"[" + g_sensInput + L"]";
                graphics.DrawString(txt.c_str(), -1, &f2, PointF(50, 150), &b);
            } else if (g_step == 2) {
                std::wstring txt = L"[" + g_dpiInput + L"]";
                graphics.DrawString(txt.c_str(), -1, &f2, PointF(50, 150), &b);
            }

            std::wstring statusLine = L"System Diagnostics: Scale Factor Pending | Raw Acc = " + std::to_wstring(g_dxA + g_dxB);
            if (g_step == 5) statusLine = L"Resulting Normal Scale: " + std::to_wstring(g_result.scale_normal);
            graphics.DrawString(statusLine.c_str(), -1, &f2, PointF(50, 250), &b);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    ULONG_PTR token;
    GdiplusStartupInput input;
    GdiplusStartup(&token, &input, NULL);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ConfigWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BetterAngleConfig";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"BetterAngleConfig", L"BetterAngle Calibration Wizard",
        WS_POPUP,
        100, 100, 800, 400,
        NULL, NULL, hInstance, NULL
    );

    WNDCLASS wcMsg = { 0 };
    wcMsg.lpfnWndProc = MsgWndProc;
    wcMsg.hInstance = hInstance;
    wcMsg.lpszClassName = L"BetterAngleConfigMsgWnd";
    RegisterClass(&wcMsg);
    HWND hMsgWnd = CreateWindowEx(0, L"BetterAngleConfigMsgWnd", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    RegisterRawMouse(hMsgWnd);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(token);
    return (int)msg.wParam;
}
