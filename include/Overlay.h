#ifndef OVERLAY_H
#define OVERLAY_H

#include <windows.h>
#include <gdiplus.h>

void InitializeOverlay(HWND hwnd);
void CleanupOverlay();
void DrawOverlay(HWND hwnd, double angle, const char* status);

#endif // OVERLAY_H
