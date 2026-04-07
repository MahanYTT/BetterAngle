#ifndef INPUT_H
#define INPUT_H

#include <windows.h>

// Raw Input Mouse Delta Capture
void RegisterRawMouse(HWND hwnd);
int GetRawInputDeltaX(LPARAM lparam);

#endif // INPUT_H
