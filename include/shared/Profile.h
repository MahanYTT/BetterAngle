#ifndef PROFILE_H
#define PROFILE_H

#include <string>
#include <vector>
#include <windows.h>

struct Keybinds {
  UINT toggleMod = MOD_CONTROL;
  UINT toggleKey = 'U';
  UINT roiMod = MOD_CONTROL;
  UINT roiKey = '8';
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
  float fov = 0.0f;
  int resolutionWidth = 0;
  int resolutionHeight = 0;
  float renderScale = 0.0f;

  // Detector Logic
  int roi_x = 0, roi_y = 0, roi_w = 0, roi_h = 0;
  COLORREF target_color = 0;
  int tolerance = 2;

  // Crosshair Settings
  bool showCrosshair = true;
  float crossThickness = 2.0f;
  COLORREF crossColor = RGB(0, 255, 204);
  float crossOffsetX = 0.0f;
  float crossOffsetY = 0.0f;
  float crossAngle = 0.0f;
  bool crossPulse = true;

  Keybinds keybinds;
  std::vector<CrosshairPreset> crosshairPresets;

  bool Load(const std::wstring &path);
  bool Save(const std::wstring &path);
};

std::vector<Profile> GetProfiles(const std::wstring &directory);

#endif // PROFILE_H
