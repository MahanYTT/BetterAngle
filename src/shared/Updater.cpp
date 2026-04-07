#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <string>

#pragma comment(lib, "wininet.lib")

bool CheckForUpdates() {
    HINTERNET hInternet = InternetOpenA("BetterAngle/4.9.4", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
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
            return (g_latestVersion > 4.92f); 
        }
    }

    return false;
}

bool DownloadUpdate(const std::wstring& url, const std::wstring& dest) {
    // Basic placeholder for download logic
    return true; 
}

void ApplyUpdateAndRestart() {
    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    std::wstring currentFullPath = szPath;
    size_t lastBackslash = currentFullPath.find_last_of(L"\\");
    std::wstring currentExe = (lastBackslash != std::wstring::npos) ? currentFullPath.substr(lastBackslash + 1) : L"BetterAngle.exe";

    std::wofstream bat(L"update_swap.bat");
    bat << L"@echo off\n";
    bat << L"timeout /t 2 /nobreak > nul\n";
    bat << L"if not exist BetterAngle_new.exe (\n";
    bat << L"  echo [ERROR] New version not found. Restarting current binary...\n";
    bat << L"  start \"\" \"" << currentExe << L"\"\n";
    bat << L"  exit\n";
    bat << L")\n";
    bat << L"del \"" << currentExe << L"\"\n";
    bat << L"move /y BetterAngle_new.exe \"" << currentExe << L"\"\n"; 
    bat << L"start \"\" \"" << currentExe << L"\"\n";
    bat << L"del %0\n";
    bat.close();

    ShellExecute(NULL, L"open", L"update_swap.bat", NULL, NULL, SW_HIDE);
    PostQuitMessage(0);
}
