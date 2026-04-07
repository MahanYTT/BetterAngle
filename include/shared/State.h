#ifndef STATE_H
#define STATE_H

#include <windows.h>
#include <atomic>
#include <vector>
#include <string>

// HUD & Global Shared State
extern float g_detectionRatio;
extern bool g_showCrosshair;
extern bool g_isSelectionMode;
extern int g_selectionStep; // 0 = ROI, 1 = Color
extern COLORREF g_pickedColor;
extern RECT g_selectionRect;
extern POINT g_startPoint;
extern std::string g_status;

#endif // STATE_H
