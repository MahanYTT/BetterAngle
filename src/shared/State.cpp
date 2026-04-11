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
bool g_isDownloadingUpdate = false;
bool g_downloadComplete    = false;
std::string g_updateHistory = "";
std::atomic<bool> g_fortniteFocusedCache(false);
bool g_setupComplete = false;
std::string g_lastVersionRun = "";


Profile g_currentProfile;
std::vector<Profile> g_allProfiles;
int g_selectedProfileIdx = 0;

// Global g_keybinds removed (v4.20.37)
std::wstring g_lastLoadedProfileName = L"";
float g_glideThreshold = 0.05f;
float g_freefallThreshold = 0.20f;

#include <fstream>
#include <string>
#include <locale>
#include <sstream>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

std::wstring GetAppRootPath() {
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        std::wstring path = std::wstring(appdata) + L"\\BetterAngle";
        CreateDirectoryW(path.c_str(), NULL);
        return path + L"\\";
    }
    return L"";
}

std::wstring GetProfilesPath() {
    std::wstring root = GetAppRootPath();
    if (root.empty()) return L"";
    std::wstring pPath = root + L"profiles";
    CreateDirectoryW(pPath.c_str(), NULL);
    return pPath + L"\\";
}


// Legacy Registry functions removed in favor of unified hidden JSON storage.


void LoadSettings() {
  std::wstring sp = GetAppRootPath() + L"settings.json";
  std::ifstream ifs(sp.c_str());
  if (ifs.is_open()) {
    std::string content;
    ifs.seekg(0, std::ios::end);
    content.reserve((size_t)ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    
    auto eFloat = [&](std::string k, float def) -> float {
      size_t p = content.find("\"" + k + "\":");
      if (p == std::string::npos) return def;
      size_t valStart = content.find_first_not_of(" \t\n\r", p + k.length() + 2);
      if (valStart == std::string::npos) return def;
      try { 
        std::istringstream iss(content.substr(valStart));
        iss.imbue(std::locale("C"));
        float v; iss >> v; return v;
      } catch (...) { return def; }
    };
    auto eInt = [&](std::string k, int def) -> int {
      size_t p = content.find("\"" + k + "\":");
      if (p == std::string::npos) return def;
      size_t valStart = content.find_first_not_of(" \t\n\r", p + k.length() + 2);
      if (valStart == std::string::npos) return def;
      try { return std::stoi(content.substr(valStart)); } catch (...) { return def; }
    };

    g_glideThreshold = eFloat("glideThreshold", 0.05f);
    g_freefallThreshold = eFloat("freefallThreshold", 0.20f);
    g_hudX = eInt("hudX", 40);
    g_hudY = eInt("hudY", 40);
    g_crossThickness = eFloat("crossThickness", 2.0f);
    g_crossColor     = (COLORREF)eFloat("crossColor", (float)RGB(255, 0, 0));
    g_crossOffsetX   = eFloat("crossOffsetX", 0.0f);
    g_crossOffsetY   = eFloat("crossOffsetY", 0.0f);
    g_crossAngle     = eFloat("crossAngle", 0.0f);
    g_crossPulse     = eFloat("crossPulse", 0.0f) > 0.5f;
    g_setupComplete  = eFloat("setupComplete", 0.0f) > 0.5f;
    g_showCrosshair  = eFloat("showCrosshair", 1.0f) > 0.5f;
    g_debugMode      = eFloat("debugMode", 0.0f) > 0.5f;
    g_forceDiving    = eFloat("forceDiving", 0.0f) > 0.5f;
    g_forceDetection = eFloat("forceDetection", 0.0f) > 0.5f;
    g_selectedProfileIdx = eInt("selectedProfileIdx", 0);

    size_t vp = content.find("\"lastVersionRun\":\"");
    if (vp != std::string::npos) {
      size_t valS = vp + 18;
      size_t end = content.find("\"", valS);
      if (end != std::string::npos) g_lastVersionRun = content.substr(valS, end - valS);
    }

    size_t pp = content.find("\"lastProfile\":\"");
    if (pp != std::string::npos) {
      size_t valS = pp + 15;
      size_t end = content.find("\"", valS);
      if (end != std::string::npos) {
          std::string n = content.substr(valS, end - valS);
          g_lastLoadedProfileName = std::wstring(n.begin(), n.end());
      }
    }
  } else {
    // Migration: Check if it exists in the OLD path (profiles/settings.json)
    std::wstring oldPath = GetProfilesPath() + L"settings.json";
    if (GetFileAttributesW(oldPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        MoveFileW(oldPath.c_str(), sp.c_str());
        LoadSettings(); 
        return;
    }
  }
}


void SaveSettings() {
  std::wstring sp = GetAppRootPath() + L"settings.json";
  
  // Ensure file is not hidden before writing to avoid permission issues with some stream implementations
  SetFileAttributesW(sp.c_str(), FILE_ATTRIBUTE_NORMAL);

  std::wofstream ofs(sp.c_str(), std::ios::trunc);
  if (!ofs.is_open()) return;

  std::wostringstream oss;
  oss.imbue(std::locale("C"));

  oss << L"{\n";
  oss << L"  \"glideThreshold\": " << g_glideThreshold << L",\n";
  oss << L"  \"freefallThreshold\": " << g_freefallThreshold << L",\n";
  oss << L"  \"hudX\": " << g_hudX << L",\n";
  oss << L"  \"hudY\": " << g_hudY << L",\n";
  oss << L"  \"crossThickness\": " << g_crossThickness << L",\n";
  oss << L"  \"crossColor\": " << (float)g_crossColor << L",\n";
  oss << L"  \"crossOffsetX\": " << g_crossOffsetX << L",\n";
  oss << L"  \"crossOffsetY\": " << g_crossOffsetY << L",\n";
  oss << L"  \"crossAngle\": " << g_crossAngle << L",\n";
  oss << L"  \"crossPulse\": " << (g_crossPulse ? 1 : 0) << L",\n";
  oss << L"  \"setupComplete\": " << (g_setupComplete ? 1 : 0) << L",\n";
  oss << L"  \"showCrosshair\": " << (g_showCrosshair ? 1 : 0) << L",\n";
  oss << L"  \"debugMode\": " << (g_debugMode ? 1 : 0) << L",\n";
  oss << L"  \"forceDiving\": " << (g_forceDiving ? 1 : 0) << L",\n";
  oss << L"  \"forceDetection\": " << (g_forceDetection ? 1 : 0) << L",\n";
  oss << L"  \"selectedProfileIdx\": " << g_selectedProfileIdx << L",\n";
  
  std::string vs = VERSION_STR;
  std::wstring wvs(vs.begin(), vs.end());
  oss << L"  \"lastVersionRun\":\"" << wvs << L"\",\n";

  std::wstring lp = g_lastLoadedProfileName;
  oss << L"  \"lastProfile\":\"" << lp << L"\"\n";
  oss << L"}\n";
  
  ofs << oss.str();
  ofs.close();
  
  SetFileAttributesW(sp.c_str(), FILE_ATTRIBUTE_HIDDEN);
}


bool g_showCrosshair = false;
float g_crossThickness = 2.0f;
COLORREF g_crossColor = RGB(255, 0, 0);
float g_crossOffsetX = 0.0f;
float g_crossOffsetY = 0.0f;
float g_crossAngle = 0.0f;
bool g_crossPulse = false;

COLORREF g_targetColor = RGB(255, 255, 255);
COLORREF g_pickedColor = RGB(255, 255, 255);
float g_latestVersion = 4.920f; 
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
HWND g_hHUD = NULL;
HWND g_hPanel = NULL;
