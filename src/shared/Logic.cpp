#include "shared/Logic.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <shlobj.h>

double FetchFortniteSensitivity() {
    wchar_t appdata[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata)))
        return -1.0;

    // Fortnite stores settings in WindowsClient subfolder
    std::wstring pPath = std::wstring(appdata) +
        L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";

    std::ifstream ifs(pPath.c_str());
    if (!ifs.is_open() || !ifs.good())
        return -1.0; // File not found

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.find("MouseSensitivityX=") != std::string::npos) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                try {
                    double val = std::stod(line.substr(eqPos + 1));
                    return (std::max)(val, 0.001); // Return actual value
                } catch (...) { }
            }
            break;
        }
    }
    return -1.0; // Key not found in file
}

bool IsFortniteFocused() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    wchar_t cls[256] = { 0 };
    GetClassNameW(fg, cls, 256);
    if (wcscmp(cls, L"UnrealWindow") != 0) return false;
    wchar_t title[256] = { 0 };
    GetWindowTextW(fg, title, 256);
    return wcsstr(title, L"Fortnite") != nullptr;
}

AngleLogic::AngleLogic(double sensX) : m_sensX(sensX), m_accumDx(0), m_isDiving(false), m_baseDx(0), m_baseAngle(0.0) {}

void AngleLogic::Update(int dx) {
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    // 0.05555 deg/tick * sens gives the real Fortnite FOV scale
    double normalScale = 0.05555 * m_sensX.load();
    double scale = m_isDiving.load() ? (normalScale * 1.0916) : normalScale;
    return m_accumDx.load() * scale;
}

void AngleLogic::SetZero() {
    m_accumDx = 0;
}

void AngleLogic::LoadProfile(double sensX) {
    m_sensX = sensX;
}

void AngleLogic::SetDivingState(bool diving) {
    m_isDiving = diving;
}

double AngleLogic::Norm360(double a) const {
    while (a >= 360.0) a -= 360.0;
    while (a < 0.0) a += 360.0;
    return a;
}
