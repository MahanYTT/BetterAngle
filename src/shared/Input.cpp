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
  static HWND s_lastFg = NULL;
  static bool s_lastResult = false;

  HWND fg = GetForegroundWindow();
  if (!fg) {
    s_lastFg = NULL;
    return false;
  }

  if (fg == s_lastFg) {
    return s_lastResult;
  }

  s_lastFg = fg;
  s_lastResult = false;

  DWORD pid = 0;
  GetWindowThreadProcessId(fg, &pid);
  if (pid == 0) {
    return false;
  }

  // Method 1: Try OpenProcess + QueryFullProcessImageNameW (Fastest)
  wchar_t processPath[MAX_PATH] = {};
  const wchar_t *processName = GetProcessBaseName(fg, processPath, MAX_PATH);
  if (processName && processName[0] && IsFortniteProcessName(processName)) {
    s_lastResult = true;
    return true;
  }

  // Method 2: Fallback using CreateToolhelp32Snapshot (Slowest, only if needed)
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
  // Since we called ReleaseHeldKeys() before the block, we MUST re-synthesize
  // KEYDOWN for everything that the user is still physically holding down.
  
  std::vector<INPUT> inputs;
  inputs.reserve(32);

  for (int vk = 1; vk < 255; vk++) {
    bool downAfter = (GetAsyncKeyState(vk) & 0x8000) != 0;

    if (downAfter) {
      // Key is physically held. Re-synthesize the press.
      INPUT in = {0};
      
      if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
        in.type = INPUT_MOUSE;
        if      (vk == VK_LBUTTON)  in.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        else if (vk == VK_RBUTTON)  in.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        else if (vk == VK_MBUTTON)  in.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        else if (vk == VK_XBUTTON1) { in.mi.dwFlags = MOUSEEVENTF_XDOWN; in.mi.mouseData = XBUTTON1; }
        else if (vk == VK_XBUTTON2) { in.mi.dwFlags = MOUSEEVENTF_XDOWN; in.mi.mouseData = XBUTTON2; }
      } else {
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = (WORD)vk;
        in.ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        in.ki.dwFlags = KEYEVENTF_SCANCODE; // KEYDOWN
        switch(vk) {
            case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT:
            case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
            case VK_PRIOR: case VK_NEXT: case VK_RCONTROL: case VK_RMENU:
                in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                break;
        }
      }
      inputs.push_back(in);
    }
  }

  if (!inputs.empty()) {
    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
  }
}

void ReleaseHeldKeys() {
  // Synthesize KEYUP / mouse-button-UP for every key that is physically held down
  // RIGHT NOW. Must be called immediately before BlockInput(TRUE) so the game
  // receives release events and stops the character from ghosting/sliding.
  // All inputs are batched into a single SendInput call to minimise latency.
  std::vector<INPUT> inputs;
  inputs.reserve(32);

  for (int vk = 1; vk < 255; vk++) {
    if (!(GetAsyncKeyState(vk) & 0x8000))
      continue; // Not held

    INPUT in = {};

    if (vk == VK_LBUTTON || vk == VK_RBUTTON ||
        vk == VK_MBUTTON  || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
      in.type = INPUT_MOUSE;
      if      (vk == VK_LBUTTON)  in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
      else if (vk == VK_RBUTTON)  in.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
      else if (vk == VK_MBUTTON)  in.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
      else if (vk == VK_XBUTTON1) { in.mi.dwFlags = MOUSEEVENTF_XUP; in.mi.mouseData = XBUTTON1; }
      else if (vk == VK_XBUTTON2) { in.mi.dwFlags = MOUSEEVENTF_XUP; in.mi.mouseData = XBUTTON2; }
    } else {
      in.type       = INPUT_KEYBOARD;
      in.ki.wVk     = (WORD)vk;
      in.ki.wScan   = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      in.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
      switch (vk) {
        case VK_UP:    case VK_DOWN:  case VK_LEFT:  case VK_RIGHT:
        case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
        case VK_PRIOR: case VK_NEXT:  case VK_RCONTROL: case VK_RMENU:
          in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
          break;
      }
    }
    inputs.push_back(in);
  }

  if (!inputs.empty())
    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
}
void SyncMovementKeys() {
  // Movement Cluster: WASD, Space, Shift, Ctrl, Arrows
  static const int movementKeys[] = {
    'W', 'A', 'S', 'D', 
    VK_SPACE, VK_LSHIFT, VK_LCONTROL, 
    VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT 
  };

  std::vector<INPUT> inputs;
  inputs.reserve(std::size(movementKeys) * 2);

  for (int vk : movementKeys) {
    bool physicallyDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
    
    INPUT in = {0};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = (WORD)vk;
    in.ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    
    if (physicallyDown) {
      // Re-press if still held
      in.ki.dwFlags = KEYEVENTF_SCANCODE;
    } else {
      // Release if let go during lock
      in.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    }

    // Extended key bit for arrows
    if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT) {
      in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    inputs.push_back(in);
  }

  if (!inputs.empty()) {
    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
  }
}
