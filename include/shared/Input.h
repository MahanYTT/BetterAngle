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

// Immediately synthesize KEYUP for all currently-held keys to prevent character ghosting
// when BlockInput fires while movement/action keys are still physically held down.
void ReleaseHeldKeys();

#endif // INPUT_H
