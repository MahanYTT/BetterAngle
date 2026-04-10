#ifndef PROFILE_H
#define PROFILE_H

#include <string>
#include <vector>
#include <windows.h>


struct Profile {
  std::wstring name;
  int dpi;
  double sensitivityX;
  double sensitivityY;
  double divingScaleMultiplier;

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

  bool Load(const std::wstring &path);
  bool Save(const std::wstring &path);
};

std::vector<Profile> GetProfiles(const std::wstring &directory);

#endif // PROFILE_H
