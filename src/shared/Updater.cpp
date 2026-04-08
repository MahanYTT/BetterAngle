#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <wininet.h>
#include <urlmon.h>
#include <fstream>
#include <string>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")

bool CheckForUpdates() {
    g_isCheckingForUpdates = true;
    HINTERNET hInternet = InternetOpenA("BetterAngle/4.9.24", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
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
        
        if (g_latestVersionOnline != "v4.9.24") {
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
    std::wofstream bat(L"cleanup.bat");
    bat << L"@echo off\n";
    bat << L":loop\n";
    bat << L"taskkill /F /IM BetterAngle.exe >nul 2>&1\n";
    bat << L"if %errorlevel%==0 goto loop\n";
    bat << L"del BetterAngle.exe\n";
    bat << L"rename update_tmp.exe BetterAngle.exe\n";
    bat << L"start BetterAngle.exe\n";
    bat << L"del \"%~f0\"\n";
    bat.close();

    ShellExecuteW(NULL, L"open", L"cleanup.bat", NULL, NULL, SW_HIDE);
    exit(0);
}
