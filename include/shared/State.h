#ifndef STATE_H
#define STATE_H

#include <windows.h>
#include <atomic>
#include <vector>
#include <string>

// HUD & Global Shared State
enum SelectionState { NONE, SELECTING_ROI, SELECTING_COLOR };
extern SelectionState g_currentSelection;
extern bool g_isSelectionActive;
extern bool g_isDiving;
extern bool g_showROIBox;
extern int g_currentTab;
extern float g_detectionRatio;
extern bool g_isCheckingForUpdates;
extern float g_updateSpinAngle;
extern bool g_updateAvailable;
extern bool g_showCrosshair;
extern COLORREF g_pickedColor;
extern COLORREF g_targetColor;
extern float g_latestVersion;
extern std::wstring g_latestName;
extern RECT g_selectionRect;
extern POINT g_startPoint;
extern std::string g_status;
extern std::string g_latestVersionOnline;
extern float g_currentAngle;

#endif // STATE_H
