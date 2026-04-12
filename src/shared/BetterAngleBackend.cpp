#include "shared/BetterAngleBackend.h"
#include "shared/FirstTimeSetup.h"
#include "shared/Logic.h"
#include "shared/Profile.h"
#include "shared/State.h"
#include "shared/Updater.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <mutex>
#include <shlobj.h>
#include <string>
#include <thread>
#include <windows.h>

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern AngleLogic g_logic;
extern double FetchFortniteSensitivity();
extern HWND g_hHUD;
extern HWND g_hPanel;
#include "shared/ControlPanel.h"
extern void __cdecl RefreshHotkeys(HWND hWnd);
void LogStartup(const std::string &msg);

std::recursive_mutex g_profileMutex;

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
    static bool lastCross = g_showCrosshair;
    static bool lastDebug = g_debugMode;

    if (g_showCrosshair != lastCross) {
      lastCross = g_showCrosshair;
      emit crosshairChanged();
    }
    if (g_debugMode != lastDebug) {
      lastDebug = g_debugMode;
      emit debugChanged();
    }

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

    // Smooth Progress Emission (v4.27.10)
    static int lastProgress = 0;
    if (g_loadingProgress != lastProgress) {
      lastProgress = g_loadingProgress;
      emit loadingProgressChanged();
    }
  });
  timer->start(100);

  // Auto-check for updates on startup
  QTimer::singleShot(2000, this, [this]() {
    if (!g_hasCheckedForUpdates && !g_isCheckingForUpdates) {
      checkForUpdates();
    }
  });

  // Task B Fix: Removed the 500ms auto-show timer.
  // The control panel is now explicitly requested via Splash.qml when its
  // animation finishes.
}

double BetterAngleBackend::sensX() const {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return 0.05;
  return g_allProfiles[g_selectedProfileIdx].sensitivityX;
}
void BetterAngleBackend::setSensX(double v) {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].sensitivityX = (std::max)(0.00001, v);
  g_allProfiles[g_selectedProfileIdx].Save(
      GetProfilesPath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
  SaveSettings();
  g_logic.LoadProfile(v);
  emit profileChanged();
}

double BetterAngleBackend::sensY() const {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return 0.05;
  return g_allProfiles[g_selectedProfileIdx].sensitivityY;
}
void BetterAngleBackend::setSensY(double v) {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].sensitivityY = (std::max)(0.00001, v);
  g_allProfiles[g_selectedProfileIdx].Save(
      GetProfilesPath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
  SaveSettings();
  emit profileChanged();
}

int BetterAngleBackend::tolerance() const {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return 2;
  return g_allProfiles[g_selectedProfileIdx].tolerance;
}
void BetterAngleBackend::setTolerance(int v) {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].tolerance = v;
  g_allProfiles[g_selectedProfileIdx].Save(
      GetProfilesPath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
  SaveSettings();
  emit profileChanged();
}

QString BetterAngleBackend::syncResult() const { return m_syncResult; }

void BetterAngleBackend::syncAndSaveProfile() {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.showCrosshair = g_showCrosshair;
  p.crossThickness = g_crossThickness;
  p.crossColor = g_crossColor;
  p.crossOffsetX = g_crossOffsetX;
  p.crossOffsetY = g_crossOffsetY;
  p.crossPulse = g_crossPulse;
  p.Save(GetProfilesPath() + p.name + L".json");
  SaveSettings();
}

bool BetterAngleBackend::crosshairOn() const {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return true;
  return g_allProfiles[g_selectedProfileIdx].showCrosshair;
}
void BetterAngleBackend::setCrosshairOn(bool v) {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].showCrosshair = v;
  g_showCrosshair = v;
  // Note: syncAndSaveProfile is not called here to avoid recursive lock if it
  // also locks g_profileMutex
  g_allProfiles[g_selectedProfileIdx].Save(
      GetProfilesPath() + g_allProfiles[g_selectedProfileIdx].name + L".json");
  SaveSettings();
  if (g_hHUD) {
    InvalidateRect(g_hHUD, NULL, FALSE);
    UpdateWindow(g_hHUD);
  }
  emit crosshairChanged();
}

float BetterAngleBackend::crossThickness() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return 2.0f;
  return g_allProfiles[g_selectedProfileIdx].crossThickness;
}
void BetterAngleBackend::setCrossThickness(float v) {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].crossThickness = v;
  g_crossThickness = v;
  syncAndSaveProfile();
  if (g_hHUD) {
    InvalidateRect(g_hHUD, NULL, FALSE);
    UpdateWindow(g_hHUD);
  }
  emit crosshairChanged();
}

