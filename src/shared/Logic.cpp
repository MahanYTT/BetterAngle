#include "shared/Logic.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <shlobj.h>
#include <vector>

double FetchFortniteSensitivity() {
    // Build path via ExpandEnvironmentStrings (%LOCALAPPDATA%)
    wchar_t expPath[MAX_PATH] = {};
    std::wstring basePath;
    std::vector<std::wstring> potentialPaths;

    if (ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\FortniteGame\\Saved\\Config", expPath, MAX_PATH) && expPath[0] != L'%') {
        basePath = expPath;
    } else {
        // Fallback 1: SHGetFolderPath
        wchar_t appdata[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
            basePath = std::wstring(appdata) + L"\\FortniteGame\\Saved\\Config";
        } else {
            // Fallback 2: Direct path construction from USERPROFILE
            wchar_t userProfile[MAX_PATH] = {};
            if (ExpandEnvironmentStringsW(L"%USERPROFILE%\\AppData\\Local\\FortniteGame\\Saved\\Config", userProfile, MAX_PATH)) {
                basePath = userProfile;
            } else {
                return -1.0;
            }
        }
    }

    std::vector<std::wstring> potentialPaths;
    potentialPaths.push_back(basePath + L"\\WindowClient\\GameUserSettings.ini");
    potentialPaths.push_back(basePath + L"\\WindowsClient\\GameUserSettings.ini");
    potentialPaths.push_back(basePath + L"\\WindowsNoEditor\\GameUserSettings.ini");


    // Dynamic search fallback
    WIN32_FIND_DATAW findData;
    std::wstring searchPattern = basePath + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                    potentialPaths.push_back(basePath + L"\\" + findData.cFileName + L"\\GameUserSettings.ini");
                }
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }


    for (const auto& pPath : potentialPaths) {
        std::ifstream ifs(pPath, std::ios::binary);
        if (!ifs.is_open()) continue;

        ifs.seekg(0, std::ios::end);
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size <= 0 || size > 10 * 1024 * 1024) continue; 

        std::string buffer(static_cast<size_t>(size), '\0');
        if (!ifs.read(&buffer[0], size)) continue;

        // Strip null bytes — handles UTF-16 LE INI files
        buffer.erase(std::remove(buffer.begin(), buffer.end(), '\0'), buffer.end());

        // Search for sensitivity keys in priority order
        const char* keys[] = { "MouseSensitivityX=", "MouseSensitivity=", "MouseX=" };
        for (const char* key : keys) {
            size_t pos = buffer.find(key);
            if (pos != std::string::npos) {
                size_t valStart = pos + strlen(key);
                size_t valEnd   = buffer.find_first_of("\r\n", valStart);
                if (valEnd == std::string::npos) valEnd = buffer.size();

                std::string valStr = buffer.substr(valStart, valEnd - valStart);
                // Trim trailing spaces/CR/LF
                while (!valStr.empty() && (valStr.back() == ' ' || valStr.back() == '\r' || valStr.back() == '\n' || valStr.back() == '\t'))
                    valStr.pop_back();

                try {
                    double val = std::stod(valStr);
                    if (val > 0.0) return val;
                } catch (...) {}
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
