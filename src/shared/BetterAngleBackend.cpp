#include "shared/BetterAngleBackend.h"
#include "shared/Logic.h"
#include "shared/Profile.h"
#include "shared/State.h"
#include "shared/Updater.h"
#include <QGuiApplication>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <shlobj.h>
#include <thread>
#include <windows.h>

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern AngleLogic g_logic;

static double g_pendingSetupSensX = 0.05;
static double g_pendingSetupSensY = 0.05;

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

  // Auto-check for updates on startup
  QTimer::singleShot(2000, this, [this]() {
    if (!g_hasCheckedForUpdates && !g_isCheckingForUpdates) {
      checkForUpdates();
    }
  });
}

double BetterAngleBackend::sensX() const {
  if (g_allProfiles.empty())
    return g_pendingSetupSensX;
  return g_allProfiles[g_selectedProfileIdx].sensitivityX;
}
void BetterAngleBackend::setSensX(double v) {
  double normalized = (std::max)(0.0001, std::round(v * 10.0) / 10.0);
  if (g_allProfiles.empty()) {
    g_pendingSetupSensX = normalized;
    emit profileChanged();
    return;
  }
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.sensitivityX = normalized;
  g_logic.LoadProfile(p.sensitivityX);
  p.Save(GetProfilesPath() + p.name + L".json");
  SaveSettings();
  emit profileChanged();
}

double BetterAngleBackend::sensY() const {
  if (g_allProfiles.empty())
    return g_pendingSetupSensY;
  return g_allProfiles[g_selectedProfileIdx].sensitivityY;
}
void BetterAngleBackend::setSensY(double v) {
  double normalized = (std::max)(0.0001, std::round(v * 10.0) / 10.0);
  if (g_allProfiles.empty()) {
    g_pendingSetupSensY = normalized;
    emit profileChanged();
    return;
  }
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.sensitivityY = normalized;
  p.Save(GetProfilesPath() + p.name + L".json");
  SaveSettings();
  emit profileChanged();
}

