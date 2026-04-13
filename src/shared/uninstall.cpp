//
// Created by ItsDolphin on 4/13/2026.
//

#include "uninstall.h"

#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <system_error>

namespace fs = std::filesystem;

static std::wstring GetEnvVar(const wchar_t* name) {
    DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
    if (needed == 0) {
        return L"";
    }

    std::wstring value(needed, L'\0');
    DWORD written = GetEnvironmentVariableW(name, value.data(), needed);
    if (written == 0 || written >= needed) {
        return L"";
    }

    value.resize(written);
    return value;
}

static std::wstring GetKnownFolder(REFKNOWNFOLDERID folderId) {
    PWSTR raw = nullptr;
    std::wstring result;

    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, nullptr, &raw)) && raw != nullptr) {
        result = raw;
    }

    if (raw != nullptr) {
        CoTaskMemFree(raw);
    }

    return result;
}

static bool IsSameFileNameInsensitive(const std::wstring& a, const std::wstring& b) {
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

static void RemoveReadOnlyIfNeeded(const fs::path& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
        SetFileAttributesW(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }
}

static bool KillProcessByName(const wchar_t* exeName) {
    bool killedAny = false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (IsSameFileNameInsensitive(entry.szExeFile, exeName)) {
                HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, entry.th32ProcessID);
                if (process != nullptr) {
                    if (TerminateProcess(process, 0)) {
                        WaitForSingleObject(process, 5000);
                        killedAny = true;
                    }
                    CloseHandle(process);
                }
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return killedAny;
}

static bool RemoveFileIfExists(const fs::path& path) {
    std::error_code ec;

    if (!fs::exists(path, ec)) {
        return true;
    }

    RemoveReadOnlyIfNeeded(path);
    if (fs::is_regular_file(path, ec) || fs::is_symlink(path, ec)) {
        fs::remove(path, ec);
        return !ec;
    }

    return false;
}

static bool RemoveDirectoryRecursively(const fs::path& dir) {
    std::error_code ec;

    if (!fs::exists(dir, ec)) {
        return true;
    }

    for (fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end;
         it != end;
         it.increment(ec)) {
        if (ec) {
            break;
        }
        RemoveReadOnlyIfNeeded(it->path());
    }

    ec.clear();
    fs::remove_all(dir, ec);
    return !ec;
}

static void RemoveKnownFilesInDirectory(const fs::path& dir, const std::vector<std::wstring>& names) {
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        return;
    }

    for (const auto& name : names) {
        RemoveFileIfExists(dir / name);
    }
}

static void RemoveLikelyBetterAngleArtifacts(const fs::path& root) {
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        return;
    }

    const std::vector<std::wstring> exactNames = {
        L"BetterAngle.exe",
        L"betterangle.exe",
        L"settings.json",
        L"config.json",
        L"roi.json",
        L"roi.txt",
        L"position.json",
        L"color.json",
        L"overlay.json",
        L"state.json"
    };

    RemoveKnownFilesInDirectory(root, exactNames);

    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         it != end;
         it.increment(ec)) {
        if (ec) {
            break;
        }

        const fs::path current = it->path();
        const std::wstring filename = current.filename().wstring();

        for (const auto& known : exactNames) {
            if (IsSameFileNameInsensitive(filename, known)) {
                RemoveFileIfExists(current);
                break;
            }
        }
    }
}

int wmain() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    KillProcessByName(L"BetterAngle.exe");

    std::vector<fs::path> directoriesToRemove;
    std::vector<fs::path> extraFilesToRemove;

    const std::wstring localAppData = GetKnownFolder(FOLDERID_LocalAppData);
    const std::wstring roamingAppData = GetKnownFolder(FOLDERID_RoamingAppData);
    const std::wstring programData = GetKnownFolder(FOLDERID_ProgramData);
    const std::wstring desktop = GetKnownFolder(FOLDERID_Desktop);
    const std::wstring startMenu = GetKnownFolder(FOLDERID_Programs);

    const std::wstring programFiles = GetEnvVar(L"ProgramFiles");
    const std::wstring programFilesX86 = GetEnvVar(L"ProgramFiles(x86)");

    if (!programFiles.empty()) {
        directoriesToRemove.emplace_back(fs::path(programFiles) / L"BetterAngle");
        extraFilesToRemove.emplace_back(fs::path(programFiles) / L"BetterAngle.exe");
    }

    if (!programFilesX86.empty()) {
        directoriesToRemove.emplace_back(fs::path(programFilesX86) / L"BetterAngle");
        extraFilesToRemove.emplace_back(fs::path(programFilesX86) / L"BetterAngle.exe");
    }

    if (!localAppData.empty()) {
        directoriesToRemove.emplace_back(fs::path(localAppData) / L"BetterAngle");
    }

    if (!roamingAppData.empty()) {
        directoriesToRemove.emplace_back(fs::path(roamingAppData) / L"BetterAngle");
    }

    if (!programData.empty()) {
        directoriesToRemove.emplace_back(fs::path(programData) / L"BetterAngle");
    }

    if (!desktop.empty()) {
        extraFilesToRemove.emplace_back(fs::path(desktop) / L"BetterAngle.lnk");
    }

    if (!startMenu.empty()) {
        directoriesToRemove.emplace_back(fs::path(startMenu) / L"BetterAngle");
        extraFilesToRemove.emplace_back(fs::path(startMenu) / L"BetterAngle.lnk");
    }

    bool ok = true;

    for (const auto& file : extraFilesToRemove) {
        if (!RemoveFileIfExists(file)) {
            ok = false;
        }
    }

    for (const auto& dir : directoriesToRemove) {
        RemoveLikelyBetterAngleArtifacts(dir);
        if (!RemoveDirectoryRecursively(dir)) {
            ok = false;
        }
    }

    std::wcout << (ok ? L"Uninstall completed.\n" : L"Uninstall completed with some errors.\n");

    CoUninitialize();
    return ok ? 0 : 1;
}