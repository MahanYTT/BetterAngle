#include "shared/Logic.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <shlobj.h>
#include <vector>

void FindGameUserSettingsRecursive(const std::wstring& directory, std::vector<std::wstring>& outPaths) {
    if (outPaths.size() > 20) return; // Limit search to prevent hangs

    WIN32_FIND_DATAW findData;
    std::wstring searchPattern = directory + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

        std::wstring fullPath = directory + L"\\" + findData.cFileName;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            FindGameUserSettingsRecursive(fullPath, outPaths);
        } else {
            if (_wcsicmp(findData.cFileName, L"GameUserSettings.ini") == 0) {
                outPaths.push_back(fullPath);
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

// Case-insensitive string search helper
size_t find_case_insensitive(const std::string& buffer, const std::string& key) {
    auto it = std::search(
        buffer.begin(), buffer.end(),
        key.begin(), key.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return (it != buffer.end()) ? std::distance(buffer.begin(), it) : std::string::npos;
}

double FetchFortniteSensitivity() {
    wchar_t expPath[MAX_PATH] = {};
    std::wstring basePath;
    std::vector<std::wstring> potentialPaths;

    // Search from 'Saved' instead of just 'Config' to be MORE aggressive (covers all edge cases)
    if (ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\FortniteGame\\Saved", expPath, MAX_PATH) && expPath[0] != L'%') {
        basePath = expPath;
    } else {
        wchar_t appdata[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
            basePath = std::wstring(appdata) + L"\\FortniteGame\\Saved";
        } else {
            return -1.0;
        }
    }

    // Crawl subdirectories looking for GameUserSettings.ini
    FindGameUserSettingsRecursive(basePath, potentialPaths);

    for (const auto& pPath : potentialPaths) {
        std::ifstream ifs(pPath, std::ios::binary);
        if (!ifs.is_open()) continue;

        ifs.seekg(0, std::ios::end);
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size <= 0 || size > 10 * 1024 * 1024) continue; 

        std::string buffer(static_cast<size_t>(size), '\0');
        if (!ifs.read(&buffer[0], size)) continue;

        // Handle UTF-16 LE
        if (size >= 2 && (unsigned char)buffer[0] == 0xFF && (unsigned char)buffer[1] == 0xFE) {
            std::wstring wbuf((size - 2) / 2, L'\0');
            memcpy(&wbuf[0], buffer.data() + 2, size - 2);
            buffer.clear();
            for (wchar_t c : wbuf) buffer += (c < 128 ? (char)c : '?');
        } else {
            buffer.erase(std::remove(buffer.begin(), buffer.end(), '\0'), buffer.end());
        }

        // Search for sensitivity keys (CASE-INSENSITIVE)
        const char* keys[] = { "MouseSensitivityX", "MouseSensitivity", "MouseX" };
        for (const char* key : keys) {
            size_t pos = find_case_insensitive(buffer, key);
            if (pos != std::string::npos) {
                size_t eqPos = buffer.find('=', pos);
                if (eqPos != std::string::npos && eqPos < pos + 50) {
                    size_t valStart = eqPos + 1;
                    size_t valEnd   = buffer.find_first_of("\r\n", valStart);
                    if (valEnd == std::string::npos) valEnd = buffer.size();

                    std::string valStr = buffer.substr(valStart, valEnd - valStart);
                    valStr.erase(0, valStr.find_first_not_of(" \t\r\n\"'"));
                    valStr.erase(valStr.find_last_not_of(" \t\r\n\"'") + 1);

                    if (!valStr.empty()) {
                        try {
                            double val = std::stod(valStr);
                            if (val > 0.0) return val;
                        } catch (...) {}
                    }
                }
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
