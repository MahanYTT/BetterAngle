#include "shared/Updater.h"
#include "shared/State.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

// Placeholders for actual URLs — user should replace these in GitHub Actions or
// similar if needed GitHub API for latest release info
const wchar_t *VERSION_URL =
    L"https://api.github.com/repos/MahanYTT/BetterAngle/releases/latest";
const wchar_t *DOWNLOAD_URL = L"https://github.com/MahanYTT/BetterAngle/"
                              L"releases/latest/download/BetterAngle_Setup.exe";

bool DownloadFile(const std::wstring &url, const std::wstring &dest) {
  HINTERNET hInternet =
      InternetOpenW(L"BetterAngle", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (!hInternet)
    return false;

  // GitHub API requires a User-Agent and recommends an Accept header
  std::wstring headers =
      L"Accept: application/vnd.github.v3+json\r\nUser-Agent: "
      L"BetterAngleUpdater\r\n";
  HINTERNET hUrl =
      InternetOpenUrlW(hInternet, url.c_str(), headers.c_str(), (DWORD)-1,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);

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
  while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) &&
         bytesRead > 0) {
    ofs.write(buffer, bytesRead);
  }

  ofs.close();
  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInternet);
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
      std::string json((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
      ifs.close();

      // Search for "tag_name": "vX.Y.Z"
      size_t tagPos = json.find("\"tag_name\":");
      if (tagPos != std::string::npos) {
        size_t start = json.find("\"", tagPos + 11);
        size_t end = (start != std::string::npos) ? json.find("\"", start + 1)
                                                  : std::string::npos;

        if (start != std::string::npos && end != std::string::npos) {
          std::string latestVerStr = json.substr(start + 1, end - start - 1);
          g_latestVersionOnline = latestVerStr;
          success = true;

          // Search for "browser_download_url": "..."
          size_t assetPos = json.find("\"browser_download_url\":");
          while (assetPos != std::string::npos) {
            size_t uS = json.find("\"", assetPos + 23);
            size_t uE = (uS != std::string::npos) ? json.find("\"", uS + 1)
                                                  : std::string::npos;
            if (uS != std::string::npos && uE != std::string::npos) {
              std::string uStr = json.substr(uS + 1, uE - uS - 1);
              if (uStr.find("BetterAngle_Setup.exe") != std::string::npos) {
                g_dynamicDownloadUrl = std::wstring(uStr.begin(), uStr.end());
                break;
              }
            }
            assetPos = json.find("\"browser_download_url\":", assetPos + 23);
          }

          // Version normalization (strip 'v')
          std::string normLatest = latestVerStr;
          if (!normLatest.empty() &&
              (normLatest[0] == 'v' || normLatest[0] == 'V'))
            normLatest = normLatest.substr(1);

          std::string currentVer = VERSION_STR;
          if (!currentVer.empty() &&
              (currentVer[0] == 'v' || currentVer[0] == 'V'))
            currentVer = currentVer.substr(1);

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
  g_hasCheckedForUpdates = true; // Always true after attempt, even if failed
  if (!success) {
    g_updateHistory = "Update check failed";
  }
  return g_updateAvailable;
}

void UpdateApp() {
  if (g_isDownloadingUpdate || g_downloadComplete)
    return;

  g_isDownloadingUpdate = true;
  std::thread([]() {
    std::wstring dest = GetAppRootPath() + L"BetterAngle_Setup_update.exe";
    if (DownloadFile(g_dynamicDownloadUrl, dest)) {
      g_downloadComplete = true;
    }
    g_isDownloadingUpdate = false;
  }).detach();
}

void CleanupUpdateJunk() {
  std::wstring root = GetAppRootPath();
  DeleteFileW((root + L"ba_update.bat").c_str());
  DeleteFileW((root + L"BetterAngle_Setup_update.exe").c_str());
  DeleteFileW((root + L"latest_version.txt").c_str());
}

void ApplyUpdateAndRestart() {
  std::wstring root = GetAppRootPath();
  std::wstring installerPath = root + L"BetterAngle_Setup_update.exe";

  if (GetFileAttributesW(installerPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    g_downloadComplete = false;
    g_updateAvailable = true;
    ShellExecuteW(NULL, L"open",
                  L"https://github.com/MahanYTT/BetterAngle/releases/latest",
                  NULL, NULL, SW_SHOWNORMAL);
    return;
  }

  std::wstring params = L"/SP- /VERYSILENT /SUPPRESSMSGBOXES /NORESTART "
                        L"/CLOSEAPPLICATIONS /FORCECLOSEAPPLICATIONS";
  HINSTANCE result = ShellExecuteW(NULL, L"open", installerPath.c_str(),
                                   params.c_str(), NULL, SW_SHOWNORMAL);
  if ((INT_PTR)result <= 32) {
    g_downloadComplete = false;
    g_updateAvailable = true;
    ShellExecuteW(NULL, L"open",
                  L"https://github.com/MahanYTT/BetterAngle/releases/latest",
                  NULL, NULL, SW_SHOWNORMAL);
    return;
  }

  exit(0);
}
