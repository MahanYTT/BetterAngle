#include "shared/Input.h"
#include <cwchar>
#include <tlhelp32.h>
#include <windows.h>

namespace {
bool IsFortniteProcessName(const wchar_t *processName) {
  if (!processName || !processName[0])
    return false;

  const wchar_t *knownPrefixes[] = {L"FortniteClient-Win64-Shipping",
                                    L"FortniteLauncher", L"FortniteClient"};

  for (const wchar_t *prefix : knownPrefixes) {
    const size_t prefixLen = wcslen(prefix);
    if (_wcsnicmp(processName, prefix, prefixLen) == 0) {
      return true;
    }
  }

  return false;
}

const wchar_t *GetProcessBaseName(HWND hwnd, wchar_t *buffer,
                                  DWORD bufferCount) {
  if (!hwnd || !buffer || bufferCount == 0)
    return L"";

  buffer[0] = L'\0';

  DWORD processId = 0;
  GetWindowThreadProcessId(hwnd, &processId);
  if (processId == 0)
    return L"";

  HANDLE process =
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
  if (!process)
    return L"";

  DWORD size = bufferCount;
  if (!QueryFullProcessImageNameW(process, 0, buffer, &size) || size == 0) {
    CloseHandle(process);
    buffer[0] = L'\0';
    return L"";
  }

  CloseHandle(process);

  const wchar_t *lastSlash = wcsrchr(buffer, L'\\');
  const wchar_t *lastForwardSlash = wcsrchr(buffer, L'/');
  const wchar_t *baseName = buffer;

  if (lastSlash && lastForwardSlash)
    baseName =
        (lastSlash > lastForwardSlash) ? lastSlash + 1 : lastForwardSlash + 1;
  else if (lastSlash)
    baseName = lastSlash + 1;
  else if (lastForwardSlash)
    baseName = lastForwardSlash + 1;

  return baseName;
}
} // namespace

void RegisterRawMouse(HWND hwnd) {
  RAWINPUTDEVICE rid;
  rid.usUsagePage = 0x01; // Generic Desktop
  rid.usUsage = 0x02;     // Mouse
  rid.dwFlags = RIDEV_INPUTSINK;
  rid.hwndTarget = hwnd;

  if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
    OutputDebugStringA("Failed to register raw input devices.\n");
  }
}

int GetRawInputDeltaX(LPARAM lparam) {
  UINT dwSize;
  GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dwSize,
                  sizeof(RAWINPUTHEADER));

  if (dwSize == 0)
    return 0;

  LPBYTE lpb = new BYTE[dwSize];
  if (lpb == NULL)
    return 0;

  int bytesCopied = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize,
                                    sizeof(RAWINPUTHEADER));
  if (bytesCopied == (UINT)-1) {
    delete[] lpb;
    return 0;
  }

  RAWINPUT *raw = (RAWINPUT *)lpb;
  int dx = 0;

  if (raw->header.dwType == RIM_TYPEMOUSE) {
    if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
      static int lastAbsX = -1;
      int absX = raw->data.mouse.lLastX;
      if (lastAbsX != -1 && absX != 0) {
        dx = absX - lastAbsX;
      }
      if (absX != 0)
        lastAbsX = absX;
    } else {
      dx = raw->data.mouse.lLastX;
    }
  }

  delete[] lpb;
  return dx;
}

