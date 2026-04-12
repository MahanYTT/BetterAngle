#include "shared/Logic.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <shlobj.h>
#include <vector>
#include <filesystem>

// Case-insensitive string search helper
size_t find_case_insensitive(const std::string& buffer, const std::string& key) {
    auto it = std::search(
        buffer.begin(), buffer.end(),
        key.begin(), key.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return (it != buffer.end()) ? std::distance(buffer.begin(), it) : std::string::npos;
}

double FetchFortniteSensitivity() {
    wchar_t appdata[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        return -1.0;
    }

    // FIX 1: Start at FortniteGame\ not FortniteGame\Saved\ so search still works
    // if Saved\ is missing or named differently
    std::wstring localPath = std::wstring(appdata) + L"\\FortniteGame";

    wchar_t userDoc[MAX_PATH] = {};
    SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, userDoc);
    std::wstring docPath = std::wstring(userDoc) + L"\\FortniteGame";

    std::vector<std::wstring> configFiles;

    std::vector<std::wstring> rootPaths = { localPath, docPath };
    for (const auto& root : rootPaths) {
        if (root.empty()) continue;

        // Priority: check the known exact path first
        std::wstring exact = root + L"\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";
        std::error_code ec;
        if (std::filesystem::exists(exact, ec) && !ec) {
            configFiles.push_back(exact);
        }
    }

    if (configFiles.empty()) {
        for (const auto& root : rootPaths) {
            if (configFiles.size() > 5) break;

            // FIX 2: Check root exists before iterating to avoid silent failures
            std::error_code ec;
            if (!std::filesystem::exists(root, ec) || ec) {
                std::wcerr << L"FORTNITE DETECT: Root not accessible: " << root << std::endl;
                continue;
            }

            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                root, std::filesystem::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file()) {
                    std::wstring fname = entry.path().filename().wstring();
                    std::wstring fext  = entry.path().extension().wstring();

                    std::wstring fnameLower = fname;
                    std::transform(fnameLower.begin(), fnameLower.end(), fnameLower.begin(), ::towlower);

                    if (fnameLower.find(L"gameusersettings") != std::wstring::npos &&
                        (fext == L".ini" || fext == L".INI")) {
                        configFiles.push_back(entry.path().wstring());
                    }
                }
            }
        }
    }

    if (configFiles.empty()) {
        // Diagnostic: show exact path being checked
        std::wstring diagPath = localPath + L"\\Saved\\Config\\WindowsClient\\GameUserSettings.ini";
        std::wcerr << L"FORTNITE DETECT: File not found. Checked: " << diagPath << std::endl;
        return -1.0;
    }

    for (const auto& path : configFiles) {
        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (!ifs.is_open()) continue;

        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size <= 0 || size > 10 * 1024 * 1024) continue;

        std::string buffer(static_cast<size_t>(size), '\0');
        if (!ifs.read(&buffer[0], size)) continue;

        // Handle UTF-16 LE
        if (size >= 2 && (unsigned char)buffer[0] == 0xFF && (unsigned char)buffer[1] == 0xFE) {
            std::wstring wbuf((size - 2) / 2, L'\0');
            memcpy(&wbuf[0], buffer.data() + 2, size - 2);
            buffer.clear();
            for (wchar_t c : wbuf) {
                if (c == 0) break; // FIX 3: break instead of continue on null wchar
                buffer += (c < 128 ? (char)c : ' ');
            }
        } else {
            buffer.erase(std::remove(buffer.begin(), buffer.end(), '\0'), buffer.end());
        }

        // FIX 4: Narrow search to the correct INI section so we don't grab
        // a MouseSensitivity value from the wrong section
        size_t sectionPos = find_case_insensitive(buffer, "[/Script/FortniteGame.FortGameUserSettings]");
        std::string searchArea = (sectionPos != std::string::npos)
            ? buffer.substr(sectionPos)
            : buffer; // fallback to full file if section not found

        // FIX 5: Removed "MouseMouseSensitivity" typo, correct priority order
        const char* keys[] = { "MouseSensitivity", "MouseSensitivityX", "MouseX" };
        for (const char* key : keys) {
            size_t pos = find_case_insensitive(searchArea, key);
            if (pos != std::string::npos) {
                size_t eqPos = searchArea.find('=', pos);
                if (eqPos != std::string::npos && eqPos < pos + 50) {
                    size_t valStart = eqPos + 1;
                    size_t valEnd   = searchArea.find_first_of("\r\n", valStart);
                    if (valEnd == std::string::npos) valEnd = searchArea.size();

                    std::string valStr = searchArea.substr(valStart, valEnd - valStart);
                    valStr.erase(0, valStr.find_first_not_of(" \t\r\n\"'"));
                    valStr.erase(valStr.find_last_not_of(" \t\r\n\"'") + 1);

                    if (!valStr.empty()) {
                        try {
                            double val = std::stod(valStr);
                            if (val > 0.0) return val;
                        } catch (...) {}
                    }
                }
            }
        }
    }

    return -1.0;
}

bool IsFortniteFocused() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    wchar_t cls[256] = { 0 };
    GetClassNameW(fg, cls, 256);
    if (wcscmp(cls, L"UnrealWindow") != 0) return false;
    wchar_t title[256] = { 0 };
    GetWindowTextW(fg, title, 256);
    return wcsstr(title, L"Fortnite") != nullptr;
}

AngleLogic::AngleLogic(double sensX)
    : m_sensX(sensX), m_isDiving(false), m_accumDx(0), m_baseDx(0), m_baseAngle(0.0) {}

void AngleLogic::Update(int dx) {
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    double currentSens = m_sensX.load();
    double scale = 0.00555555 * currentSens;
    if (m_isDiving.load()) {
        scale *= 1.0916;
    }
    double delta = (double)(m_accumDx.load() - m_baseDx.load());
    return m_baseAngle.load() + (delta * scale);
}

void AngleLogic::SetZero() {
    m_accumDx = 0;
    m_baseDx  = 0;
    m_baseAngle = 0.0;
}

void AngleLogic::LoadProfile(double sensX) {
    m_baseAngle = GetAngle();
    m_baseDx    = m_accumDx.load();
    m_sensX     = sensX;
}

void AngleLogic::SetDivingState(bool diving) {
    if (diving == m_isDiving.load()) return;
    m_baseAngle = GetAngle();
    m_baseDx    = m_accumDx.load();
    m_isDiving  = diving;
}

double AngleLogic::Norm360(double a) const {
    while (a >= 360.0) a -= 360.0;
    while (a < 0.0)    a += 360.0;
    return a;
}
