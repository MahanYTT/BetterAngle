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

#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

std::wstring GetAppRootPath() {
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        std::wstring path = std::wstring(appdata) + L"\\BetterAngle";
        CreateDirectoryW(path.c_str(), NULL);
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN); // Keep it clean as requested
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


void LoadFromRegistry() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BetterAngle", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        auto LoadDouble = [&](const wchar_t* name, double& val) {
            wchar_t buf[64];
            DWORD size = sizeof(buf);
            if (RegQueryValueExW(hKey, name, NULL, NULL, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
                try { val = std::stod(buf); } catch (...) {}
            }
        };
        auto LoadInt = [&](const wchar_t* name, int& val) {
            DWORD v;
            DWORD size = sizeof(v);
            if (RegQueryValueExW(hKey, name, NULL, NULL, (LPBYTE)&v, &size) == ERROR_SUCCESS) {
                val = (int)v;
            }
        };
        auto LoadDWORD = [&](const wchar_t* name, DWORD& val) {
            DWORD v;
            DWORD size = sizeof(v);
            if (RegQueryValueExW(hKey, name, NULL, NULL, (LPBYTE)&v, &size) == ERROR_SUCCESS) {
                val = v;
            }
        };

        if (!g_allProfiles.empty()) {
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            LoadDouble(L"SensitivityX", p.sensitivityX);
            LoadDouble(L"SensitivityY", p.sensitivityY);
            LoadInt(L"ROI_X", p.roi_x);
            LoadInt(L"ROI_Y", p.roi_y);
            LoadInt(L"ROI_W", p.roi_w);
            LoadInt(L"ROI_H", p.roi_h);
            DWORD color;
            DWORD colorSize = sizeof(color);
            if (RegQueryValueExW(hKey, L"TargetColor", NULL, NULL, (LPBYTE)&color, &colorSize) == ERROR_SUCCESS) {
                p.target_color = (COLORREF)color;
                g_targetColor = p.target_color;
            }
        }
        RegCloseKey(hKey);
    }
}

void SaveToRegistry() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\BetterAngle", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        auto SaveDouble = [&](const wchar_t* name, double val) {
            std::wstring s = std::to_wstring(val);
            RegSetValueExW(hKey, name, 0, REG_SZ, (LPBYTE)s.c_str(), (DWORD)((s.length() + 1) * sizeof(wchar_t)));
        };
        auto SaveInt = [&](const wchar_t* name, int val) {
            DWORD v = (DWORD)val;
            RegSetValueExW(hKey, name, 0, REG_DWORD, (LPBYTE)&v, sizeof(v));
        };

        if (!g_allProfiles.empty()) {
            Profile& p = g_allProfiles[g_selectedProfileIdx];
            SaveDouble(L"SensitivityX", p.sensitivityX);
            SaveDouble(L"SensitivityY", p.sensitivityY);
            SaveInt(L"ROI_X", p.roi_x);
            SaveInt(L"ROI_Y", p.roi_y);
            SaveInt(L"ROI_W", p.roi_w);
            SaveInt(L"ROI_H", p.roi_h);
            SaveInt(L"TargetColor", (int)p.target_color);
        }
        RegCloseKey(hKey);
    }
}

void LoadSettings() {
  std::wstring sp = GetAppStoragePath() + L"settings.json";
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
        return std::stof(content.substr(valStart));
      } catch (...) {
        return def;
      }
    };
    auto eInt = [&](std::string k, UINT def) -> UINT {
      size_t p = content.find("\"" + k + "\":");
      if (p == std::string::npos) return def;
      size_t valStart = content.find_first_not_of(" \t\n\r", p + k.length() + 2);
      if (valStart == std::string::npos) return def;
      try {
        return (UINT)std::stoi(content.substr(valStart));
      } catch (...) {
        return def;
      }
    };

    
    // Global Keybinds loading removed (v4.20.37)
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

    size_t vp = content.find("\"lastVersionRun\":\"");
    if (vp != std::string::npos) {
      size_t valS = vp + 18; // offset of "lastVersionRun":"
      size_t end = content.find("\"", valS);
      if (end != std::string::npos) {
          g_lastVersionRun = content.substr(valS, end - valS);
      }
    }

    size_t pp = content.find("\"lastProfile\":\"");
    if (pp != std::string::npos) {
      size_t valS = pp + 15; // offset of "lastProfile":"
      size_t end = content.find("\"", valS);
      if (end != std::string::npos) {
          std::string n = content.substr(valS, end - valS);
          g_lastLoadedProfileName = std::wstring(n.begin(), n.end());
      }
    }
  } else {
    // Migration: Check if it exists in the OLD path (profiles/settings.json)
    std::wstring oldPath = GetProfilesPath() + L"settings.json";
    std::ifstream oldIfs(oldPath.c_str());
    if (oldIfs.is_open()) {
        oldIfs.close();
        MoveFileW(oldPath.c_str(), sp.c_str());
        LoadSettings(); // Recursive call once to lead moved file
        return;
    }
  }

  
  LoadFromRegistry();
}

void SaveSettings() {
  std::wstring sp = GetAppRootPath() + L"settings.json";
  std::ofstream ofs(sp.c_str());

  ofs << "{\n";
  // Global Keybinds saving removed (v4.20.37)
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
  ofs << "  \"setupComplete\": " << (g_setupComplete ? 1 : 0) << ",\n";
  ofs << "  \"showCrosshair\": " << (g_showCrosshair ? 1 : 0) << ",\n";
  ofs << "  \"lastVersionRun\":\"" << VERSION_STR << "\",\n";


  std::string lp = "";
  for (wchar_t c : g_lastLoadedProfileName)
    lp += (char)c;
  ofs << "  \"lastProfile\":\"" << lp << "\"\n";
  ofs << "}\n";
  
  SaveToRegistry();
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
