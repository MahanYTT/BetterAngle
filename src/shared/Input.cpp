#include "shared/Input.h"
#include <iostream>

void RegisterRawMouse(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;  // Generic Desktop
    rid.usUsage = 0x02;      // Mouse
    rid.dwFlags = RIDEV_INPUTSINK; 
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "Failed to register raw input devices." << std::endl;
    }
}

int GetRawInputDeltaX(LPARAM lparam) {
    UINT dwSize;
    GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    
    if (dwSize == 0) return 0;

    LPBYTE lpb = new BYTE[dwSize];
    if (lpb == NULL) return 0;

    int bytesCopied = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
    if (bytesCopied == (UINT)-1) {
        delete[] lpb;
        return 0;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb;
    int dx = 0;

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        dx = raw->data.mouse.lLastX;
    }

    delete[] lpb;
    return dx;
}
#include "shared/State.h"

void PollInputKeys() {
    static bool f9Pressed = false;
    if (GetAsyncKeyState(VK_F9) & 0x8000) {
        if (!f9Pressed) {
            g_showROIBox = !g_showROIBox;
            f9Pressed = true;
        }
    } else {
        f9Pressed = false;
    }

    static bool rPressed = false;
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('R') & 0x8000)) {
        if (!rPressed) {
            if (g_currentSelection == NONE) {
                g_currentSelection = SELECTING_ROI;
                g_isSelectionActive = true;
            } else {
                g_currentSelection = NONE;
                g_isSelectionActive = false;
            }
            rPressed = true;
        }
    } else {
        rPressed = false;
    }
}
