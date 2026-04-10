#ifndef STATE_H
#define STATE_H

#include "shared/Logic.h"
#include <atomic>
#include <string>
#include <vector>
#include <windows.h>


// --- Inside State.h ---
#define STRING_HELPER(x) #x
#define TO_STRING(x) STRING_HELPER(x)
#define TO_WSTRING(x) L"" TO_STRING(x)
#include <string>

std::wstring GetAppStoragePath();

// Define the "raw" version if not passed by compiler flags
#ifndef APP_VERSION
#define APP_VERSION 4.18.0
#endif

// This creates the actual strings "4.9.36" and L"4.9.36"
#define VERSION_STR TO_STRING(APP_VERSION)
#define VERSION_WSTR TO_WSTRING(APP_VERSION)

// HUD & Global Shared State
enum SelectionState { NONE, SELECTING_ROI, SELECTING_COLOR };
extern SelectionState g_currentSelection;
extern bool g_isSelectionActive;
extern HBITMAP g_screenSnapshot;
extern bool g_isDiving;
extern bool g_showROIBox;
extern int g_currentTab;
extern std::wstring g_latestName;
extern bool g_isCheckingForUpdates;
extern bool g_updateAvailable;
extern float g_freefallThreshold;
extern float g_glideThreshold;
struct Keybinds {
  UINT toggleMod = MOD_CONTROL;
  UINT toggleKey = 'U';
  UINT roiMod = MOD_CONTROL;
  UINT roiKey = 'R';
  UINT crossMod = 0;
  UINT crossKey = VK_F10;
  UINT zeroMod = MOD_CONTROL;
  UINT zeroKey = 'G';
  UINT debugMod = MOD_CONTROL;
  UINT debugKey = '9';
};
extern Keybinds g_keybinds;
extern std::wstring g_lastLoadedProfileName;

void LoadSettings();
void SaveSettings();
extern bool g_updateAvailable;
extern bool g_showCrosshair;
extern COLORREF g_targetColor;
extern COLORREF g_pickedColor;
extern float g_latestVersion;
extern float g_detectionRatio;
extern float g_updateSpinAngle;
extern RECT g_selectionRect;
extern POINT g_startPoint;
extern std::string g_latestVersionOnline;
extern float g_currentAngle;
extern bool g_debugMode;
extern bool g_isCursorVisible;
extern AngleLogic g_logic;
extern bool g_forceDiving;
extern bool g_forceDetection;

#endif // STATE_H
