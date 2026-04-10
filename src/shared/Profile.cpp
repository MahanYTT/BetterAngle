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

  dpi = (int)extractDouble("dpi");
  if (dpi == 0) dpi = 800;
  
  sensitivityX = extractDouble("sensitivityX");
  if (sensitivityX <= 0.0) sensitivityX = 0.05;
  
  sensitivityY = extractDouble("sensitivityY");
  if (sensitivityY <= 0.0) sensitivityY = 0.05;
  
  divingScaleMultiplier = extractDouble("divingScaleMultiplier");
  if (divingScaleMultiplier <= 0) divingScaleMultiplier = 1.22;

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

  // Load Crosshair (with defaults for legacy files)
  crossThickness = (float)extractDouble("crossThickness");
  if (crossThickness <= 0) crossThickness = 2.0f;
  crossColor = (COLORREF)extractDouble("crossColor");
  if (crossColor == 0) crossColor = RGB(255, 0, 0); // Default Red
  crossOffsetX = (float)extractDouble("crossOffsetX");
  crossOffsetY = (float)extractDouble("crossOffsetY");
  crossAngle = (float)extractDouble("crossAngle");
  crossPulse = extractDouble("crossPulse") > 0.5;


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
  oss << "  \"dpi\": " << dpi << ",\n";
  oss << "  \"sensitivityX\": " << sensitivityX << ",\n";
  oss << "  \"sensitivityY\": " << sensitivityY << ",\n";
  oss << "  \"divingScaleMultiplier\": " << divingScaleMultiplier << ",\n";
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
  oss << "  \"crossThickness\": " << crossThickness << ",\n";
  oss << "  \"crossColor\": " << crossColor << ",\n";
  oss << "  \"crossOffsetX\": " << crossOffsetX << ",\n";
  oss << "  \"crossOffsetY\": " << crossOffsetY << ",\n";
  oss << "  \"crossAngle\": " << crossAngle << ",\n";
  oss << "  \"crossPulse\": " << (crossPulse ? 1 : 0) << "\n";
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
