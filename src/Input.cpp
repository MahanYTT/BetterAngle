#include "Input.h"
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

    if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
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