bool IsFortniteForeground() {
  // Cache the result: recompute only when the foreground window actually
  // changes. This avoids calling CreateToolhelp32Snapshot() on every WM_INPUT
  // message (hundreds of times/sec), which caused severe lag after alt-tab
  // when EAC/BattlEye blocks the fast OpenProcess path.
  static HWND   s_lastFgHwnd = nullptr;
  static bool   s_lastResult  = false;
  static DWORD  s_lastPid     = 0;

  HWND fg = GetForegroundWindow();

  // Fast path: same window as last check, return cached answer immediately.
  if (fg == s_lastFgHwnd)
    return s_lastResult;

  // Foreground window changed — invalidate and recompute.
  s_lastFgHwnd = fg;
  s_lastResult  = false;
  s_lastPid     = 0;

  if (!fg)
    return false;
  }

  DWORD pid = 0;
  GetWindowThreadProcessId(fg, &pid);
  if (pid == 0) {
    return false;
  }

  s_lastPid = pid;

  // Method 1: Try OpenProcess + QueryFullProcessImageNameW (fast, ~0ms).
  wchar_t processPath[MAX_PATH] = {};
  const wchar_t *processName = GetProcessBaseName(fg, processPath, MAX_PATH);
  if (processName && processName[0] && IsFortniteProcessName(processName)) {
    s_lastResult = true;
    return true;
  }

  // Method 2: Fallback using CreateToolhelp32Snapshot.
  // Only reached once per foreground-window change, not on every mouse event.
  // Works even when OpenProcess is blocked by anti-cheat (EAC/BattlEye).
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(snap, &pe)) {
      do {
        if (pe.th32ProcessID == pid) {
          CloseHandle(snap);
          s_lastResult = IsFortniteProcessName(pe.szExeFile);
          return s_lastResult;
        }
      } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
  }

  s_lastResult = false;
  return false;
}

bool IsCursorCurrentlyVisible() {
  CURSORINFO cursorInfo = {sizeof(CURSORINFO)};
  if (!GetCursorInfo(&cursorInfo))
    return true;
  return (cursorInfo.flags & CURSOR_SHOWING) != 0;
}

void SyncKeyStates(const std::vector<int>& preBlockKeys) {
  // Sync physical hardware state with logical state after a BlockInput window.
  // Re-evaluates all 255 virtual keys.
  for (int vk = 1; vk < 255; vk++) {
    bool downBefore = false;
    for (int pre_vk : preBlockKeys) {
      if (pre_vk == vk) {
        downBefore = true;
        break;
      }
    }

    bool downAfter = (GetAsyncKeyState(vk) & 0x8000) != 0;

    if (downBefore && !downAfter) {
      // Key was physically released while blocked. The system ate the KEYUP.
      // We must synthesize it to prevent game character ghosting.
      INPUT in = {0};
      
      if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
        in.type = INPUT_MOUSE;
        if (vk == VK_LBUTTON) in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        else if (vk == VK_RBUTTON) in.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        else if (vk == VK_MBUTTON) in.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        else if (vk == VK_XBUTTON1) { in.mi.dwFlags = MOUSEEVENTF_XUP; in.mi.mouseData = XBUTTON1; }
        else if (vk == VK_XBUTTON2) { in.mi.dwFlags = MOUSEEVENTF_XUP; in.mi.mouseData = XBUTTON2; }
      } else {
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = 0; // Must be 0 when using KEYEVENTF_SCANCODE
        in.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        in.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
        // Extend arrows etc.
        switch(vk) {
            case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT:
            case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
            case VK_PRIOR: case VK_NEXT: case VK_RCONTROL: case VK_RMENU:
                in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                break;
        }
      }
      SendInput(1, &in, sizeof(INPUT));
    }
    else if (!downBefore && downAfter) {
      // Key was physically pressed while blocked. The system ate the KEYDOWN.
      // We must synthesize it to prevent keys appearing stuck.
      INPUT in = {0};
      
      if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
        in.type = INPUT_MOUSE;
        if (vk == VK_LBUTTON) in.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        else if (vk == VK_RBUTTON) in.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        else if (vk == VK_MBUTTON) in.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        else if (vk == VK_XBUTTON1) { in.mi.dwFlags = MOUSEEVENTF_XDOWN; in.mi.mouseData = XBUTTON1; }
        else if (vk == VK_XBUTTON2) { in.mi.dwFlags = MOUSEEVENTF_XDOWN; in.mi.mouseData = XBUTTON2; }
      } else {
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = 0; // Must be 0 when using KEYEVENTF_SCANCODE
        in.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        in.ki.dwFlags = KEYEVENTF_SCANCODE; // KEYDOWN, not KEYUP
        // Extend arrows etc.
        switch(vk) {
            case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT:
            case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
            case VK_PRIOR: case VK_NEXT: case VK_RCONTROL: case VK_RMENU:
                in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                break;
        }
      }
      SendInput(1, &in, sizeof(INPUT));
    }
  }
}
