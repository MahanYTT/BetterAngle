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
    HINTERNET hInternet = InternetOpenA("BetterAngle/4.9.8", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, "https://api.github.com/repos/MahanYTT/BetterAngle/releases/latest", NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    std::string jsonResponse;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        jsonResponse.append(buffer, bytesRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    // Manual Parsing for "tag_name":"vX.X.X"
    size_t tagPos = jsonResponse.find("\"tag_name\":\"");
    if (tagPos != std::string::npos) {
        tagPos += 12; // Skip "tag_name":"
        size_t endPos = jsonResponse.find("\"", tagPos);
        if (endPos != std::string::npos) {
            g_latestVersionOnline = jsonResponse.substr(tagPos, endPos - tagPos);
            
            // Convert "v4.9.3" to float 4.93f for comparison
            std::string verNum = g_latestVersionOnline;
            if (verNum[0] == 'v') verNum = verNum.substr(1);
            try {
                g_latestVersion = std::stof(verNum);
            } catch (...) {
                g_latestVersion = 4.82f;
            }
            
            g_latestName = L"GitHub Production Release";
            
            if (g_latestVersionOnline != "v4.9.8") {
                g_updateAvailable = true;
            } else {
                g_updateAvailable = false;
            }
            
            g_isCheckingForUpdates = false;
            return g_updateAvailable; 
        }
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
    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    std::wstring currentFullPath = szPath;
    size_t lastBackslash = currentFullPath.find_last_of(L"\\");
    std::wstring currentExe = (lastBackslash != std::wstring::npos) ? currentFullPath.substr(lastBackslash + 1) : L"BetterAngle.exe";

    std::wofstream bat(L"update.bat");
    bat << L"@echo off\n";
    bat << L":loop\n";
    bat << L"taskkill /F /IM \"" << currentExe << L"\" >nul 2>&1\n";
    bat << L"if %errorlevel%==0 goto loop\n";
    bat << L"del \"" << currentExe << L"\"\n";
    bat << L"rename update_tmp.exe \"" << currentExe << L"\"\n"; 
    bat << L"start \"\" \"" << currentExe << L"\"\n";
    bat << L"del %0\n";
    bat.close();

    ShellExecute(NULL, L"open", L"update.bat", NULL, NULL, SW_HIDE);
    exit(0);
}
