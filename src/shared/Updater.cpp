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
    HINTERNET hInternet = InternetOpenW(L"BetterAngle", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    // GitHub API requires a User-Agent and recommends an Accept header
    std::wstring headers = L"Accept: application/vnd.github.v3+json\r\nUser-Agent: BetterAngleUpdater\r\n";
    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), headers.c_str(), (DWORD)-1, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
    
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

    char buffer[8192];
    DWORD bytesRead;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        ofs.write(buffer, bytesRead);
    }

    ofs.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    // Safety Guard: Check if file size is suspiciously small (e.g. < 5MB)
    // A corrupted or "Not Found" HTML page is usually a few KB.
    std::ifstream check(dest, std::ios::binary | std::ios::ate);
    if (check.is_open()) {
        std::streamsize size = check.tellg();
        if (size < 1 * 1024 * 1024) { // 1 MB minimum
            check.close();
            DeleteFileW(dest.c_str());
            return false;
        }
        check.close();
    }

    return true;
}

static std::wstring g_dynamicDownloadUrl = DOWNLOAD_URL;

bool CheckForUpdates() {
    g_isCheckingForUpdates = true;
    bool success = false;
    
    std::wstring tempRes = GetAppRootPath() + L"latest_release.json";
    if (DownloadFile(VERSION_URL, tempRes)) {
        std::ifstream ifs(tempRes);
        if (ifs.is_open()) {
            std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            
            // Search for "tag_name": "vX.Y.Z"
            size_t tagPos = json.find("\"tag_name\":");
            if (tagPos != std::string::npos) {
                size_t start = json.find("\"", tagPos + 11);
                size_t end = (start != std::string::npos) ? json.find("\"", start + 1) : std::string::npos;
                
                if (start != std::string::npos && end != std::string::npos) {
                    std::string latestVerStr = json.substr(start + 1, end - start - 1);
                    g_latestVersionOnline = latestVerStr;
                    success = true;

                    // Search for "browser_download_url": "..."
                    size_t assetPos = json.find("\"browser_download_url\":");
                    while (assetPos != std::string::npos) {
                        size_t uS = json.find("\"", assetPos + 23);
                        size_t uE = (uS != std::string::npos) ? json.find("\"", uS + 1) : std::string::npos;
                        if (uS != std::string::npos && uE != std::string::npos) {
                            std::string uStr = json.substr(uS + 1, uE - uS - 1);
                            if (uStr.find("BetterAngle.exe") != std::string::npos) {
                                g_dynamicDownloadUrl = std::wstring(uStr.begin(), uStr.end());
                                break;
                            }
                        }
                        assetPos = json.find("\"browser_download_url\":", assetPos + 23);
                    }
                    
                    // Version normalization (strip 'v')
                    std::string normLatest = latestVerStr;
                    if (!normLatest.empty() && (normLatest[0] == 'v' || normLatest[0] == 'V')) normLatest = normLatest.substr(1);
                    
                    std::string currentVer = VERSION_STR;
                    if (!currentVer.empty() && (currentVer[0] == 'v' || currentVer[0] == 'V')) currentVer = currentVer.substr(1);

                    if (!normLatest.empty() && normLatest != currentVer) {
                        g_updateAvailable = true;
                        g_updateHistory = "Latest: " + latestVerStr + " (Manual Check)";
                    } else {
                        g_updateAvailable = false;
                    }
                }
            }
        }
        DeleteFileW(tempRes.c_str());
    }

    g_isCheckingForUpdates = false;
    g_hasCheckedForUpdates = success;
    return g_updateAvailable;
}

void UpdateApp() {
    if (g_isDownloadingUpdate || g_downloadComplete) return;

    g_isDownloadingUpdate = true;
    std::thread([]() {
        std::wstring dest = GetAppRootPath() + L"update_tmp.exe";
        if (DownloadFile(g_dynamicDownloadUrl, dest)) {
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
