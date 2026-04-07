#include "shared/Updater.h"
#include "shared/Remote.h"
#include <iostream>
#include <string>

// Version Tag for v4.0.0
const std::string CURRENT_VERSION = "v4.0.0";

bool CheckForUpdates() {
    std::string latest = FetchRemoteString(L"https://raw.githubusercontent.com/MahanYTT/BetterAngle/main/VERSION");
    if (latest.empty()) return false;
    
    // Clean whitespace
    latest.erase(latest.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (latest != CURRENT_VERSION) {
        return true; 
    }
    return false;
}

void StartUpdate() {
    // Open the browser to the latest release for now, 
    // real auto-update will use a background download + swap script in next phase.
    ShellExecute(NULL, L"open", L"https://github.com/MahanYTT/BetterAngle/releases/latest", NULL, NULL, SW_SHOWNORMAL);
}
