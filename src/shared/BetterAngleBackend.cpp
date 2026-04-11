#include "shared/BetterAngleBackend.h"
#include "shared/Logic.h"
#include "shared/Profile.h"
#include "shared/State.h"
#include "shared/Updater.h"
#include <QGuiApplication>
#include <QTimer>
#include <thread>
#include <windows.h>
#include <shlobj.h>


extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern AngleLogic g_logic;
extern double FetchFortniteSensitivity();

BetterAngleBackend::BetterAngleBackend(QObject *parent) : QObject(parent) {
  // Emit profileChanged once the Qt event loop starts so QML fields
  // refresh with whatever sensitivityX is in g_allProfiles (set by setup or
  // loaded from disk).
  QTimer::singleShot(0, this, [this]() { emit profileChanged(); });

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this]() {
    static bool lastChecking = false;
    static bool lastDownloading = false;
    static bool lastComplete = false;
    static bool lastChecked = false;
    if (g_hasCheckedForUpdates != lastChecked) {
      lastChecked = g_hasCheckedForUpdates;
      emit updateStatusChanged();
    }

    if (g_isCheckingForUpdates != lastChecking) {
      lastChecking = g_isCheckingForUpdates;
      emit updateStatusChanged();
    }
    if (g_isDownloadingUpdate != lastDownloading) {
      lastDownloading = g_isDownloadingUpdate;
      emit updateStatusChanged();
    }
    if (g_downloadComplete != lastComplete) {
      lastComplete = g_downloadComplete;
      emit updateStatusChanged();
    }
  });
  timer->start(100);
}

double BetterAngleBackend::sensX() const {
  if (g_allProfiles.empty())
    return 1.0;
  return g_allProfiles[g_selectedProfileIdx].sensitivityX;
}
void BetterAngleBackend::setSensX(double v) {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.sensitivityX = (std::max)(0.001, v);
  g_logic.LoadProfile(p.sensitivityX);
  p.Save(GetProfilesPath() + p.name + L".json");
  SaveSettings();
  emit profileChanged();
}

double BetterAngleBackend::sensY() const {
  if (g_allProfiles.empty())
    return 1.0;
  return g_allProfiles[g_selectedProfileIdx].sensitivityY;
}
void BetterAngleBackend::setSensY(double v) {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.sensitivityY = (std::max)(0.001, v);
  p.Save(GetProfilesPath() + p.name + L".json");
  SaveSettings();
  emit profileChanged();
}

int BetterAngleBackend::tolerance() const {
  if (g_allProfiles.empty())
    return 25;
  return g_allProfiles[g_selectedProfileIdx].tolerance;
}
void BetterAngleBackend::setTolerance(int v) {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.tolerance = v;
  p.Save(GetProfilesPath() + p.name + L".json");
  SaveSettings();
  emit profileChanged();
}

QString BetterAngleBackend::syncResult() const { return m_syncResult; }

bool BetterAngleBackend::crosshairOn() const { return g_showCrosshair; }
void BetterAngleBackend::setCrosshairOn(bool v) {
  g_showCrosshair = v;
  SaveSettings();
  emit crosshairChanged();
}

float BetterAngleBackend::crossThickness() const { return g_crossThickness; }
void BetterAngleBackend::setCrossThickness(float v) {
  g_crossThickness = v;
  SaveSettings();
  emit crosshairChanged();
}

float BetterAngleBackend::crossOffsetX() const { return g_crossOffsetX; }
void BetterAngleBackend::setCrossOffsetX(float v) {
  g_crossOffsetX = v;
  SaveSettings();
  emit crosshairChanged();
}

float BetterAngleBackend::crossOffsetY() const { return g_crossOffsetY; }
void BetterAngleBackend::setCrossOffsetY(float v) {
  g_crossOffsetY = v;
  SaveSettings();
  emit crosshairChanged();
}

bool BetterAngleBackend::crossPulse() const { return g_crossPulse; }
void BetterAngleBackend::setCrossPulse(bool v) {
  g_crossPulse = v;
  SaveSettings();
  emit crosshairChanged();
}

QColor BetterAngleBackend::crossColor() const {
  return QColor(GetRValue(g_crossColor), GetGValue(g_crossColor),
                GetBValue(g_crossColor));
}
void BetterAngleBackend::setCrossColor(const QColor &c) {
  g_crossColor = RGB(c.red(), c.green(), c.blue());
  SaveSettings();
  emit crosshairChanged();
}

bool BetterAngleBackend::debugMode() const { return g_debugMode; }
void BetterAngleBackend::setDebugMode(bool v) {
  g_debugMode = v;
  SaveSettings();
  emit debugChanged();
}

bool BetterAngleBackend::forceDiving() const { return g_forceDiving; }
void BetterAngleBackend::setForceDiving(bool v) {
  g_forceDiving = v;
  SaveSettings();
  emit debugChanged();
}

bool BetterAngleBackend::forceDetection() const { return g_forceDetection; }
void BetterAngleBackend::setForceDetection(bool v) {
  g_forceDetection = v;
  SaveSettings();
  emit debugChanged();
}

float BetterAngleBackend::glideThreshold() const { return g_glideThreshold; }
void BetterAngleBackend::setGlideThreshold(float v) {
  g_glideThreshold = v;
  SaveSettings();
  emit debugChanged();
}

