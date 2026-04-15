#include "shared/Input.h"
#include <cwchar>
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
  wchar_t processPath[MAX_PATH] = {};
  const wchar_t *processName =
      GetProcessBaseName(GetForegroundWindow(), processPath, MAX_PATH);
  return IsFortniteProcessName(processName);
}

bool IsCursorCurrentlyVisible() {
  CURSORINFO cursorInfo = {sizeof(CURSORINFO)};
  if (!GetCursorInfo(&cursorInfo))
    return true;
  return (cursorInfo.flags & CURSOR_SHOWING) != 0;
}

void SyncKeyStates(const std::vector<int> &preBlockKeys) {
  // Sync physical hardware state with logical state after a BlockInput window.
  // Re-evaluates all 255 virtual keys.

  // First, wait a bit to ensure the system has processed the BlockInput release
  Sleep(10);

  for (int vk = 1; vk < 255; vk++) {
    bool downBefore = false;
    for (int pre_vk : preBlockKeys) {
      if (pre_vk == vk) {
        downBefore = true;
        break;
      }
    }

    // Get the current physical key state
    bool downAfter = (GetAsyncKeyState(vk) & 0x8000) != 0;

    // Also check with GetKeyState to see if the key is toggled (for caps lock,
    // num lock, etc.) But for regular keys, we care about the high bit (0x8000)
    // which indicates physical press

    if (downBefore && !downAfter) {
      // Key was physically released while blocked. The system ate the KEYUP.
      // We must synthesize it to prevent game character ghosting.
      INPUT in = {0};

      if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
          vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
        in.type = INPUT_MOUSE;
        if (vk == VK_LBUTTON)
          in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        else if (vk == VK_RBUTTON)
          in.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        else if (vk == VK_MBUTTON)
          in.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        else if (vk == VK_XBUTTON1) {
          in.mi.dwFlags = MOUSEEVENTF_XUP;
          in.mi.mouseData = XBUTTON1;
        } else if (vk == VK_XBUTTON2) {
          in.mi.dwFlags = MOUSEEVENTF_XUP;
          in.mi.mouseData = XBUTTON2;
        }
      } else {
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = vk;
        in.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        in.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
        // Extend arrows etc.
        switch (vk) {
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_RCONTROL:
        case VK_RMENU:
          in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
          break;
        }
      }
      SendInput(1, &in, sizeof(INPUT));
      // Small delay between synthetic events to avoid overwhelming the input
      // system
      Sleep(1);
    } else if (!downBefore && downAfter) {
      // Key was physically pressed during or after blocking.
      // This could be a rapid tap that happened right at the boundary.
      // We should synthesize a KEYDOWN followed by KEYUP to ensure the game
      // sees it. But only for rapid taps (keys that are already released by the
      // time we check). Actually, let's not do this automatically as it might
      // cause double-presses. Instead, we'll rely on the user pressing the key
      // again.
    }
  }

  // Additional pass: check for keys that might be stuck due to timing issues
  // Some keyboards (like SteelSeries Apex Pro) might have different timing
  Sleep(5);

  // Final verification pass
  for (int vk = 1; vk < 255; vk++) {
    bool downBefore = false;
    for (int pre_vk : preBlockKeys) {
      if (pre_vk == vk) {
        downBefore = true;
        break;
      }
    }

    bool downAfter = (GetAsyncKeyState(vk) & 0x8000) != 0;

    // If a key was pressed before and is still pressed now, but the game might
    // think it's released due to missing KEYDOWN, we should ensure it's in the
    // correct state. This is complex and game-dependent, so we'll leave it as
    // is for now.
  }
}
