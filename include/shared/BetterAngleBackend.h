#pragma once
#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>


class BetterAngleBackend : public QObject {
  Q_OBJECT
  Q_PROPERTY(double sensX READ sensX WRITE setSensX NOTIFY profileChanged)
  Q_PROPERTY(double sensY READ sensY WRITE setSensY NOTIFY profileChanged)
  Q_PROPERTY(
      int tolerance READ tolerance WRITE setTolerance NOTIFY profileChanged)

  Q_PROPERTY(bool crosshairOn READ crosshairOn WRITE setCrosshairOn NOTIFY
                 crosshairChanged)
  Q_PROPERTY(float crossThickness READ crossThickness WRITE setCrossThickness
                 NOTIFY crosshairChanged)
  Q_PROPERTY(float crossOffsetX READ crossOffsetX WRITE setCrossOffsetX NOTIFY
                 crosshairChanged)
  Q_PROPERTY(float crossOffsetY READ crossOffsetY WRITE setCrossOffsetY NOTIFY
                 crosshairChanged)
  Q_PROPERTY(bool crossPulse READ crossPulse WRITE setCrossPulse NOTIFY
                 crosshairChanged)
  Q_PROPERTY(QColor crossColor READ crossColor WRITE setCrossColor NOTIFY
                 crosshairChanged)

  Q_PROPERTY(
      bool debugMode READ debugMode WRITE setDebugMode NOTIFY debugChanged)
  Q_PROPERTY(bool forceDiving READ forceDiving WRITE setForceDiving NOTIFY
                 debugChanged)
  Q_PROPERTY(bool forceDetection READ forceDetection WRITE setForceDetection
                 NOTIFY debugChanged)
  Q_PROPERTY(float glideThreshold READ glideThreshold WRITE setGlideThreshold
                 NOTIFY debugChanged)
  Q_PROPERTY(float freefallThreshold READ freefallThreshold WRITE
                 setFreefallThreshold NOTIFY debugChanged)

  Q_PROPERTY(QString versionStr READ versionStr CONSTANT)
  Q_PROPERTY(
      QString latestVersion READ latestVersion NOTIFY updateStatusChanged)
  Q_PROPERTY(
      bool updateAvailable READ updateAvailable NOTIFY updateStatusChanged)
  Q_PROPERTY(
      bool downloadComplete READ downloadComplete NOTIFY updateStatusChanged)
  Q_PROPERTY(
      QString updateHistory READ updateHistory NOTIFY updateStatusChanged)
  Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateStatusChanged)
  Q_PROPERTY(bool hasCheckedForUpdates READ hasCheckedForUpdates NOTIFY
                 updateStatusChanged)
  Q_PROPERTY(bool isCheckingForUpdates READ isCheckingForUpdates NOTIFY
                 updateStatusChanged)
  Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY updateStatusChanged)

  // Custom Keybinds
  Q_PROPERTY(
      QString keyToggle READ keyToggle WRITE setKeyToggle NOTIFY hotkeysChanged)
  Q_PROPERTY(QString keyRoi READ keyRoi WRITE setKeyRoi NOTIFY hotkeysChanged)
  Q_PROPERTY(
      QString keyCross READ keyCross WRITE setKeyCross NOTIFY hotkeysChanged)
  Q_PROPERTY(
      QString keyZero READ keyZero WRITE setKeyZero NOTIFY hotkeysChanged)
  Q_PROPERTY(
      QString keyDebug READ keyDebug WRITE setKeyDebug NOTIFY hotkeysChanged)

public:
  explicit BetterAngleBackend(QObject *parent = nullptr);

  double sensX() const;
  void setSensX(double v);

  double sensY() const;
  void setSensY(double v);

  int tolerance() const;
  void setTolerance(int v);


  bool crosshairOn() const;
  void setCrosshairOn(bool v);

  float crossThickness() const;
  void setCrossThickness(float v);

  float crossOffsetX() const;
  void setCrossOffsetX(float v);

  float crossOffsetY() const;
  void setCrossOffsetY(float v);

  bool crossPulse() const;
  void setCrossPulse(bool v);

  QColor crossColor() const;
  void setCrossColor(const QColor &c);

  bool debugMode() const;
  void setDebugMode(bool v);

  bool forceDiving() const;
  void setForceDiving(bool v);

  bool forceDetection() const;
  void setForceDetection(bool v);

  float glideThreshold() const;
  void setGlideThreshold(float v);

  float freefallThreshold() const;
  void setFreefallThreshold(float v);

  QString versionStr() const;
  QString latestVersion() const;
  bool updateAvailable() const;
  bool isDownloading() const;
  bool downloadComplete() const;
  QString updateHistory() const;
  QString updateStatus() const;
  bool hasCheckedForUpdates() const;
  bool isCheckingForUpdates() const;

  Q_INVOKABLE void terminateApp();
  Q_INVOKABLE void checkForUpdates();
  Q_INVOKABLE void downloadUpdate();
  Q_INVOKABLE void saveThresholds();
  Q_INVOKABLE void requestShowControlPanel();
  Q_INVOKABLE void finishSetup();

  // Crosshair preset management
  Q_INVOKABLE QStringList crosshairPresetNames() const;
  Q_INVOKABLE void saveCrosshairPreset(const QString &name);
  Q_INVOKABLE void loadCrosshairPreset(int index);
  Q_INVOKABLE void deleteCrosshairPreset(int index);

  // Keybind management
  QString keyToggle() const;
  void setKeyToggle(const QString &s);
  QString keyRoi() const;
  void setKeyRoi(const QString &s);
  QString keyCross() const;
  void setKeyCross(const QString &s);
  QString keyZero() const;
  void setKeyZero(const QString &s);
  QString keyDebug() const;
  void setKeyDebug(const QString &s);
  Q_INVOKABLE void saveKeybinds();

signals:
  void profileChanged();
  void crosshairChanged();
  void debugChanged();
  void updateStatusChanged();
  void crosshairPresetsChanged();
  void showControlPanelRequested();
  void hotkeysChanged();

private:
};
