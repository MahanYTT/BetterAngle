#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <string>
#include <windows.h>


class QQmlApplicationEngine;

HWND CreateControlPanel(HINSTANCE hInst);
void ShowControlPanel();
void ShowSplashScreen();
void CloseSplashScreenDirect();
void EnsureEngineInitialized();
LRESULT CALLBACK ControlPanelWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                     LPARAM lParam);

extern HINSTANCE g_hInstance;
extern QQmlApplicationEngine *g_qmlEngine;

void LogStartup(const std::string &msg);

#endif
