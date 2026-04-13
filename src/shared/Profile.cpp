#include "shared/Profile.h"
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>
#include <windows.h>


bool Profile::Load(const std::wstring &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open())
    return false;

  // Simple manual JSON parser to avoid external dependencies
  std::string content((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());

  // Extract name
  size_t namePos = content.find("\"name\": \"");
  if (namePos != std::string::npos) {
    size_t end = content.find("\"", namePos + 9);
    std::string n = content.substr(namePos + 9, end - (namePos + 9));
    name = std::wstring(n.begin(), n.end());
  }

  // Extract scales
  auto extractDouble = [&](const std::string &key) -> double {
    size_t pos = content.find("\"" + key + "\": ");
    if (pos == std::string::npos)
      return 0.003;
    size_t end = content.find_first_of(",}", pos + 3 + key.length());
    std::string valStr =
        content.substr(pos + 3 + key.length(), end - (pos + 3 + key.length()));
    for (char &c : valStr)
      if (c == ',')
        c = '.'; // Fix existing comma-polluted files
    std::istringstream iss(valStr);
    iss.imbue(std::locale("C"));
    double d = 0.003;
    iss >> d;
    return d;
  };

  auto extractString = [&](const std::string &key) -> std::string {
    size_t pos = content.find("\"" + key + "\": \"");
    if (pos == std::string::npos) return "";
    size_t end = content.find("\"", pos + 4 + key.length());
    return content.substr(pos + 4 + key.length(), end - (pos + 4 + key.length()));
  };

  sensitivityX = extractDouble("sensitivityX");
  if (sensitivityX <= 0.0) sensitivityX = 0.05;
  
  sensitivityY = extractDouble("sensitivityY");
  if (sensitivityY <= 0.0) sensitivityY = 0.05;

  fov = (float)extractDouble("fov");
  resolutionWidth = (int)extractDouble("resolutionWidth");
  resolutionHeight = (int)extractDouble("resolutionHeight");
  renderScale = (float)extractDouble("renderScale");

  roi_x = (int)extractDouble("roi_x");
  roi_y = (int)extractDouble("roi_y");
  roi_w = (int)extractDouble("roi_w");
  roi_h = (int)extractDouble("roi_h");
  target_color = (COLORREF)extractDouble("target_color");
  tolerance = (int)extractDouble("tolerance");

  // Load Keybinds
  keybinds.toggleMod = (UINT)extractDouble("kb_toggleMod");
  keybinds.toggleKey = (UINT)extractDouble("kb_toggleKey");
  keybinds.roiMod    = (UINT)extractDouble("kb_roiMod");
  keybinds.roiKey    = (UINT)extractDouble("kb_roiKey");
  keybinds.crossMod  = (UINT)extractDouble("kb_crossMod");
  keybinds.crossKey  = (UINT)extractDouble("kb_crossKey");
  keybinds.zeroMod   = (UINT)extractDouble("kb_zeroMod");
  keybinds.zeroKey   = (UINT)extractDouble("kb_zeroKey");
  keybinds.debugMod  = (UINT)extractDouble("kb_debugMod");
  keybinds.debugKey  = (UINT)extractDouble("kb_debugKey");

  // Fallback defaults for new files or legacy ones
  if (keybinds.toggleKey == 0) { keybinds.toggleMod = MOD_CONTROL; keybinds.toggleKey = 'U'; }
  if (keybinds.roiKey == 0)    { keybinds.roiMod    = MOD_CONTROL; keybinds.roiKey    = 'R'; }
  if (keybinds.crossKey == 0)  { keybinds.crossMod  = 0;           keybinds.crossKey  = VK_F10; }
  if (keybinds.zeroKey == 0)   { keybinds.zeroMod   = MOD_CONTROL; keybinds.zeroKey   = 'G'; }
  if (keybinds.debugKey == 0)  { keybinds.debugMod  = MOD_CONTROL; keybinds.debugKey  = '9'; }

  // Load Crosshair (with defaults for legacy files)
  crossThickness = (float)extractDouble("crossThickness");
  if (crossThickness <= 0) crossThickness = 2.0f;
  crossColor = (COLORREF)extractDouble("crossColor");
  if (crossColor == 0) crossColor = RGB(255, 0, 0); // Default Red
  crossOffsetX = (float)extractDouble("crossOffsetX");
  crossOffsetY = (float)extractDouble("crossOffsetY");
  crossAngle = (float)extractDouble("crossAngle");
  bool pulseVal = extractDouble("crossPulse") > 0.5;
  crossPulse = pulseVal;
  showCrosshair = extractDouble("showCrosshair") > 0.5;
  if (content.find("\"showCrosshair\"") == std::string::npos) showCrosshair = true; 

  // Load Presets Array (Manual Parser)
  crosshairPresets.clear();
  size_t arrPos = content.find("\"crosshairPresets\": [");
  if (arrPos != std::string::npos) {
    size_t endArr = content.find("]", arrPos);
    std::string arrContent = content.substr(arrPos, endArr - arrPos);
    size_t objPos = 0;
    while ((objPos = arrContent.find("{", objPos)) != std::string::npos) {
        size_t objEnd = arrContent.find("}", objPos);
        if (objEnd == std::string::npos) break;
        std::string obj = arrContent.substr(objPos, objEnd - objPos);
        
        CrosshairPreset cp;
        // Parse name
        size_t nP = obj.find("\"name\": \"");
        if (nP != std::string::npos) {
            size_t nE = obj.find("\"", nP + 9);
            std::string nStr = obj.substr(nP + 9, nE - (nP + 9));
            cp.name = std::wstring(nStr.begin(), nStr.end());
        }
        // Parse coords
        auto exD = [&](std::string k) -> float {
            size_t p = obj.find("\"" + k + "\": ");
            if (p == std::string::npos) return 0.0f;
            return (float)std::atof(obj.substr(p + k.length() + 3).c_str());
        };
        cp.offsetX = exD("x");
        cp.offsetY = exD("y");
        cp.angle   = exD("a");
        crosshairPresets.push_back(cp);
        objPos = objEnd + 1;
    }
  }

  // Ensure default if empty
  if (crosshairPresets.empty()) {
    CrosshairPreset def = { L"🎯 Screen Center", 0.0f, 0.0f, 0.0f };
    crosshairPresets.push_back(def);
  }

  return true;
}

