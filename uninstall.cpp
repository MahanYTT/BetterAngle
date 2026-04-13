#include <windows.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include <filesystem>
#include <fstream>
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

static std::wstring Quote(const std::wstring& value) {
    return L"\"" + value + L"\"";
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

static std::wstring GetModulePath() {
    std::wstring buffer(MAX_PATH, L'\0');

    for (;;) {
        DWORD written = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) {
            return L"";
        }

        if (written < buffer.size() - 1) {
            buffer.resize(written);
            return buffer;
        }

        buffer.resize(buffer.size() * 2);
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

static fs::path DetectInstallDirFromCurrentExecutable() {
    const fs::path currentExe = GetModulePath();
    if (currentExe.empty()) {
        return {};
    }

    const fs::path parent = currentExe.parent_path();
    const std::wstring parentName = parent.filename().wstring();

    if (EqualsIgnoreCase(parentName, L"BetterAngle")) {
        return parent;
    }

    if (EqualsIgnoreCase(currentExe.filename().wstring(), L"uninstaller.exe")) {
        return parent;
    }

    return parent;
}

static bool WriteCleanupScript(const fs::path& scriptPath,
                               const fs::path& currentExe,
                               const fs::path& installDir,
                               const std::vector<fs::path>& extraDirs,
                               const std::vector<fs::path>& extraFiles) {
    std::wofstream script(scriptPath);
    if (!script.is_open()) {
        return false;
    }

    script << L"@echo off\n";
    script << L"setlocal\n";
    script << L"timeout /t 2 /nobreak >nul\n";
    script << L":retry_main\n";
    script << L"del /f /q " << Quote(currentExe.wstring()) << L" >nul 2>nul\n";
    script << L"if exist " << Quote(currentExe.wstring()) << L" (\n";
    script << L"  timeout /t 1 /nobreak >nul\n";
    script << L"  goto retry_main\n";
    script << L")\n";

    for (const auto& file : extraFiles) {
        script << L"del /f /q " << Quote(file.wstring()) << L" >nul 2>nul\n";
    }

    for (const auto& dir : extraDirs) {
        script << L"rmdir /s /q " << Quote(dir.wstring()) << L" >nul 2>nul\n";
    }

    if (!installDir.empty()) {
        script << L"rmdir /s /q " << Quote(installDir.wstring()) << L" >nul 2>nul\n";
    }

    script << L"del /f /q %~f0 >nul 2>nul\n";
    return true;
}

static bool LaunchCleanupScript(const fs::path& scriptPath) {
    std::wstring command = L"/c start \"\" /min " + Quote(scriptPath.wstring());

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::wstring mutableCommand = command;
    BOOL ok = CreateProcessW(
        L"C:\\Windows\\System32\\cmd.exe",
        mutableCommand.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);

    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }

    return false;
}

int wmain() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    const fs::path currentExe = GetModulePath();
    const fs::path installDir = DetectInstallDirFromCurrentExecutable();

    KillProcessByName(L"BetterAngle.exe");
    Sleep(1500);

    const std::wstring localAppData = GetKnownFolder(FOLDERID_LocalAppData);
    const std::wstring roamingAppData = GetKnownFolder(FOLDERID_RoamingAppData);
    const std::wstring programData = GetKnownFolder(FOLDERID_ProgramData);
    const std::wstring desktop = GetKnownFolder(FOLDERID_Desktop);
    const std::wstring startMenu = GetKnownFolder(FOLDERID_Programs);
    const std::wstring tempDir = GetEnvVar(L"TEMP");

    std::vector<fs::path> filesToRemoveNow;
    std::vector<fs::path> dirsToRemoveNow;
    std::vector<fs::path> filesToRemoveLater;
    std::vector<fs::path> dirsToRemoveLater;

    if (!localAppData.empty()) {
        dirsToRemoveNow.emplace_back(fs::path(localAppData) / L"BetterAngle");
    }

    if (!roamingAppData.empty()) {
        dirsToRemoveNow.emplace_back(fs::path(roamingAppData) / L"BetterAngle");
    }

    if (!programData.empty()) {
        dirsToRemoveNow.emplace_back(fs::path(programData) / L"BetterAngle");
    }

    if (!desktop.empty()) {
        filesToRemoveNow.emplace_back(fs::path(desktop) / L"BetterAngle.lnk");
    }

    if (!startMenu.empty()) {
        filesToRemoveNow.emplace_back(fs::path(startMenu) / L"BetterAngle.lnk");
        dirsToRemoveNow.emplace_back(fs::path(startMenu) / L"BetterAngle");
    }

    const std::vector<std::wstring> likelyDataFiles = {
        L"roi.json",
        L"roi.txt",
        L"position.json",
        L"color.json",
        L"settings.json",
        L"config.json",
        L"state.json",
        L"overlay.json"
    };

    bool ok = true;

    for (const auto& dir : dirsToRemoveNow) {
        RemoveNamedFilesRecursively(dir, likelyDataFiles);
    }

    for (const auto& file : filesToRemoveNow) {
        if (!RemoveFileIfExists(file)) {
            ok = false;
        }
    }

    for (const auto& dir : dirsToRemoveNow) {
        if (!RemoveDirectoryIfExists(dir)) {
            ok = false;
        }
    }

    if (!installDir.empty()) {
        filesToRemoveLater.emplace_back(installDir / L"BetterAngle.exe");
        dirsToRemoveLater.emplace_back(installDir);
    }

    if (currentExe.empty() || tempDir.empty()) {
        std::wcerr << L"Unable to prepare uninstall cleanup.\n";
        CoUninitialize();
        return 1;
    }

    const fs::path scriptPath = fs::path(tempDir) / L"betterangle_cleanup.cmd";

    if (!WriteCleanupScript(scriptPath, currentExe, installDir, dirsToRemoveLater, filesToRemoveLater)) {
        std::wcerr << L"Failed to create cleanup script.\n";
        CoUninitialize();
        return 1;
    }

    if (!LaunchCleanupScript(scriptPath)) {
        std::wcerr << L"Failed to launch cleanup script.\n";
        CoUninitialize();
        return 1;
    }

    std::wcout << (ok ? L"BetterAngle uninstall started.\n"
                      : L"BetterAngle uninstall started, but some files may require elevation.\n");

    CoUninitialize();
    return 0;
}