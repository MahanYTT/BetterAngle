#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <thread>

#pragma comment(lib, "wininet.lib")

// Placeholders for actual URLs — user should replace these in GitHub Actions or similar if needed
// GitHub API for latest release info
const wchar_t* VERSION_URL = L"https://api.github.com/repos/MahanYTT/BetterAngle/releases/latest";
const wchar_t* DOWNLOAD_URL = L"https://github.com/MahanYTT/BetterAngle/releases/latest/download/BetterAngle.exe";

bool DownloadFile(const std::wstring& url, const std::wstring& dest) {
    HINTERNET hInternet = InternetOpenW(L"BetterAngleUpdater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), L"User-Agent: BetterAngleUpdater\r\n", -1L, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream ofs(dest, std::ios::binary);
    if (!ofs.is_open()) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        ofs.write(buffer, bytesRead);
    }

    ofs.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return true;
}

bool CheckForUpdates() {
    g_isCheckingForUpdates = true;
    
    std::wstring tempRes = GetAppRootPath() + L"latest_release.json";
    if (DownloadFile(VERSION_URL, tempRes)) {
        std::ifstream ifs(tempRes);
        std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        
        // Simple but robust JSON parsing for "tag_name": "vX.Y.Z"
        size_t tagPos = json.find("\"tag_name\":");
        if (tagPos != std::string::npos) {
            size_t start = json.find("\"", tagPos + 11);
            if (start != std::string::npos) {
                size_t end = json.find("\"", start + 1);
                if (end != std::string::npos) {
                    std::string latestVerStr = json.substr(start + 1, end - start - 1);
                    
                    // Normalize (strip 'v' prefix if present)
                    std::string normLatest = latestVerStr;
                    if (!normLatest.empty() && (normLatest[0] == 'v' || normLatest[0] == 'V')) {
                        normLatest = normLatest.substr(1);
                    }

                    g_latestVersionOnline = latestVerStr;
                    
                    // Strip 'v' from current version for comparison if it exists
                    std::string currentVer = VERSION_STR;
                    if (!currentVer.empty() && (currentVer[0] == 'v' || currentVer[0] == 'V')) {
                        currentVer = currentVer.substr(1);
                    }

                    if (!normLatest.empty() && normLatest != currentVer) {
                        g_updateAvailable = true;
                        g_updateHistory = "Latest: " + latestVerStr + " (Current: " + VERSION_STR + ")";
                    } else {
                        g_updateAvailable = false;
                    }
                }
            }
        }
        DeleteFileW(tempRes.c_str());
    }

    g_isCheckingForUpdates = false;
    g_hasCheckedForUpdates = true;
    return g_updateAvailable;
}

void UpdateApp() {
    if (g_isDownloadingUpdate || g_downloadComplete) return;

    g_isDownloadingUpdate = true;
    std::thread([]() {
        std::wstring dest = GetAppRootPath() + L"update_tmp.exe";
        if (DownloadFile(DOWNLOAD_URL, dest)) {
            g_downloadComplete = true;
        }
        g_isDownloadingUpdate = false;
    }).detach();
}

void CleanupUpdateJunk() {
    std::wstring root = GetAppRootPath();
    DeleteFileW((root + L"ba_update.bat").c_str());
    DeleteFileW((root + L"update_tmp.exe").c_str());
    DeleteFileW((root + L"latest_version.txt").c_str());
}

void ApplyUpdateAndRestart() {
    std::wstring root = GetAppRootPath();
    std::wstring exePath = root + L"update_tmp.exe";
    
    wchar_t currentExe[MAX_PATH];
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);

    // Create a batch file to swap EXEs and restart
    std::wstring batPath = root + L"ba_update.bat";
    std::wofstream ofs(batPath);
    ofs << L"@echo off\n";
    ofs << L"timeout /t 2 /nobreak > nul\n";
    ofs << L"move /y \"" << exePath << L"\" \"" << currentExe << L"\"\n";
    ofs << L"start \"\" \"" << currentExe << L"\"\n";
    ofs << L"del \"%~f0\"\n";
    ofs.close();

    ShellExecuteW(NULL, L"open", batPath.c_str(), NULL, NULL, SW_HIDE);
    exit(0);
}