bool Profile::Save(const std::wstring &path) {
  std::wstring tempPath = path + L".tmp";
  
  // Ensure file is not hidden before writing to avoid permission issues
  SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);

  std::wofstream f(tempPath.c_str(), std::ios::trunc);
  if (!f.is_open())
    return false;

  std::string nStr;
  for (wchar_t c : name)
    nStr += (char)c;
  
  // Ensure we write with a safe dot decimal regardless of locale
  std::wostringstream oss;
  oss.imbue(std::locale("C"));

  oss << L"{\n";
  oss << L"  \"name\": \"" << name << L"\",\n";
  oss << L"  \"sensitivityX\": " << sensitivityX << L",\n";
  oss << L"  \"sensitivityY\": " << sensitivityY << L",\n";
  oss << L"  \"fov\": " << fov << L",\n";
  oss << L"  \"resolutionWidth\": " << resolutionWidth << L",\n";
  oss << L"  \"resolutionHeight\": " << resolutionHeight << L",\n";
  oss << L"  \"renderScale\": " << renderScale << L",\n";
  oss << L"  \"roi_x\": " << roi_x << L",\n";
  oss << L"  \"roi_y\": " << roi_y << L",\n";
  oss << L"  \"roi_w\": " << roi_w << L",\n";
  oss << L"  \"roi_h\": " << roi_h << L",\n";
  oss << L"  \"target_color\": " << (float)target_color << L",\n";
  oss << L"  \"tolerance\": " << tolerance << L",\n";
  oss << L"  \"kb_toggleMod\": " << keybinds.toggleMod << L",\n";
  oss << L"  \"kb_toggleKey\": " << keybinds.toggleKey << L",\n";
  oss << L"  \"kb_roiMod\": " << keybinds.roiMod << L",\n";
  oss << L"  \"kb_roiKey\": " << keybinds.roiKey << L",\n";
  oss << L"  \"kb_crossMod\": " << keybinds.crossMod << L",\n";
  oss << L"  \"kb_crossKey\": " << keybinds.crossKey << L",\n";
  oss << L"  \"kb_zeroMod\": " << keybinds.zeroMod << L",\n";
  oss << L"  \"kb_zeroKey\": " << keybinds.zeroKey << L",\n";
  oss << L"  \"kb_debugMod\": " << keybinds.debugMod << L",\n";
  oss << L"  \"kb_debugKey\": " << keybinds.debugKey << L",\n";
  oss << L"  \"crossThickness\": " << crossThickness << L",\n";
  oss << L"  \"showCrosshair\": " << (showCrosshair ? 1 : 0) << L",\n";
  oss << L"  \"crossColor\": " << (float)crossColor << L",\n";
  oss << L"  \"crossOffsetX\": " << crossOffsetX << L",\n";
  oss << L"  \"crossOffsetY\": " << crossOffsetY << L",\n";
  oss << L"  \"crossAngle\": " << crossAngle << L",\n";
  oss << L"  \"crossPulse\": " << (crossPulse ? 1 : 0) << L",\n";
  
  oss << L"  \"crosshairPresets\": [\n";
  for (size_t i = 0; i < crosshairPresets.size(); i++) {
    const auto& cp = crosshairPresets[i];
    oss << L"    {\"name\": \"" << cp.name << L"\", \"x\": " << cp.offsetX << L", \"y\": " << cp.offsetY << L", \"a\": " << cp.angle << L"}";
    if (i < crosshairPresets.size() - 1) oss << L",";
    oss << L"\n";
  }
  oss << L"  ]\n";
  oss << L"}";

  f << oss.str();
  f.close();
  
  // Atomic swap
  DeleteFileW(path.c_str());
  MoveFileW(tempPath.c_str(), path.c_str());

  SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
  return true;
}

std::vector<Profile> GetProfiles(const std::wstring &directory) {
  std::vector<Profile> profiles;
  WIN32_FIND_DATAW findData;
  std::wstring searchPath = directory + L"/*.json";
  HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        Profile p;
        if (p.Load(directory + findData.cFileName)) {
          profiles.push_back(p);
        }
      }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
  }

  return profiles;
}
