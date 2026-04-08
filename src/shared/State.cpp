#include "shared/State.h"
#include "shared/Logic.h"

SelectionState g_currentSelection = NONE;
bool g_isSelectionActive = false;
HBITMAP g_screenSnapshot = NULL;
bool g_isDiving = false;
bool g_showROIBox = true;
int g_currentTab = 0;
float g_detectionRatio = 0.0f;
bool g_isCheckingForUpdates = false;
float g_updateSpinAngle = 0.0f;
bool g_updateAvailable = false;
bool g_showCrosshair = false;
COLORREF g_pickedColor = RGB(255, 255, 255);
COLORREF g_targetColor = RGB(255, 255, 255);
float g_latestVersion = 4.920f; // v4.9.20 format fallback
std::wstring g_latestName = L"Pending Scan";
RECT g_selectionRect = { 0, 0, 0, 0 };
POINT g_startPoint = { 0 };
#ifndef APP_VERSION_STR
#define APP_VERSION_STR "4.9.36"
#endif

std::string g_status = "Connected (v" APP_VERSION_STR " Pro)";
float g_currentAngle = 0.0f;
bool g_debugMode = false;
bool g_isCursorVisible = false;
std::string g_latestVersionOnline = "v" APP_VERSION_STR;
AngleLogic g_logic(800, 6.5);
