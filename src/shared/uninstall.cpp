#include <windows.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

static std::wstring GetKnownFolder(REFKNOWNFOLDERID folderId) {
    PWSTR raw = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, nullptr, &raw)) && raw != nullptr) {
        result = raw;
        CoTaskMemFree(raw);
    }
    return result;
}

static std::wstring GetEnvVar(const wchar_t* name) {
    DWORD size = GetEnvironmentVariableW(name, nullptr, 0);
    if (size == 0) {
        return L"";
    }

    std::wstring value(size, L'\0');
    DWORD written = GetEnvironmentVariableW(name, value.data(), size);
    if (written == 0 || written >= size) {
        return L"";
    }

    value.resize(written);
    return value;
}

static bool EqualsIgnoreCase(const std::wstring& a, const std::wstring& b) {
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

static void ClearReadOnlyAttribute(const fs::path& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
        SetFileAttributesW(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }
}

static bool KillProcessByName(const wchar_t* processName) {
    bool terminated = false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (EqualsIgnoreCase(entry.szExeFile, processName)) {
                HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, entry.th32ProcessID);
                if (process != nullptr) {
                    if (TerminateProcess(process, 0)) {
                        WaitForSingleObject(process, 5000);
                        terminated = true;
                    }
                    CloseHandle(process);
                }
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return terminated;
}

static bool RemoveFileIfExists(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return true;
    }

    ClearReadOnlyAttribute(path);
    fs::remove(path, ec);
    return !ec;
}

static bool RemoveDirectoryIfExists(const fs::path& dir) {
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
        ClearReadOnlyAttribute(it->path());
    }

    ec.clear();
    fs::remove_all(dir, ec);
    return !ec;
}

static void RemoveNamedFilesRecursively(const fs::path& root, const std::vector<std::wstring>& names) {
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        return;
    }

    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         it != end;
         it.increment(ec)) {
        if (ec) {
            break;
        }

        const std::wstring filename = it->path().filename().wstring();
        for (const auto& name : names) {
            if (EqualsIgnoreCase(filename, name)) {
                RemoveFileIfExists(it->path());
                break;
            }
        }
    }
}

int wmain() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    KillProcessByName(L"BetterAngle.exe");

    const std::wstring localAppData = GetKnownFolder(FOLDERID_LocalAppData);
    const std::wstring roamingAppData = GetKnownFolder(FOLDERID_RoamingAppData);
    const std::wstring programData = GetKnownFolder(FOLDERID_ProgramData);
    const std::wstring desktop = GetKnownFolder(FOLDERID_Desktop);
    const std::wstring startMenu = GetKnownFolder(FOLDERID_Programs);
    const std::wstring programFiles = GetEnvVar(L"ProgramFiles");
    const std::wstring programFilesX86 = GetEnvVar(L"ProgramFiles(x86)");

    std::vector<fs::path> filesToRemove;
    std::vector<fs::path> directoriesToRemove;

    if (!programFiles.empty()) {
        filesToRemove.emplace_back(fs::path(programFiles) / L"BetterAngle.exe");
        directoriesToRemove.emplace_back(fs::path(programFiles) / L"BetterAngle");
    }

    if (!programFilesX86.empty()) {
        filesToRemove.emplace_back(fs::path(programFilesX86) / L"BetterAngle.exe");
        directoriesToRemove.emplace_back(fs::path(programFilesX86) / L"BetterAngle");
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
        filesToRemove.emplace_back(fs::path(desktop) / L"BetterAngle.lnk");
    }

    if (!startMenu.empty()) {
        filesToRemove.emplace_back(fs::path(startMenu) / L"BetterAngle.lnk");
        directoriesToRemove.emplace_back(fs::path(startMenu) / L"BetterAngle");
    }

    const std::vector<std::wstring> likelyDataFiles = {
        L"roi.json",
        L"roi.txt",
        L"position.json",
        L"color.json",
        L"settings.json",
        L"config.json",
        L"state.json",
        L"overlay.json",
        L"BetterAngle.exe"
    };

    bool ok = true;

    for (const auto& dir : directoriesToRemove) {
        RemoveNamedFilesRecursively(dir, likelyDataFiles);
    }

    for (const auto& file : filesToRemove) {
        if (!RemoveFileIfExists(file)) {
            ok = false;
        }
    }

    for (const auto& dir : directoriesToRemove) {
        if (!RemoveDirectoryIfExists(dir)) {
            ok = false;
        }
    }

    std::wcout << (ok ? L"BetterAngle uninstall completed.\n"
                      : L"BetterAngle uninstall completed with some errors.\n");

    CoUninitialize();
    return ok ? 0 : 1;
}