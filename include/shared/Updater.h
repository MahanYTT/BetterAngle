#ifndef UPDATER_H
#define UPDATER_H

#include <windows.h>
#include <string>
#include <vector>

bool CheckForUpdates();
void StartUpdate();
bool DownloadUpdate(const std::wstring& url, const std::wstring& dest);
void ApplyUpdateAndRestart();

extern std::vector<std::wstring> g_cloudProfileNames;
extern std::vector<std::wstring> g_cloudProfileUrls;
extern bool g_isCheckingCloud;
void FetchCloudProfiles();
void DownloadCloudProfile(int index);

#endif // UPDATER_H
