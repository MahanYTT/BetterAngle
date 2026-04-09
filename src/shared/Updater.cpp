#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <wininet.h>
#include <urlmon.h>
#include <fstream>
#include <string>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")

#ifndef VERSION_STR
#define VERSION_STR TO_STRING(APP_VERSION)
#define VERSION_WSTR TO_WSTRING(APP_VERSION)
#endif
#ifndef APP_VERSION_STR
#define APP_VERSION_STR "4.9.36"
#endif

bool CheckForUpdates() {
    g_isCheckingForUpdates = true;
    std::string userAgent = "BetterAngle/" APP_VERSION_STR;
    HINTERNET hInternet = InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, "https://raw.githubusercontent.com/MahanYTT/BetterAngle/main/VERSION", NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[128];
    DWORD bytesRead = 0;
    std::string newVersion;
    if (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        newVersion = buffer;
        // Trim any trailing whitespace/newlines
        size_t last = newVersion.find_last_not_of(" \n\r\t");
        if (last != std::string::npos) newVersion = newVersion.substr(0, last + 1);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (!newVersion.empty()) {
        g_latestVersionOnline = "v" + newVersion;
        
        try {
            g_latestVersion = std::stof(newVersion);
        } catch (...) {
            g_latestVersion = 4.920f;
        }
        
        g_latestName = L"GitHub Main Branch (v" + std::wstring(newVersion.begin(), newVersion.end()) + L")";
        
        if (g_latestVersionOnline != ("v" APP_VERSION_STR)) {
            g_updateAvailable = true;
        } else {
            g_updateAvailable = false;
        }
        
        g_isCheckingForUpdates = false;
        return g_updateAvailable;
    }

    g_isCheckingForUpdates = false;
    return false;
}

bool DownloadUpdate(const std::wstring& url, const std::wstring& dest) {
    if (URLDownloadToFileW(NULL, url.c_str(), dest.c_str(), 0, NULL) == S_OK) {
        return true;
    }
    return false;
}

void ApplyUpdateAndRestart() {
    // FIX 1: Use standard ofstream (ASCII) so cmd.exe parses it safely
    std::ofstream bat("cleanup.bat");

    bat << "@echo off\n";

    // FIX 2: Wait 2 seconds for BetterAngle.exe to fully close and release file locks
    bat << "timeout /t 2 /nobreak >nul\n";

    // Force kill just in case, but DO NOT loop infinitely
    bat << "taskkill /F /IM BetterAngle.exe >nul 2>&1\n";

    // Swap the files
    bat << "del BetterAngle.exe\n";
    bat << "rename update_tmp.exe BetterAngle.exe\n";

    // FIX 3: Start securely with empty title quotes to prevent path issues
    bat << "start \"\" \"BetterAngle.exe\"\n";

    // Self-destruct the batch file
    bat << "del \"%~f0\"\n";

    bat.close();

    // Execute the new ASCII batch script silently
    ShellExecuteA(NULL, "open", "cleanup.bat", NULL, NULL, SW_HIDE);

    // Exit the current instance immediately so the batch file can delete it
    exit(0);
}