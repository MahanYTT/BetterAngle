#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <wininet.h>
#include <urlmon.h>
#include <fstream>
#include <string>
#include <iostream>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")

// Fallback if the build system fails to provide it
#ifndef VERSION_STR
#define VERSION_STR "4.20.1"
#endif
#define APP_VERSION_STR VERSION_STR

// Helper to construct the predictable GitHub download URL
std::wstring GetReleaseDownloadUrl(const std::string& tag) {
    // Format: https://github.com/MahanYTT/BetterAngle/releases/download/v4.20.1/BetterAngle.exe
    std::string url = "https://github.com/MahanYTT/BetterAngle/releases/download/" + tag + "/BetterAngle.exe";
    return std::wstring(url.begin(), url.end());
}

bool CheckForUpdates() {
    g_isCheckingForUpdates = true;

    // GitHub API REQUIRES a valid User-Agent, or it returns 403 Forbidden.
    std::string userAgent = "BetterAngle-Updater/" VERSION_STR;
    HINTERNET hInternet = InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        g_isCheckingForUpdates = false;
        return false;
    }

    // Target the specific GitHub API endpoint for the latest release
    HINTERNET hUrl = InternetOpenUrlA(hInternet,
        "https://api.github.com/repos/MahanYTT/BetterAngle/releases/latest",
        NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);

    if (!hUrl) {
        InternetCloseHandle(hInternet);
        g_isCheckingForUpdates = false;
        return false;
    }

    // Read the JSON response from GitHub
    std::string jsonResponse;
    char buffer[1024];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        jsonResponse += buffer;
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (jsonResponse.empty()) {
        g_isCheckingForUpdates = false;
        return false;
    }

    // Dependency-free extraction of the "tag_name" (e.g., "v4.9.36")
    std::string tagPrefix = "\"tag_name\": \"";
    size_t tagPos = jsonResponse.find(tagPrefix);
    if (tagPos == std::string::npos) {
        g_isCheckingForUpdates = false;
        return false;
    }

    size_t startPos = tagPos + tagPrefix.length();
    size_t endPos = jsonResponse.find("\"", startPos);
    std::string newVersionTag = jsonResponse.substr(startPos, endPos - startPos);

    if (!newVersionTag.empty()) {
        g_latestVersionOnline = newVersionTag; // Usually looks like "v4.9.36"

        // Strip the 'v' for float conversion (e.g., v4.9.36 -> 4.936)
        std::string rawVersion = newVersionTag;
        if (rawVersion[0] == 'v' || rawVersion[0] == 'V') {
            rawVersion = rawVersion.substr(1);
        }

        try {
            // Keep your existing float fallback logic for internal comparisons
            g_latestVersion = std::stof(rawVersion);
        } catch (...) {
            g_latestVersion = 4.920f;
        }

        g_latestName = L"GitHub Release (" + std::wstring(newVersionTag.begin(), newVersionTag.end()) + L")";

        // Compare with local version. Ensure local APP_VERSION_STR matches tag formatting.
        std::string localVersion = std::string(APP_VERSION_STR);
        std::string localVersionWithV = "v" + localVersion;

        if (newVersionTag != localVersion && newVersionTag != localVersionWithV) {
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

// Ensure the UI passes the correct download URL dynamically
bool DownloadUpdate(const std::wstring& url, const std::wstring& dest) {
    // If the UI passes an empty URL or a generic one, construct the exact GitHub Release URL
    std::wstring targetUrl = url;
    if (targetUrl.empty() || targetUrl == L"AUTO") {
        std::string tagStr = g_latestVersionOnline;
        targetUrl = GetReleaseDownloadUrl(tagStr);
    }

    if (URLDownloadToFileW(NULL, targetUrl.c_str(), dest.c_str(), 0, NULL) == S_OK) {
        return true;
    }
    return false;
}

// The fixed, freeze-free batch script
void ApplyUpdateAndRestart() {
    std::ofstream bat("cleanup.bat");

    bat << "@echo off\n";
    bat << "timeout /t 2 /nobreak >nul\n";
    bat << "taskkill /F /IM BetterAngle.exe >nul 2>&1\n";
    bat << "del BetterAngle.exe\n";
    bat << "rename update_tmp.exe BetterAngle.exe\n";
    bat << "start \"\" \"BetterAngle.exe\"\n";
    bat << "del \"%~f0\"\n";

    bat.close();

    ShellExecuteA(NULL, "open", "cleanup.bat", NULL, NULL, SW_HIDE);
    exit(0);
}
