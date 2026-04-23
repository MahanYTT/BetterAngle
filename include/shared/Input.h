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

// Anti-Ghosting Hardware Synchronizer
void SyncKeyStates(const std::vector<int>& preBlockKeys);

#endif // INPUT_H
