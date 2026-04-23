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

// Low-Level Hardware Hooks for anti-ghosting input locking
void InstallHardwareHooks();
void UninstallHardwareHooks();

#endif // INPUT_H
