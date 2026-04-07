#ifndef STATE_H
#define STATE_H

#include <windows.h>
#include <atomic>
#include <vector>
#include <string>

// HUD & Global Shared State
extern bool g_isDiving;
extern bool g_showROIBox;
extern int g_currentTab;
extern float g_detectionRatio;
extern bool g_showCrosshair;
extern bool g_isSelectionMode;
extern int g_selectionStep; // 0 = ROI, 1 = Color
extern COLORREF g_pickedColor;
extern COLORREF g_targetColor;
extern float g_latestVersion;
extern std::wstring g_latestName;
extern RECT g_selectionRect;
extern POINT g_startPoint;
extern std::string g_status;
extern float g_currentAngle;
extern std::string g_latestVersionOnline;

#endif // STATE_H
