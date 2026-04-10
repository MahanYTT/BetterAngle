#ifndef PROFILE_H
#define PROFILE_H

#include <string>
#include <vector>
#include <windows.h>

struct CrosshairPreset {
  std::wstring name;
  float offsetX;
  float offsetY;
  float angle;
};


struct Profile {
  std::wstring name;
  double sensitivityX;
  double sensitivityY;

  // Reference Metadata
  float fov;
  int resolutionWidth;
  int resolutionHeight;
  float renderScale;

  // Detector Logic
  int roi_x, roi_y, roi_w, roi_h;
  COLORREF target_color;
  int tolerance;

  // Crosshair Settings
  float crossThickness;
  COLORREF crossColor;
  float crossOffsetX;
  float crossOffsetY;
  float crossAngle;
  bool crossPulse;

  std::vector<CrosshairPreset> crosshairPresets;

  bool Load(const std::wstring &path);
  bool Save(const std::wstring &path);
};

std::vector<Profile> GetProfiles(const std::wstring &directory);

#endif // PROFILE_H