float BetterAngleBackend::crossOffsetX() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return 0.0f;
  return g_allProfiles[g_selectedProfileIdx].crossOffsetX;
}
void BetterAngleBackend::setCrossOffsetX(float v) {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].crossOffsetX = v;
  g_crossOffsetX = v;
  syncAndSaveProfile();
  if (g_hHUD) {
    InvalidateRect(g_hHUD, NULL, FALSE);
    UpdateWindow(g_hHUD);
  }
  emit crosshairChanged();
}

float BetterAngleBackend::crossOffsetY() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return 0.0f;
  return g_allProfiles[g_selectedProfileIdx].crossOffsetY;
}
void BetterAngleBackend::setCrossOffsetY(float v) {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].crossOffsetY = v;
  g_crossOffsetY = v;
  syncAndSaveProfile();
  if (g_hHUD) {
    InvalidateRect(g_hHUD, NULL, FALSE);
    UpdateWindow(g_hHUD);
  }
  emit crosshairChanged();
}

bool BetterAngleBackend::crossPulse() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return false;
  return g_allProfiles[g_selectedProfileIdx].crossPulse;
}
void BetterAngleBackend::setCrossPulse(bool v) {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  g_allProfiles[g_selectedProfileIdx].crossPulse = v;
  g_crossPulse = v;
  syncAndSaveProfile();
  if (g_hHUD) {
    InvalidateRect(g_hHUD, NULL, FALSE);
    UpdateWindow(g_hHUD);
  }
  emit crosshairChanged();
}

QColor BetterAngleBackend::crossColor() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return QColor(0, 255, 204);
  COLORREF c = g_allProfiles[g_selectedProfileIdx].crossColor;
  return QColor(GetRValue(c), GetGValue(c), GetBValue(c));
}
void BetterAngleBackend::setCrossColor(const QColor &c) {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  COLORREF cr = RGB(c.red(), c.green(), c.blue());
  g_allProfiles[g_selectedProfileIdx].crossColor = cr;
  g_crossColor = cr;
  syncAndSaveProfile();
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
    m_syncResult = "NOT FOUND: Scanned AppData and Documents trees";
  }
  emit syncResultChanged();
}

void BetterAngleBackend::terminateApp() {
  // Final safety sync before process exits
  syncAndSaveProfile();

  if (g_hHUD) {
    PostMessage(g_hHUD, WM_CLOSE, 0, 0);
  }
  // Also terminate the Qt loop immediately to ensure everything shuts down
  QCoreApplication::quit();
}
void BetterAngleBackend::checkForUpdates() {
  if (g_isCheckingForUpdates)
    return;

  g_hasCheckedForUpdates = false;
  g_updateAvailable = false;
  emit updateStatusChanged();

  // Run check in a background thread and notify backend when done
  std::thread([this]() {
    CheckForUpdates();
    // Use QMetaObject to safely emit signal back to UI thread
    QMetaObject::invokeMethod(
        this,
        [this]() {
          emit updateStatusChanged();
          // Notify UI that update is available, but do NOT auto-download.
          if (g_updateAvailable) {
            qDebug() << "[UPDATER] Update found. Waiting for user to initiate "
                        "download.";
          }
        },
        Qt::QueuedConnection);
  }).detach();
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
  LogStartup("UI: requestShowControlPanel invoked.");
  emit closeSplashRequested();
  LogStartup("UI: closeSplashRequested emitted.");

  LogStartup("UI: Creating or reusing control panel root.");
  CreateControlPanel(g_hInstance);

  if (g_hHUD) {
    ShowWindow(g_hHUD, SW_SHOW);
    UpdateWindow(g_hHUD);
    LogStartup("UI: HUD window shown.");
  } else {
    LogStartup("UI: HUD window handle not available yet.");
  }

  LogStartup("UI: Emitting showControlPanelRequested.");
  emit showControlPanelRequested();
}

void BetterAngleBackend::requestToggleControlPanel() {
  emit toggleControlPanelRequested();
}

void BetterAngleBackend::finishSetup() {
  // 1. Sync current state (from QML fields) into the profile
  syncAndSaveProfile();

  // 2. Call the legacy helper (now safeguarded) for marker/atomic persistence
  extern void FinishSetup();
  FinishSetup();

  // 3. Mark setup as definitively done in persistent state
  g_setupComplete = true;
  g_needsSetup = false;

  // 4. Force atomic save of settings.json
  SaveSettings();

  // 5. Final HUD/Dashboard reveal
  if (g_hHUD) {
    ShowWindow(g_hHUD, SW_SHOW);
    UpdateWindow(g_hHUD);
    InvalidateRect(g_hHUD, NULL, FALSE);
  }
  emit profileChanged();
  emit showControlPanelRequested();
}

