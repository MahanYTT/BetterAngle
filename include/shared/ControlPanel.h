#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <windows.h>
#include <string>

class QQmlApplicationEngine;

HWND CreateControlPanel(HINSTANCE hInst);
void ShowControlPanel();
void ShowSplashScreen();
void EnsureEngineInitialized();
LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

extern HINSTANCE g_hInstance;
extern QQmlApplicationEngine* g_qmlEngine;

#endif
