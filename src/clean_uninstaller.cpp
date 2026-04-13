#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <tlhelp32.h>
#include <shlobj.h>

namespace fs = std::filesystem;

void KillProcessByName(const std::wstring& filename) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (filename == pe.szExeFile) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // 1. Kill any running BetterAngle processes
    KillProcessByName(L"BetterAngle.exe");
    Sleep(1000); // Give it time to close

    // 2. Identify paths to delete
    wchar_t localAppData[MAX_PATH];
    wchar_t appData[MAX_PATH];
    
    std::vector<fs::path> pathsToDelete;

    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH)) {
        pathsToDelete.push_back(fs::path(localAppData) / "BetterAngle");
    }
    if (GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH)) {
        pathsToDelete.push_back(fs::path(appData) / "BetterAngle");
    }

    // Path where the uninstaller is usually running from (the install dir)
    wchar_t modulePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, modulePath, MAX_PATH)) {
        fs::path installDir = fs::path(modulePath).parent_path();
        pathsToDelete.push_back(installDir);
    }

    // 2.5. Shortcuts Cleanup
    auto deleteLnk = [&](int csidl, const std::string& name) {
        wchar_t path[MAX_PATH];
        if (SHGetSpecialFolderPathW(NULL, path, csidl, FALSE)) {
            fs::path shortcut = fs::path(path) / (name + ".lnk");
            if (fs::exists(shortcut)) {
                try { fs::remove(shortcut); } catch(...) {}
            }
        }
    };

    std::vector<std::string> names = { "BetterAngle Pro", "BetterAngle" };
    for (const auto& name : names) {
        deleteLnk(CSIDL_DESKTOP, name);
        deleteLnk(CSIDL_STARTUP, name);
        deleteLnk(CSIDL_PROGRAMS, name);
        deleteLnk(CSIDL_COMMON_STARTUP, name);
    }

    // 3. Delete folders
    for (const auto& p : pathsToDelete) {
        try {
            if (fs::exists(p)) {
                fs::remove_all(p);
            }
        } catch (...) {
            // Some files might be in use or protected
        }
    }

    // 4. Show completion message (optional, but good for UX)
    MessageBoxW(NULL, L"BetterAngle has been completely removed from your system.", L"Uninstallation Complete", MB_OK | MB_ICONINFORMATION);

    return 0;
}