QStringList BetterAngleBackend::crosshairPresetNames() const {
  std::lock_guard<std::recursive_mutex> lock(g_profileMutex);
  QStringList list;
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return list;
  for (const auto &cp : g_allProfiles[g_selectedProfileIdx].crosshairPresets) {
    list << QString::fromStdWString(cp.name) + QString(" (%1, %2)")
                                                   .arg(cp.offsetX, 0, 'f', 1)
                                                   .arg(cp.offsetY, 0, 'f', 1);
  }
  return list;
}

void BetterAngleBackend::saveCrosshairPreset(const QString &name) {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
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
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
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
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
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
  if (upper == "VOL-")
    return VK_VOLUME_DOWN;
  if (upper == "VOL+")
    return VK_VOLUME_UP;
  if (upper == "MUTE")
    return VK_VOLUME_MUTE;
  if (upper == "PLAY")
    return VK_MEDIA_PLAY_PAUSE;
  if (upper == "STOP")
    return VK_MEDIA_STOP;
  if (upper.startsWith("NUM")) {
    QString numStr = upper.mid(3);
    if (numStr == "0")
      return VK_NUMPAD0;
    if (numStr == "1")
      return VK_NUMPAD1;
    if (numStr == "2")
      return VK_NUMPAD2;
    if (numStr == "3")
      return VK_NUMPAD3;
    if (numStr == "4")
      return VK_NUMPAD4;
    if (numStr == "5")
      return VK_NUMPAD5;
    if (numStr == "6")
      return VK_NUMPAD6;
    if (numStr == "7")
      return VK_NUMPAD7;
    if (numStr == "8")
      return VK_NUMPAD8;
    if (numStr == "9")
      return VK_NUMPAD9;
    if (numStr == ".")
      return VK_DECIMAL;
    if (numStr == "/")
      return VK_DIVIDE;
  }
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
  if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
    return "Num" + QString::number(vk - VK_NUMPAD0);
  if (vk == VK_DECIMAL)
    return "Num.";
  if (vk == VK_DIVIDE)
    return "Num/";
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
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return "Ctrl + U";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.toggleMod, k.toggleKey);
}
void BetterAngleBackend::setKeyToggle(const QString &s) {
  if (!g_allProfiles.empty() && g_selectedProfileIdx >= 0 &&
      g_selectedProfileIdx < (int)g_allProfiles.size()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.toggleMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.toggleKey);
    syncAndSaveProfile();
    RefreshHotkeys(g_hHUD);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyRoi() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return "Ctrl + 8";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.roiMod, k.roiKey);
}
void BetterAngleBackend::setKeyRoi(const QString &s) {
  if (!g_allProfiles.empty() && g_selectedProfileIdx >= 0 &&
      g_selectedProfileIdx < (int)g_allProfiles.size()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.roiMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.roiKey);
    syncAndSaveProfile();
    RefreshHotkeys(g_hHUD);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyCross() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return "F10";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.crossMod, k.crossKey);
}
void BetterAngleBackend::setKeyCross(const QString &s) {
  if (!g_allProfiles.empty() && g_selectedProfileIdx >= 0 &&
      g_selectedProfileIdx < (int)g_allProfiles.size()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.crossMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.crossKey);
    syncAndSaveProfile();
    RefreshHotkeys(g_hHUD);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyZero() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return "Ctrl + G";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.zeroMod, k.zeroKey);
}
void BetterAngleBackend::setKeyZero(const QString &s) {
  if (!g_allProfiles.empty() && g_selectedProfileIdx >= 0 &&
      g_selectedProfileIdx < (int)g_allProfiles.size()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.zeroMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.zeroKey);
    syncAndSaveProfile();
    RefreshHotkeys(g_hHUD);
    emit hotkeysChanged();
  }
}

QString BetterAngleBackend::keyDebug() const {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return "Ctrl + 9";
  const auto &k = g_allProfiles[g_selectedProfileIdx].keybinds;
  return fullKeyToString(k.debugMod, k.debugKey);
}
void BetterAngleBackend::setKeyDebug(const QString &s) {
  if (!g_allProfiles.empty() && g_selectedProfileIdx >= 0 &&
      g_selectedProfileIdx < (int)g_allProfiles.size()) {
    parseFullKey(s, g_allProfiles[g_selectedProfileIdx].keybinds.debugMod,
                 g_allProfiles[g_selectedProfileIdx].keybinds.debugKey);
    syncAndSaveProfile();
    RefreshHotkeys(g_hHUD);
    emit hotkeysChanged();
  }
}

void BetterAngleBackend::saveKeybinds() {
  if (g_allProfiles.empty() || g_selectedProfileIdx < 0 ||
      g_selectedProfileIdx >= (int)g_allProfiles.size())
    return;
  Profile &p = g_allProfiles[g_selectedProfileIdx];
  p.Save(GetProfilesPath() + p.name + L".json");
  RefreshHotkeys(g_hHUD);
}

int BetterAngleBackend::loadingProgress() const {
  return g_loadingProgress.load();
}
