#ifndef UPDATER_H
#define UPDATER_H

#include <windows.h>
#include <string>

class Updater {
public:
    Updater(const std::string& repo);
    bool CheckForUpdate(const std::string& currentVersion);
    bool DownloadLatest(const std::string& path);

private:
    std::string m_repo;
    std::string m_latestTag;
    std::string m_downloadUrl;
};

#endif // UPDATER_H
