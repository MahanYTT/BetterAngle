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
  std::ofstream f(path, std::ios::trunc);
  if (!f.is_open())
    return false;

  std::string n;
  for (wchar_t c : name)
    n += (char)c;
  // Ensure we write with a safe dot decimal regardless of locale
  std::ostringstream oss;
  oss.imbue(std::locale("C"));
  oss << "{\n";
  oss << "  \"name\": \"" << n << "\",\n";
  oss << "  \"sensitivityX\": " << sensitivityX << ",\n";
  oss << "  \"sensitivityY\": " << sensitivityY << ",\n";
  oss << "  \"fov\": " << fov << ",\n";
  oss << "  \"resolutionWidth\": " << resolutionWidth << ",\n";
  oss << "  \"resolutionHeight\": " << resolutionHeight << ",\n";
  oss << "  \"renderScale\": " << renderScale << ",\n";
  oss << "  \"roi_x\": " << roi_x << ",\n";
  oss << "  \"roi_y\": " << roi_y << ",\n";
  oss << "  \"roi_w\": " << roi_w << ",\n";
  oss << "  \"roi_h\": " << roi_h << ",\n";
  oss << "  \"target_color\": " << target_color << ",\n";
  oss << "  \"tolerance\": " << tolerance << ",\n";
  oss << "  \"kb_toggleMod\": " << keybinds.toggleMod << ",\n";
  oss << "  \"kb_toggleKey\": " << keybinds.toggleKey << ",\n";
  oss << "  \"kb_roiMod\": " << keybinds.roiMod << ",\n";
  oss << "  \"kb_roiKey\": " << keybinds.roiKey << ",\n";
  oss << "  \"kb_crossMod\": " << keybinds.crossMod << ",\n";
  oss << "  \"kb_crossKey\": " << keybinds.crossKey << ",\n";
  oss << "  \"kb_zeroMod\": " << keybinds.zeroMod << ",\n";
  oss << "  \"kb_zeroKey\": " << keybinds.zeroKey << ",\n";
  oss << "  \"kb_debugMod\": " << keybinds.debugMod << ",\n";
  oss << "  \"kb_debugKey\": " << keybinds.debugKey << ",\n";
  oss << "  \"crossThickness\": " << crossThickness << ",\n";
  oss << "  \"crossColor\": " << crossColor << ",\n";
  oss << "  \"crossOffsetX\": " << crossOffsetX << ",\n";
  oss << "  \"crossOffsetY\": " << crossOffsetY << ",\n";
  oss << "  \"crossAngle\": " << crossAngle << ",\n";
  oss << "  \"crossPulse\": " << (crossPulse ? 1 : 0) << ",\n";
  
  oss << "  \"crosshairPresets\": [\n";
  for (size_t i = 0; i < crosshairPresets.size(); i++) {
    const auto& cp = crosshairPresets[i];
    std::string n; for (wchar_t c : cp.name) n += (char)c;
    oss << "    {\"name\": \"" << n << "\", \"x\": " << cp.offsetX << ", \"y\": " << cp.offsetY << ", \"a\": " << cp.angle << "}";
    if (i < crosshairPresets.size() - 1) oss << ",";
    oss << "\n";
  }
  oss << "  ]\n";
  oss << "}";

  f << oss.str();

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
        if (p.Load(directory + L"/" + findData.cFileName)) {
          profiles.push_back(p);
        }
      }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
  }

  return profiles;
}
