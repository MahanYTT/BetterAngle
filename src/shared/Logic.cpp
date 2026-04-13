#include "shared/Logic.h"
#include "shared/State.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <vector>

void LogStartup(const std::string &msg);

// Case-insensitive string search helper
size_t find_case_insensitive(const std::string &buffer,
                             const std::string &key) {
  auto it = std::search(buffer.begin(), buffer.end(), key.begin(), key.end(),
                        [](char ch1, char ch2) {
                          return std::tolower(ch1) == std::tolower(ch2);
                        });
  return (it != buffer.end()) ? std::distance(buffer.begin(), it)
                              : std::string::npos;
}

double FetchFortniteSensitivity() {
  wchar_t appdata[MAX_PATH] = {};
  if (GetEnvironmentVariableW(L"LOCALAPPDATA", appdata, MAX_PATH) == 0) {
    return -1.0;
  }

  // Directly build the known path
  std::wstring configPath =
      std::wstring(appdata) +
      L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";

  std::error_code ec;
  if (!std::filesystem::exists(configPath, ec) || ec) {
    std::wcerr << L"FORTNITE SYNC: File not found at " << configPath
               << std::endl;
    return -1.0;
  }

  std::ifstream ifs(configPath, std::ios::binary | std::ios::ate);
  if (!ifs.is_open())
    return -1.0;

  std::streamsize size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  if (size <= 0 || size > 10 * 1024 * 1024)
    return -1.0;

  std::string buffer(static_cast<size_t>(size), '\0');
  if (!ifs.read(&buffer[0], size))
    return -1.0;

  // Handle UTF-16 LE
  if (size >= 2 && (unsigned char)buffer[0] == 0xFF &&
      (unsigned char)buffer[1] == 0xFE) {
    std::wstring wbuf((size - 2) / 2, L'\0');
    memcpy(&wbuf[0], buffer.data() + 2, size - 2);
    buffer.clear();
    for (wchar_t c : wbuf) {
      if (c == 0)
        break;
      buffer += (c < 128 ? (char)c : ' ');
    }
  } else {
    buffer.erase(std::remove(buffer.begin(), buffer.end(), '\0'), buffer.end());
  }

  // Narrow search to the correct INI section
  size_t sectionPos = find_case_insensitive(
      buffer, "[/Script/FortniteGame.FortGameUserSettings]");
  std::string searchArea =
      (sectionPos != std::string::npos) ? buffer.substr(sectionPos) : buffer;

  // Priority order for sensitivity keys
  const char *keys[] = {"MouseSensitivity", "MouseSensitivityX", "MouseX"};
  for (const char *key : keys) {
    size_t pos = find_case_insensitive(searchArea, key);
    if (pos != std::string::npos) {
      size_t eqPos = searchArea.find('=', pos);
      if (eqPos != std::string::npos && eqPos < pos + 50) {
        size_t valStart = eqPos + 1;
        size_t valEnd = searchArea.find_first_of("\r\n", valStart);
        if (valEnd == std::string::npos)
          valEnd = searchArea.size();

        std::string valStr = searchArea.substr(valStart, valEnd - valStart);
        valStr.erase(0, valStr.find_first_not_of(" \t\r\n\"'"));
        valStr.erase(valStr.find_last_not_of(" \t\r\n\"'") + 1);

        if (!valStr.empty()) {
          try {
            double val = std::stod(valStr);
            if (val > 0.0)
              return val;
          } catch (...) {
          }
        }
      }
    }
  }

  return -1.0;
}

bool IsFortniteFocused() {
  HWND fg = GetForegroundWindow();
  if (!fg)
    return false;
  wchar_t cls[256] = {0};
  GetClassNameW(fg, cls, 256);
  if (wcscmp(cls, L"UnrealWindow") != 0)
    return false;
  wchar_t title[256] = {0};
  GetWindowTextW(fg, title, 256);
  return wcsstr(title, L"Fortnite") != nullptr;
}

AngleLogic::AngleLogic(double sensX)
    : m_sensX(sensX), m_isDiving(false), m_accumDx(0), m_baseDx(0),
      m_baseAngle(0.0) {}

void AngleLogic::Update(int dx) { m_accumDx += dx; }

double AngleLogic::GetAngle() const {
  double currentSens = m_sensX.load();
  double scale = 0.00555555 * currentSens;
  if (m_isDiving.load()) {
    scale *= 1.0916;
  }
  double delta = (double)(m_accumDx.load() - m_baseDx.load());
  double rawAngle = m_baseAngle.load() + (delta * scale);
  double normalizedAngle = Norm360(rawAngle);

  static bool s_loggedNonFinite = false;
  static double s_lastLoggedRaw = 0.0;
  if (!std::isfinite(rawAngle)) {
    if (!s_loggedNonFinite) {
      LogStartup("AngleNormalize: non-finite raw angle detected; forcing "
                 "diagnostic clamp path.");
      s_loggedNonFinite = true;
    }
  } else if (std::fabs(rawAngle - normalizedAngle) >= 0.01 &&
             std::fabs(rawAngle - s_lastLoggedRaw) >= 1.0) {
    std::ostringstream oss;
    oss << "AngleNormalize: raw=" << rawAngle
        << " normalized=" << normalizedAngle << " deltaDx=" << delta
        << " sensX=" << currentSens
        << " diving=" << (m_isDiving.load() ? "true" : "false");
    LogStartup(oss.str());
    s_lastLoggedRaw = rawAngle;
  }

  return normalizedAngle;
}

void AngleLogic::SetZero() {
  m_accumDx = 0;
  m_baseDx = 0;
  m_baseAngle = 0.0;
}

void AngleLogic::LoadProfile(double sensX) {
  m_baseAngle = GetAngle();
  m_baseDx = m_accumDx.load();
  m_sensX = sensX;
}

void AngleLogic::SetDivingState(bool diving) {
  if (diving == m_isDiving.load())
    return;
  m_baseAngle = GetAngle();
  m_baseDx = m_accumDx.load();
  m_isDiving = diving;
}

double AngleLogic::Norm360(double a) const {
  if (!std::isfinite(a))
    return 0.0;
  while (a >= 360.0)
    a -= 360.0;
  while (a < 0.0)
    a += 360.0;
  return a;
}
