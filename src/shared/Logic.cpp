#include "shared/Logic.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <shlobj.h>

double FetchFortniteSensitivity() {
    wchar_t expPath[MAX_PATH];
    if (!ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini", expPath, MAX_PATH))
        return -1.0;

    std::wstring pPath = expPath;

    std::ifstream ifs(pPath, std::ios::binary);
    if (!ifs.is_open() || !ifs.good())
        return -1.0; 

    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(size);
    if (ifs.read(&buffer[0], size)) {
        // Remove null bytes seamlessly, converting UTF-16LE to standard string for basic ASCII keys
        buffer.erase(std::remove(buffer.begin(), buffer.end(), '\0'), buffer.end());

        // Fortnite often uses MouseSensitivityX, but search for MouseSensitivity as fallback
        const char* keys[] = { "MouseSensitivityX=", "MouseSensitivity=" };
        for (const char* key : keys) {
            size_t pos = buffer.find(key);
            if (pos != std::string::npos) {
                size_t valStart = pos + std::string(key).length();
                size_t valEnd = buffer.find_first_of("\r\n", valStart);
                if (valEnd == std::string::npos) valEnd = buffer.length();

                std::string valStr = buffer.substr(valStart, valEnd - valStart);
                try {
                    double val = std::stod(valStr);
                    return (std::max)(val, 0.0001);
                } catch (...) { }
            }
        }
    }
    return -1.0; 
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

AngleLogic::AngleLogic(double sensX) 
    : m_sensX(sensX), m_isDiving(false), m_accumDx(0), m_baseDx(0), m_baseAngle(0.0) {}

void AngleLogic::Update(int dx) {
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    double currentSens = m_sensX.load();
    // 0.00555555 deg/tick * sens is the true Fortnite pitch/yaw scale
    double scale = 0.00555555 * currentSens;
    if (m_isDiving.load()) {
        scale *= 1.0916; // Diving multiplier
    }

    double delta = (double)(m_accumDx.load() - m_baseDx.load());
    return m_baseAngle.load() + (delta * scale);
}

void AngleLogic::SetZero() {
    m_accumDx = 0;
    m_baseDx = 0;
    m_baseAngle = 0.0;
}

void AngleLogic::LoadProfile(double sensX) {
    // Before updating sensitivity, bake in the current angle to prevent jumping
    m_baseAngle = GetAngle();
    m_baseDx = m_accumDx.load();
    m_sensX = sensX;
}

void AngleLogic::SetDivingState(bool diving) {
    if (diving == m_isDiving.load()) return;

    // Bake in the current angle before switching scales
    m_baseAngle = GetAngle();
    m_baseDx = m_accumDx.load();
    m_isDiving = diving;
}

double AngleLogic::Norm360(double a) const {
    while (a >= 360.0) a -= 360.0;
    while (a < 0.0) a += 360.0;
    return a;
}