float BetterAngleBackend::freefallThreshold() const {
  return g_freefallThreshold;
}
void BetterAngleBackend::setFreefallThreshold(float v) {
  g_freefallThreshold = v;
  SaveSettings();
  emit debugChanged();
}

QString BetterAngleBackend::versionStr() const {
  return QString::fromLatin1(VERSION_STR);
}
QString BetterAngleBackend::latestVersion() const {
  return QString::fromStdString(g_latestVersionOnline);
}
bool BetterAngleBackend::updateAvailable() const { return g_updateAvailable; }
bool BetterAngleBackend::isDownloading() const { return g_isDownloadingUpdate; }
bool BetterAngleBackend::downloadComplete() const { return g_downloadComplete; }
QString BetterAngleBackend::updateHistory() const {
  return QString::fromStdString(g_updateHistory);
}

QString BetterAngleBackend::updateStatus() const {
  if (g_isDownloadingUpdate)
    return "Downloading update...";
  if (g_downloadComplete)
    return "Download complete! Restart to apply.";
  if (g_isCheckingForUpdates)
    return "Checking for updates...";
  if (g_hasCheckedForUpdates) {
    if (g_updateAvailable)
      return "New update available!";
    return "Application is up to date.";
  }
  return "";
}

void BetterAngleBackend::syncWithFortnite() {
  double synced = FetchFortniteSensitivity();
  if (synced > 0.0) {
    setSensX(synced);
    setSensY(synced);
    m_syncResult = QString("SYNC OK! Value: %1").arg(synced);
  } else {
    // More detailed diagnostic info for the user
    wchar_t appdata[MAX_PATH] = {};
    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata);
    QString path =
        QString::fromWCharArray(appdata) + "\\FortniteGame\\Saved\\Config";
    m_syncResult =
        QString("ERROR: No config in %1 (Checked Client & NoEditor)").arg(path);
  }
  emit syncResultChanged();
}

void BetterAngleBackend::terminateApp() {
  // Save everything before quitting so last position/preferences are preserved
  if (!g_allProfiles.empty()) {
    Profile &p = g_allProfiles[g_selectedProfileIdx];
    p.crossThickness = g_crossThickness;
    p.crossColor = g_crossColor;
    p.crossOffsetX = g_crossOffsetX;
    p.crossOffsetY = g_crossOffsetY;
    p.crossAngle = g_crossAngle;
    p.crossPulse = g_crossPulse;
    p.Save(GetProfilesPath() + p.name + L".json");
  }

  SaveSettings();
  QGuiApplication::quit();
}

void BetterAngleBackend::checkForUpdates() {
  g_hasCheckedForUpdates = false; 
  g_updateAvailable = false;
  emit updateStatusChanged();
  std::thread([]() { CheckForUpdates(); }).detach();
}

void BetterAngleBackend::downloadUpdate() {
  if (g_downloadComplete) {
      ApplyUpdateAndRestart();
      return;
  }
  UpdateApp();
}

void BetterAngleBackend::saveThresholds() { SaveSettings(); }





QStringList BetterAngleBackend::crosshairPresetNames() const {
  QStringList list;
  if (g_allProfiles.empty())
    return list;
  for (const auto &cp : g_allProfiles[g_selectedProfileIdx].crosshairPresets) {
    list << QString::fromStdWString(cp.name) + QString(" (%1, %2)")
                                                   .arg(cp.offsetX, 0, 'f', 1)
                                                   .arg(cp.offsetY, 0, 'f', 1);
  }
  return list;
}

void BetterAngleBackend::saveCrosshairPreset(const QString &name) {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  CrosshairPreset cp;
  cp.name = name.toStdWString();
  cp.offsetX = g_crossOffsetX;
  cp.offsetY = g_crossOffsetY;
  cp.angle = g_crossAngle;
  // Replace existing with same name, otherwise append
  for (auto &existing : p.crosshairPresets) {
    if (existing.name == cp.name) {
      existing = cp;
      p.Save(GetProfilesPath() + p.name + L".json");
      emit crosshairPresetsChanged();
      return;
    }
  }
  p.crosshairPresets.push_back(cp);
  p.Save(GetProfilesPath() + p.name + L".json");

  emit crosshairPresetsChanged();
}

void BetterAngleBackend::loadCrosshairPreset(int index) {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  if (index < 0 || index >= (int)p.crosshairPresets.size())
    return;
  const CrosshairPreset &cp = p.crosshairPresets[index];
  g_crossOffsetX = cp.offsetX;
  g_crossOffsetY = cp.offsetY;
  g_crossAngle = cp.angle;
  p.crossOffsetX = cp.offsetX;
  p.crossOffsetY = cp.offsetY;
  p.crossAngle = cp.angle;
  p.Save(GetProfilesPath() + p.name + L".json");
  emit crosshairChanged();
}

void BetterAngleBackend::deleteCrosshairPreset(int index) {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  if (index < 0 || index >= (int)p.crosshairPresets.size())
    return;
  p.crosshairPresets.erase(p.crosshairPresets.begin() + index);
  p.Save(GetProfilesPath() + p.name + L".json");
  emit crosshairPresetsChanged();
}
