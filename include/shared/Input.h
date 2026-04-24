#ifndef INPUT_H
#define INPUT_H

#include <windows.h>

#include <vector>

// Raw Input Mouse & Keyboard Tracking
extern bool g_physicalKeys[256];
void RegisterRawInput(HWND hwnd);
int GetRawInputDeltaX(LPARAM lparam);
void UpdatePhysicalKeyState(LPARAM lparam);

// Runtime input gating helpers
bool IsFortniteForeground();
bool IsCursorCurrentlyVisible();

// Nitro Anti-Ghosting (Delta-Only)
std::vector<bool> GetGamingKeyState();
void SyncGamingKeysNitro(const std::vector<bool>& initialState);

#endif // INPUT_H
