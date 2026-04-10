#include "shared/BetterAngleBackend.h"
#include "shared/State.h"
#include "shared/Profile.h"
#include "shared/Logic.h"
#include "shared/Updater.h"
#include <QGuiApplication>
#include <thread>
#include <windows.h>

extern std::vector<Profile> g_allProfiles;
extern int g_selectedProfileIdx;
extern AngleLogic g_logic;
extern double FetchFortniteSensitivity();

BetterAngleBackend::BetterAngleBackend(QObject *parent) : QObject(parent) {
    // A real implementation would setup a QTimer to poll g_hasCheckedForUpdates 
    // and emit updateStatusChanged() so QML knows when the thread finishes.
}

double BetterAngleBackend::sensX() const {
    if (g_allProfiles.empty()) return 1.0;
    return g_allProfiles[g_selectedProfileIdx].sensitivityX;
}
void BetterAngleBackend::setSensX(double v) {
    if (g_allProfiles.empty()) return;
    Profile& p = g_allProfiles[g_selectedProfileIdx];
    p.sensitivityX = (std::max)(0.001, v);
    g_logic.LoadProfile(p.sensitivityX);
    p.Save(GetAppStoragePath() + p.name + L".json");
    emit profileChanged();
}

double BetterAngleBackend::sensY() const {
    if (g_allProfiles.empty()) return 1.0;
    return g_allProfiles[g_selectedProfileIdx].sensitivityY;
}
void BetterAngleBackend::setSensY(double v) {
    if (g_allProfiles.empty()) return;
    Profile& p = g_allProfiles[g_selectedProfileIdx];
    p.sensitivityY = (std::max)(0.001, v);
    p.Save(GetAppStoragePath() + p.name + L".json");
    emit profileChanged();
}

int BetterAngleBackend::tolerance() const {
    if (g_allProfiles.empty()) return 25;
    return g_allProfiles[g_selectedProfileIdx].tolerance;
}
void BetterAngleBackend::setTolerance(int v) {
    if (g_allProfiles.empty()) return;
    Profile& p = g_allProfiles[g_selectedProfileIdx];
    p.tolerance = v;
    p.Save(GetAppStoragePath() + p.name + L".json");
    emit profileChanged();
}

QString BetterAngleBackend::syncResult() const { return m_syncResult; }

bool BetterAngleBackend::crosshairOn() const { return g_showCrosshair; }
void BetterAngleBackend::setCrosshairOn(bool v) { 
    g_showCrosshair = v; SaveSettings(); emit crosshairChanged(); 
}

float BetterAngleBackend::crossThickness() const { return g_crossThickness; }
void BetterAngleBackend::setCrossThickness(float v) { 
    g_crossThickness = v; SaveSettings(); emit crosshairChanged(); 
}

float BetterAngleBackend::crossOffsetX() const { return g_crossOffsetX; }
void BetterAngleBackend::setCrossOffsetX(float v) { 
    g_crossOffsetX = v; SaveSettings(); emit crosshairChanged(); 
}

float BetterAngleBackend::crossOffsetY() const { return g_crossOffsetY; }
void BetterAngleBackend::setCrossOffsetY(float v) { 
    g_crossOffsetY = v; SaveSettings(); emit crosshairChanged(); 
}

bool BetterAngleBackend::crossPulse() const { return g_crossPulse; }
void BetterAngleBackend::setCrossPulse(bool v) { 
    g_crossPulse = v; SaveSettings(); emit crosshairChanged(); 
}

QColor BetterAngleBackend::crossColor() const { 
    return QColor(GetRValue(g_crossColor), GetGValue(g_crossColor), GetBValue(g_crossColor)); 
}
void BetterAngleBackend::setCrossColor(const QColor& c) {
    g_crossColor = RGB(c.red(), c.green(), c.blue());
    SaveSettings(); emit crosshairChanged();
}

bool BetterAngleBackend::debugMode() const { return g_debugMode; }
void BetterAngleBackend::setDebugMode(bool v) { g_debugMode = v; emit debugChanged(); }

bool BetterAngleBackend::forceDiving() const { return g_forceDiving; }
void BetterAngleBackend::setForceDiving(bool v) { g_forceDiving = v; emit debugChanged(); }

bool BetterAngleBackend::forceDetection() const { return g_forceDetection; }
void BetterAngleBackend::setForceDetection(bool v) { g_forceDetection = v; emit debugChanged(); }

float BetterAngleBackend::glideThreshold() const { return g_glideThreshold; }
void BetterAngleBackend::setGlideThreshold(float v) { g_glideThreshold = v; emit debugChanged(); }

float BetterAngleBackend::freefallThreshold() const { return g_freefallThreshold; }
void BetterAngleBackend::setFreefallThreshold(float v) { g_freefallThreshold = v; emit debugChanged(); }

QString BetterAngleBackend::versionStr() const { return QString::fromLatin1(VERSION_STR); }
QString BetterAngleBackend::latestVersion() const { return QString::fromStdWString(g_latestVersionOnline); }
bool BetterAngleBackend::updateAvailable() const { return g_updateAvailable; }
bool BetterAngleBackend::isDownloading() const { return g_isDownloadingUpdate; }

void BetterAngleBackend::syncWithFortnite() {
    double synced = FetchFortniteSensitivity();
    if (synced > 0.0) {
        setSensX(synced);
        setSensY(synced);
        m_syncResult = QString("SYNC OK! sens=%1").arg(synced);
    } else {
        m_syncResult = "CONFIG NOT FOUND! Check: %LOCALAPPDATA%\\FortniteGame\\";
    }
    emit syncResultChanged();
}

void BetterAngleBackend::terminateApp() {
    QGuiApplication::quit();
}

void BetterAngleBackend::checkForUpdates() {
    std::thread([]() { CheckForUpdates(); }).detach();
}

void BetterAngleBackend::downloadUpdate() {
    UpdateApp();
}

void BetterAngleBackend::saveThresholds() {
    SaveSettings();
}
