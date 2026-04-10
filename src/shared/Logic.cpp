#include "shared/Logic.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <shlobj.h>

double FetchFortniteSensitivity() {
    double fetchedSens = 0.05;
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        std::wstring pPath = std::wstring(appdata) + L"\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";
        std::ifstream ifs(pPath.c_str());
        if (ifs.is_open() && ifs.good()) {
            std::string line;
            while (std::getline(ifs, line)) {
                if (line.find("MouseSensitivityX=") != std::string::npos) {
                    size_t eqPos = line.find("=");
                    if (eqPos != std::string::npos) {
                        try {
                            fetchedSens = std::stod(line.substr(eqPos + 1));
                        } catch (...) { }
                    }
                    break;
                }
            }
            ifs.close();
        }
    }
    return (std::max)(fetchedSens, 0.0001);
}

AngleLogic::AngleLogic(double sensX) : m_sensX(sensX), m_accumDx(0), m_isDiving(false), m_baseDx(0), m_baseAngle(0.0) {}

void AngleLogic::Update(int dx) {
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    // 360 degrees rotation is approx 1 / sensitivity units? 
    // This is a placeholder for the actual Fortnite scaling logic
    return (m_accumDx.load() * m_sensX.load() * 0.022); 
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
