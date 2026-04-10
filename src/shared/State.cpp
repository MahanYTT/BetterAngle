#include "shared/State.h"
#include "shared/Logic.h"

SelectionState g_currentSelection = NONE;
bool g_isSelectionActive = false;
HBITMAP g_screenSnapshot = NULL;
bool g_isDiving = false;
bool g_showROIBox = true;
int g_currentTab = 0;
float g_detectionRatio = 0.0f;
bool g_isCheckingForUpdates = false;
float g_updateSpinAngle = 0.0f;
bool g_updateAvailable = false;

Keybinds g_keybinds;
std::wstring g_lastLoadedProfileName = L"";

#include <fstream>
#include <string>

void LoadSettings() {
    std::ifstream ifs("profiles/settings.json");
    if (!ifs.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto eInt = [&](std::string k, UINT def) -> UINT {
        size_t p = content.find("\"" + k + "\":");
        if (p == std::string::npos) return def;
        return std::stoi(content.substr(p + k.length() + 1));
    };
    g_keybinds.toggleMod = eInt("toggleMod", MOD_CONTROL);
    g_keybinds.toggleKey = eInt("toggleKey", 'U');
    g_keybinds.roiMod = eInt("roiMod", MOD_CONTROL);
    g_keybinds.roiKey = eInt("roiKey", 'R');
    g_keybinds.crossMod = eInt("crossMod", 0);
    g_keybinds.crossKey = eInt("crossKey", VK_F10);
    g_keybinds.zeroMod = eInt("zeroMod", MOD_CONTROL);
    g_keybinds.zeroKey = eInt("zeroKey", 'G');
    g_keybinds.debugMod = eInt("debugMod", MOD_CONTROL);
    g_keybinds.debugKey = eInt("debugKey", '9');
    
    size_t pp = content.find("\"lastProfile\":\"");
    if (pp != std::string::npos) {
        size_t end = content.find("\"", pp + 15);
        std::string n = content.substr(pp + 15, end - (pp + 15));
        g_lastLoadedProfileName = std::wstring(n.begin(), n.end());
    }
}

void SaveSettings() {
    std::ofstream ofs("profiles/settings.json");
    ofs << "{\n";
    ofs << "  \"toggleMod\": " << g_keybinds.toggleMod << ",\n";
    ofs << "  \"toggleKey\": " << g_keybinds.toggleKey << ",\n";
    ofs << "  \"roiMod\": " << g_keybinds.roiMod << ",\n";
    ofs << "  \"roiKey\": " << g_keybinds.roiKey << ",\n";
    ofs << "  \"crossMod\": " << g_keybinds.crossMod << ",\n";
    ofs << "  \"crossKey\": " << g_keybinds.crossKey << ",\n";
    ofs << "  \"zeroMod\": " << g_keybinds.zeroMod << ",\n";
    ofs << "  \"zeroKey\": " << g_keybinds.zeroKey << ",\n";
    ofs << "  \"debugMod\": " << g_keybinds.debugMod << ",\n";
    ofs << "  \"debugKey\": " << g_keybinds.debugKey << ",\n";
    
    std::string lp = "Fallback_Default";
    // We assume BetterAngle.cpp exposes the list but wait, State.cpp doesn't have g_allProfiles
    // Wait, g_lastLoadedProfileName is synced whenever the user clicks a profile. 
    // We just write g_lastLoadedProfileName directly.
    lp = std::string(g_lastLoadedProfileName.begin(), g_lastLoadedProfileName.end());
    ofs << "  \"lastProfile\":\"" << lp << "\"\n";
    ofs << "}\n";
}
bool g_showCrosshair = false;
COLORREF g_pickedColor = RGB(255, 255, 255);
COLORREF g_targetColor = RGB(255, 255, 255);
float g_latestVersion = 4.920f; // v4.9.20 format fallback
std::wstring g_latestName = L"Pending Scan";
RECT g_selectionRect = { 0, 0, 0, 0 };
POINT g_startPoint = { 0 };


std::string g_status = "Connected (v" VERSION_STR " Pro)";
std::string g_latestVersionOnline = "v" VERSION_STR;
float g_currentAngle = 0.0f;
bool g_debugMode = false;
bool g_isCursorVisible = false;
AngleLogic g_logic(800, 6.5);
bool g_forceDiving = false;
bool g_forceDetection = false;
