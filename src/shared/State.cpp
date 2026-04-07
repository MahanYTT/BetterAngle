#include "shared/State.h"

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
float g_latestVersion = 4.914f; // v4.9.14 format fallback
std::wstring g_latestName = L"Pending Scan";
RECT g_selectionRect = { 0, 0, 0, 0 };
POINT g_startPoint = { 0 };
std::string g_status = "Connected (v4.9.14 Pro)";
float g_currentAngle = 0.0f;
std::string g_latestVersionOnline = "v4.9.14";