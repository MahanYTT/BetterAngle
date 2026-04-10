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

AngleLogic::AngleLogic(double sensitivity) : sensitivity(sensitivity), accumulatedDeltaX(0) {}

void AngleLogic::Update(int deltaX) {
    accumulatedDeltaX += deltaX;
}

double AngleLogic::GetCurrentAngle() {
    // 360 degrees rotation is approx 1 / sensitivity units? 
    // This is a placeholder for the actual Fortnite scaling logic
    return (accumulatedDeltaX * sensitivity * 0.022); 
}

void AngleLogic::SetZero() {
    accumulatedDeltaX = 0;
}
