#ifndef PROFILE_H
#define PROFILE_H

#include <string>
#include <vector>
#include <windows.h>

struct Keybinds {
  UINT toggleMod = MOD_CONTROL;
  UINT toggleKey = 'U';
  UINT roiMod = MOD_CONTROL;
  UINT roiKey = 'R';
  UINT crossMod = 0;
  UINT crossKey = VK_F10;
  UINT zeroMod = MOD_CONTROL;
  UINT zeroKey = 'G';
  UINT debugMod = MOD_CONTROL;
  UINT debugKey = '9';
};

struct CrosshairPreset {
  std::wstring name;
  float offsetX;
  float offsetY;
  float angle;
};

struct Profile {
  std::wstring name;
  double sensitivityX = 0.05;
  double sensitivityY = 0.05;

  // Reference Metadata
  float fov;
  int resolutionWidth;
  int resolutionHeight;
  float renderScale;

  // Detector Logic
  int roi_x = 0, roi_y = 0, roi_w = 0, roi_h = 0;
  COLORREF target_color = 0;
  int tolerance = 2;

  // Crosshair Settings
  bool showCrosshair;
  float crossThickness;
  COLORREF crossColor;
  float crossOffsetX;
  float crossOffsetY;
  float crossAngle;
  bool crossPulse;

  Keybinds keybinds;
  std::vector<CrosshairPreset> crosshairPresets;

  bool Load(const std::wstring &path);
  bool Save(const std::wstring &path);
};

std::vector<Profile> GetProfiles(const std::wstring &directory);

#endif // PROFILE_H
