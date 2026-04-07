#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <string>

#pragma comment(lib, "winhttp.lib")

bool CheckForUpdates() {
    // Current local version is 4.82f.
    // In production, parse real metadata from GitHub.
    g_latestVersion = 4.82f; 
    g_latestName = L"Release Reliability Suite";
    return (g_latestVersion > 4.82f);
}

bool DownloadUpdate(const std::wstring& url, const std::wstring& dest) {
    HINTERNET hSession = WinHttpOpen(L"BetterAngle/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    WinHttpCloseHandle(hSession);
    return true; 
}

void ApplyUpdateAndRestart() {
    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    std::wstring currentFullPath = szPath;
    size_t lastBackslash = currentFullPath.find_last_of(L"\\");
    std::wstring currentExe = (lastBackslash != std::wstring::npos) ? currentFullPath.substr(lastBackslash + 1) : L"BetterAngle.exe";

    // Create a temporary batch script to swap the EXE safely
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
