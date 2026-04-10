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
bool g_hasCheckedForUpdates = false;
float g_updateSpinAngle = 0.0f;
bool g_updateAvailable = false;
std::atomic<bool> g_fortniteFocusedCache(false);

Profile g_currentProfile;
std::vector<Profile> g_allProfiles;
int g_selectedProfileIdx = 0;

Keybinds g_keybinds;
std::wstring g_lastLoadedProfileName = L"";
float g_glideThreshold = 0.05f;
float g_freefallThreshold = 0.20f;

#include <fstream>
#include <string>

#include <shlobj.h>
#pragma comment(lib, "shell32.lib")

std::wstring GetAppStoragePath() {
    wchar_t out[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, out))) {
        std::wstring path = std::wstring(out) + L"\\BetterAngle";
        CreateDirectoryW(path.c_str(), NULL);
        path += L"\\profiles";
        CreateDirectoryW(path.c_str(), NULL);
        return path + L"\\";
    }
    return L"profiles\\"; // Fallback to local
}

void LoadSettings() {
  std::wstring sp = GetAppStoragePath() + L"settings.json";
  std::ifstream ifs(sp.c_str());
  if (!ifs.is_open())
    return;
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto eFloat = [&](std::string k, float def) -> float {
    size_t p = content.find("\"" + k + "\":");
    if (p == std::string::npos)
      return def;
    try {
      return std::stof(content.substr(p + k.length() + 2));
    } catch (...) {
      return def;
    }
  };
  auto eInt = [&](std::string k, UINT def) -> UINT {
    size_t p = content.find("\"" + k + "\":");
    if (p == std::string::npos)
      return def;
    try {
      return std::stoi(content.substr(p + k.length() + 2));
    } catch (...) {
      return def;
    }
  };
  g_keybinds.toggleMod = eInt("toggleMod", MOD_CONTROL);
  g_keybinds.toggleKey = eInt("toggleKey", 'U');
  g_keybinds.roiMod = eInt("roiMod", MOD_CONTROL);
  g_keybinds.roiKey = eInt("roiKey", 'R');
  g_keybinds.crossMod = eInt("crossMod", 0);
  g_keybinds.crossKey = eInt("crossKey", VK_F10);
  g_keybinds.zeroMod = eInt("zeroMod", MOD_CONTROL);
  g_keybinds.zeroKey = eInt("zeroKey", 'G');
  g_keybinds.debugMod = eInt("debugMod", MOD_CONTROL);
  g_keybinds.debugKey = eInt("debugKey", '9');

  g_glideThreshold = eFloat("glideThreshold", 0.05f);
  g_freefallThreshold = eFloat("freefallThreshold", 0.20f);
  
  g_hudY = eInt("hudY", 40);

  g_crossThickness = eFloat("crossThickness", 2.0f);
  g_crossColor     = (COLORREF)eFloat("crossColor", (float)RGB(255, 0, 0));
  g_crossOffsetX   = eFloat("crossOffsetX", 0.0f);
  g_crossOffsetY   = eFloat("crossOffsetY", 0.0f);
  g_crossAngle     = eFloat("crossAngle", 0.0f);
  g_crossPulse     = eFloat("crossPulse", 0.0f) > 0.5f;

  size_t pp = content.find("\"lastProfile\":\"");
  if (pp != std::string::npos) {
    size_t end = content.find("\"", pp + 15);
    std::string n = content.substr(pp + 15, end - (pp + 15));
    g_lastLoadedProfileName = std::wstring(n.begin(), n.end());
  }
}

void SaveSettings() {
  std::wstring sp = GetAppStoragePath() + L"settings.json";
  std::ofstream ofs(sp.c_str());
  ofs << "{\n";
  ofs << "  \"toggleMod\": " << g_keybinds.toggleMod << ",\n";
  ofs << "  \"toggleKey\": " << g_keybinds.toggleKey << ",\n";
  ofs << "  \"roiMod\": " << g_keybinds.roiMod << ",\n";
  ofs << "  \"roiKey\": " << g_keybinds.roiKey << ",\n";
  ofs << "  \"crossMod\": " << g_keybinds.crossMod << ",\n";
  ofs << "  \"crossKey\": " << g_keybinds.crossKey << ",\n";
  ofs << "  \"zeroMod\": " << g_keybinds.zeroMod << ",\n";
  ofs << "  \"zeroKey\": " << g_keybinds.zeroKey << ",\n";
  ofs << "  \"debugMod\": " << g_keybinds.debugMod << ",\n";
  ofs << "  \"debugKey\": " << g_keybinds.debugKey << ",\n";
  ofs << "  \"glideThreshold\": " << g_glideThreshold << ",\n";
  ofs << "  \"freefallThreshold\": " << g_freefallThreshold << ",\n";
  ofs << "  \"hudX\": " << g_hudX << ",\n";
  ofs << "  \"hudY\": " << g_hudY << ",\n";
  ofs << "  \"crossThickness\": " << g_crossThickness << ",\n";
  ofs << "  \"crossColor\": " << g_crossColor << ",\n";
  ofs << "  \"crossOffsetX\": " << g_crossOffsetX << ",\n";
  ofs << "  \"crossOffsetY\": " << g_crossOffsetY << ",\n";
  ofs << "  \"crossAngle\": " << g_crossAngle << ",\n";
  ofs << "  \"crossPulse\": " << (g_crossPulse ? 1 : 0) << ",\n";

  std::string lp = "Fallback_Default";
  lp = "";
  for (wchar_t c : g_lastLoadedProfileName)
    lp += (char)c;
  ofs << "  \"lastProfile\":\"" << lp << "\"\n";
  ofs << "}\n";
}
bool g_showCrosshair = false;
float g_crossThickness = 1.0f;
COLORREF g_crossColor = RGB(255, 0, 0);
float g_crossOffsetX = 0.0f;
float g_crossOffsetY = 0.0f;
float g_crossAngle = 0.0f;
bool g_crossPulse = false;

COLORREF g_targetColor = RGB(255, 255, 255);
COLORREF g_pickedColor = RGB(255, 255, 255);
float g_latestVersion = 4.920f; // v4.9.20 format fallback
std::wstring g_latestName = L"Pending Scan";
RECT g_selectionRect = {0, 0, 0, 0};
POINT g_startPoint = {0};

std::string g_latestVersionOnline = "v" VERSION_STR;
float g_currentAngle = 0.0f;
bool g_debugMode = false;
bool g_isCursorVisible = false;
AngleLogic g_logic(0.05);
bool g_forceDiving = false;
bool g_forceDetection = false;

int g_hudX = 40;
int g_hudY = 40;
bool g_isDraggingHUD = false;
POINT g_dragStartHUD = {0, 0};
POINT g_dragStartMouse = {0, 0};