int BetterAngleBackend::tolerance() const {
  if (g_allProfiles.empty())
    return 2;
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
bool BetterAngleBackend::isCheckingForUpdates() const {
  return g_isCheckingForUpdates;
}
bool BetterAngleBackend::downloadComplete() const { return g_downloadComplete; }
bool BetterAngleBackend::hasCheckedForUpdates() const {
  return g_hasCheckedForUpdates;
}
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

void BetterAngleBackend::terminateApp() {
  SaveSettings();
  if (g_hHUD) {
    PostMessage(g_hHUD, WM_CLOSE, 0, 0);
  }
  // Also terminate the Qt loop immediately to ensure everything shuts down
  QCoreApplication::quit();
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

void BetterAngleBackend::requestShowControlPanel() {
  emit showControlPanelRequested();
}

void BetterAngleBackend::finishSetup() {
  if (g_allProfiles.empty()) {
    Profile p;
    p.name = L"Default";
    p.tolerance = 2;
    p.roi_x = 760;
    p.roi_y = 640;
    p.roi_w = 400;
    p.roi_h = 70;
    p.target_color = RGB(150, 150, 150);
    p.fov = 80.0f;
    p.resolutionWidth = GetSystemMetrics(SM_CXSCREEN);
    p.resolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    p.renderScale = 100.0f;
    p.sensitivityX = g_pendingSetupSensX;
    p.sensitivityY = g_pendingSetupSensY;
    p.showCrosshair = g_showCrosshair;
    p.crossThickness = g_crossThickness;
    p.crossColor = g_crossColor;
    p.crossOffsetX = g_crossOffsetX;
    p.crossOffsetY = g_crossOffsetY;
    p.crossAngle = g_crossAngle;
    p.crossPulse = g_crossPulse;
    p.Save(GetProfilesPath() + L"Default.json");
    g_allProfiles.push_back(p);
    g_selectedProfileIdx = 0;
    g_lastLoadedProfileName = p.name;
  } else {
    if (g_selectedProfileIdx < 0 ||
        g_selectedProfileIdx >= (int)g_allProfiles.size()) {
      g_selectedProfileIdx = 0;
    }
    Profile &p = g_allProfiles[g_selectedProfileIdx];
    p.sensitivityX = g_pendingSetupSensX;
    p.sensitivityY = g_pendingSetupSensY;
    p.showCrosshair = g_showCrosshair;
    p.crossThickness = g_crossThickness;
    p.crossColor = g_crossColor;
    p.crossOffsetX = g_crossOffsetX;
    p.crossOffsetY = g_crossOffsetY;
    p.crossAngle = g_crossAngle;
    p.crossPulse = g_crossPulse;
    p.Save(GetProfilesPath() + p.name + L".json");
    g_lastLoadedProfileName = p.name;
  }

  g_setupComplete = true;
  SaveSettings();

  std::wstring marker = GetAppRootPath() + L".setup_done";
  HANDLE h = CreateFileW(marker.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_HIDDEN, NULL);
  if (h != INVALID_HANDLE_VALUE)
    CloseHandle(h);

  if (!g_allProfiles.empty())
    g_logic.LoadProfile(g_allProfiles[g_selectedProfileIdx].sensitivityX);

  emit profileChanged();
}

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

// --- Hotkey Helpers ---
static UINT stringToVk(const QString &s) {
  QString upper = s.toUpper().trimmed();
  if (upper.isEmpty())
    return 0;
  if (upper.length() == 1) {
    char c = upper[0].toLatin1();
    if (c >= 'A' && c <= 'Z')
      return (UINT)c;
    if (c >= '0' && c <= '9')
      return (UINT)c;
  }
  if (upper == "F1")
    return VK_F1;
  if (upper == "F2")
    return VK_F2;
  if (upper == "F3")
    return VK_F3;
  if (upper == "F4")
    return VK_F4;
  if (upper == "F5")
    return VK_F5;
  if (upper == "F6")
    return VK_F6;
  if (upper == "F7")
    return VK_F7;
  if (upper == "F8")
    return VK_F8;
  if (upper == "F9")
    return VK_F9;
  if (upper == "F10")
    return VK_F10;
  if (upper == "F11")
    return VK_F11;
  if (upper == "F12")
    return VK_F12;
  if (upper == "TAB")
    return VK_TAB;
  if (upper == "SPACE")
    return VK_SPACE;
  if (upper == "ESC")
    return VK_ESCAPE;
  if (upper == "CTRL")
    return VK_CONTROL;
  if (upper == "SHIFT")
    return VK_SHIFT;
  if (upper == "ALT")
    return VK_MENU;
  return 0;
}

static QString vkToString(UINT vk) {
  if (vk >= 'A' && vk <= 'Z')
    return QString((char)vk);
  if (vk >= '0' && vk <= '9')
    return QString((char)vk);
  if (vk == VK_F1)
    return "F1";
  if (vk == VK_F2)
    return "F2";
  if (vk == VK_F3)
    return "F3";
  if (vk == VK_F4)
    return "F4";
  if (vk == VK_F5)
    return "F5";
  if (vk == VK_F6)
    return "F6";
  if (vk == VK_F7)
    return "F7";
  if (vk == VK_F8)
    return "F8";
  if (vk == VK_F9)
    return "F9";
  if (vk == VK_F10)
    return "F10";
  if (vk == VK_F11)
    return "F11";
  if (vk == VK_F12)
    return "F12";
  if (vk == VK_TAB)
    return "TAB";
  if (vk == VK_SPACE)
    return "SPACE";
  if (vk == VK_ESCAPE)
    return "ESC";
  if (vk == VK_CONTROL)
    return "CTRL";
  if (vk == VK_SHIFT)
    return "SHIFT";
  if (vk == VK_MENU)
    return "ALT";
  return "";
}

static QString fullKeyToString(UINT mod, UINT vk) {
  QString res;
  if (mod & MOD_CONTROL)
    res += "Ctrl + ";
  if (mod & MOD_SHIFT)
    res += "Shift + ";
  if (mod & MOD_ALT)
    res += "Alt + ";
  res += vkToString(vk);
  return res;
}

static void parseFullKey(const QString &s, UINT &outMod, UINT &outKey) {
  outMod = 0;
  QString lower = s.toLower();
  if (lower.contains("ctrl"))
    outMod |= MOD_CONTROL;
  if (lower.contains("shift"))
    outMod |= MOD_SHIFT;
  if (lower.contains("alt"))
    outMod |= MOD_ALT;

  QString keyPart = s;
  if (s.contains("+")) {
    keyPart = s.split("+").last().trimmed();
  }
  outKey = stringToVk(keyPart);
}

QString BetterAngleBackend::keyToggle() const {
  if (g_allProfiles.empty())
    return "Ctrl + U";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.toggleMod, k.toggleKey);
}
void BetterAngleBackend::setKeyToggle(const QString &s) {
  if (!g_allProfiles.empty()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.toggleMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.toggleKey);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyRoi() const {
  if (g_allProfiles.empty())
    return "Ctrl + R";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.roiMod, k.roiKey);
}
void BetterAngleBackend::setKeyRoi(const QString &s) {
  if (!g_allProfiles.empty()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.roiMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.roiKey);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyCross() const {
  if (g_allProfiles.empty())
    return "F10";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.crossMod, k.crossKey);
}
void BetterAngleBackend::setKeyCross(const QString &s) {
  if (!g_allProfiles.empty()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.crossMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.crossKey);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyZero() const {
  if (g_allProfiles.empty())
    return "Ctrl + G";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.zeroMod, k.zeroKey);
}
void BetterAngleBackend::setKeyZero(const QString &s) {
  if (!g_allProfiles.empty()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.zeroMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.zeroKey);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyDebug() const {
  if (g_allProfiles.empty())
    return "Ctrl + 9";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.debugMod, k.debugKey);
}
void BetterAngleBackend::setKeyDebug(const QString &s) {
  if (!g_allProfiles.empty()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.debugMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.debugKey);
    emit hotkeysChanged();
  }
}

void BetterAngleBackend::saveKeybinds() {
  if (g_allProfiles.empty())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.Save(GetProfilesPath() + p.name + L".json");
  RefreshHotkeys(g_hHUD);
}
