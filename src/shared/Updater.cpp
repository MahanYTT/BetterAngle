#include "shared/Updater.h"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <string>

#pragma comment(lib, "winhttp.lib")

bool CheckForUpdates() {
    // Current simple logic: Check a version file on GitHub
    return true; // For testing v4.7.3 One-Click
}

bool DownloadUpdate(const std::wstring& url, const std::wstring& dest) {
    HINTERNET hSession = WinHttpOpen(L"BetterAngle/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    // Simplified download logic for v4.7.3
    // In production, this would use WinHttpOpenRequest and WinHttpSendRequest
    // to stream the BetterAngle.exe from GitHub releases.
    WinHttpCloseHandle(hSession);
    return true; 
}

void ApplyUpdateAndRestart() {
    // Create a temporary batch script to swap the EXE
    std::ofstream bat("update_swap.bat");
    bat << "@echo off\n";
    bat << "timeout /t 2 /nobreak > nul\n"; // Wait for app to close
    bat << "del BetterAngle.exe\n";
    bat << "move /y BetterAngle_new.exe BetterAngle.exe\n";
    bat << "start BetterAngle.exe\n";
    bat << "del %0\n";
    bat.close();

    ShellExecute(NULL, L"open", L"update_swap.bat", NULL, NULL, SW_HIDE);
    PostQuitMessage(0);
}
