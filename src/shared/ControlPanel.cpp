#include "shared/State.h"
#include "shared/Overlay.h"
#include "shared/Config.h"
#include "shared/ControlPanel.h"
#include "shared/Updater.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace D2D1;

ID2D1Factory* g_pD2DFactory = NULL;
IDWriteFactory* g_pDWriteFactory = NULL;
ID2D1HwndRenderTarget* g_pRenderTarget = NULL;

// UI State
bool g_isCheckingUpdate = false;

void InitD2D(HWND hWnd) {
    if (!g_pD2DFactory) {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_pDWriteFactory);
    }
    RECT rc; GetClientRect(hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    if (g_pRenderTarget) g_pRenderTarget->Release();
    g_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &g_pRenderTarget);
}

void DrawD2DButton(ID2D1HwndRenderTarget* rt, D2D1_RECT_F rect, const wchar_t* text, D2D1_COLOR_F color) {
    ID2D1SolidColorBrush* pBrush = NULL;
    rt->CreateSolidColorBrush(color, &pBrush);
    
    rt->FillRoundedRectangle(D2D1::RoundedRect(rect, 8.0f, 8.0f), pBrush);
    
    ID2D1SolidColorBrush* pWhite = NULL;
    rt->CreateSolidColorBrush(ColorF(ColorF::White), &pWhite);
    
    IDWriteTextFormat* pTextFormat = NULL;
    g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &pTextFormat);
    pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    rt->DrawText(text, (UINT32)wcslen(text), pTextFormat, rect, pWhite);
    
    pTextFormat->Release();
    pWhite->Release();
    pBrush->Release();
}

HWND CreateControlPanel(HINSTANCE hInst) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ControlPanelWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"BetterAngleControlPanel";
    RegisterClass(&wc);

    int w = 420, h = 680;
    HWND hPanel = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"BetterAngleControlPanel", L"BetterAngle Pro | Global Command Center",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hPanel, SW_SHOW);
    UpdateWindow(hPanel);
    return hPanel;
}

LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            InitD2D(hWnd);
            SetTimer(hWnd, 1, 100, NULL);
            return 0;

        case WM_SIZE:
            InitD2D(hWnd);
            return 0;

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            // Check For Updates Button
            if (x >= 40 && x <= 380 && y >= 320 && y <= 370) {
                g_isCheckingUpdate = true;
                CheckForUpdates();
            }
            // Update Now Button (Conditional)
            if (g_latestVersion > 4.81f && x >= 40 && x <= 380 && y >= 380 && y <= 430) {
                ApplyUpdateAndRestart();
            }
            // Quit Button
            if (x >= 40 && x <= 380 && y >= 580 && y <= 630) {
                PostQuitMessage(0);
            }
            return 0;
        }

        case WM_PAINT: {
            if (!g_pRenderTarget) return 0;
            g_pRenderTarget->BeginDraw();
            g_pRenderTarget->Clear(D2D1::ColorF(0.05f, 0.06f, 0.07f));

            ID2D1SolidColorBrush* pWhite = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhite);
            ID2D1SolidColorBrush* pGrey = NULL;
            g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &pGrey);

            IDWriteTextFormat* pTitleFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"en-us", &pTitleFormat);

            g_pRenderTarget->DrawText(L"Pro Command Center", 18, pTitleFormat, D2D1::RectF(40, 40, 380, 80), pWhite);
            
            // Software & Updates Section
            IDWriteTextFormat* pHeaderFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &pHeaderFormat);
            g_pRenderTarget->DrawText(L"SOFTWARE & UPDATES", 18, pHeaderFormat, D2D1::RectF(40, 120, 380, 150), pWhite);

            std::wstring curVer = L"Current Version: v4.8.2 (Release Reliability)";
            std::wstring latestVer = L"Latest Found: v" + std::to_wstring(g_latestVersion).substr(0, 4) + L" (" + g_latestName + L")";
            
            IDWriteTextFormat* pVerFormat = NULL;
            g_pDWriteFactory->CreateTextFormat(L"Segoe UI Variable Display", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", &pVerFormat);
            
            g_pRenderTarget->DrawText(curVer.c_str(), (UINT32)curVer.length(), pVerFormat, D2D1::RectF(40, 160, 380, 180), pGrey);
            g_pRenderTarget->DrawText(latestVer.c_str(), (UINT32)latestVer.length(), pVerFormat, D2D1::RectF(40, 185, 380, 205), pGrey);

            // Buttons
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 320, 380, 370), L"CHECK FOR UPDATES", D2D1::ColorF(0.15f, 0.17f, 0.2f));
            
            if (g_latestVersion > 4.82f) {
                DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 380, 380, 430), L"UPDATE NOW", D2D1::ColorF(0.0f, 0.5f, 0.8f));
            }

            // High-Resolution Liquid QUIT Button
            DrawD2DButton(g_pRenderTarget, D2D1::RectF(40, 580, 380, 630), L"QUIT SUITE", D2D1::ColorF(0.7f, 0.1f, 0.15f));

            pVerFormat->Release();
            pHeaderFormat->Release();
            pTitleFormat->Release();
            pGrey->Release();
            pWhite->Release();
            g_pRenderTarget->EndDraw();
            ValidateRect(hWnd, NULL);
            return 0;
        }

        case WM_TIMER:
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case WM_CLOSE:
            ShowWindow(hWnd, SW_MINIMIZE);
            return 0;

        case WM_DESTROY:
            if (g_pRenderTarget) g_pRenderTarget->Release();
            return 0;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
