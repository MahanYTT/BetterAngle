#include "shared/State.h"

bool g_isDiving = false;
bool g_showROIBox = true;
int g_currentTab = 0;
float g_detectionRatio = 0.0f;
bool g_isCheckingForUpdates = false;
float g_updateSpinAngle = 0.0f;
bool g_updateAvailable = false;
bool g_showCrosshair = false;
bool g_isSelectionMode = false;
int g_selectionStep = 0;
COLORREF g_pickedColor = RGB(255, 0, 255);
COLORREF g_targetColor = RGB(255, 0, 255);
float g_latestVersion = 4.96f;
std::wstring g_latestName = L"Pending Scan";
RECT g_selectionRect = { 0 };
POINT g_startPoint = { 0 };
std::string g_status = "Connected (v4.9.6 Pro)";
float g_currentAngle = 0.0f;
std::string g_latestVersionOnline = "v4.9.6";