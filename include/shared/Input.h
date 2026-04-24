#ifndef INPUT_H
#define INPUT_H

#include <windows.h>

#include <vector>

// Raw Input Mouse Delta Capture
void RegisterRawMouse(HWND hwnd);
int GetRawInputDeltaX(LPARAM lparam);

// Runtime input gating helpers
bool IsFortniteForeground();
bool IsCursorCurrentlyVisible();

// Refined Anti-Ghosting (Gaming Cluster Only)
void ReleaseGamingKeys();
void SyncGamingKeys();

#endif // INPUT_H
