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

static HHOOK g_hMouseHook = NULL;
static bool g_mouseLocked = false;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION && g_mouseLocked) {
    if (wParam == WM_MOUSEMOVE || wParam == WM_NCMOUSEMOVE) {
      // Block mouse movement only
      return 1;
    }
  }
  return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void EnableMouseLock(bool enable) {
  g_mouseLocked = enable;
  if (enable && g_hMouseHook == NULL) {
    g_hMouseHook =
        SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
  } else if (!enable && g_hMouseHook != NULL) {
    UnhookWindowsHookEx(g_hMouseHook);
    g_hMouseHook = NULL;
  }
}

bool IsMouseLocked() { return g_mouseLocked; }

void SyncKeyStates(const std::vector<int> &keysToRestore) {
  // For each key that was pressed before blocking,
  // refresh its state to prevent ghosting
  for (int vk : keysToRestore) {
    // Check current physical state
    SHORT currentState = GetAsyncKeyState(vk);
    bool currentlyPressed = (currentState & 0x8000) != 0;

    if (currentlyPressed) {
      // Key is still physically pressed. Some games may lose track of
      // held keys during input blocking. Send a brief key-up then key-down
      // to "refresh" the key state without actually releasing it.
      INPUT inputUp = {0};
      inputUp.type = INPUT_KEYBOARD;
      inputUp.ki.wVk = static_cast<WORD>(vk);
      inputUp.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      inputUp.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;

      INPUT inputDown = {0};
      inputDown.type = INPUT_KEYBOARD;
      inputDown.ki.wVk = static_cast<WORD>(vk);
      inputDown.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      inputDown.ki.dwFlags = KEYEVENTF_SCANCODE;

      // Send key-up then key-down very quickly
      // This tells the game the key was released and re-pressed,
      // which should clear any stuck state while maintaining the
      // perception of continuous hold (the gap is <5ms)
      SendInput(1, &inputUp, sizeof(INPUT));
      Sleep(2); // Very short delay
      SendInput(1, &inputDown, sizeof(INPUT));
    } else {
      // Key is no longer physically pressed but might be stuck
      // in pressed state in the game. Send a key-up to ensure it's released.
      INPUT inputUp = {0};
      inputUp.type = INPUT_KEYBOARD;
      inputUp.ki.wVk = static_cast<WORD>(vk);
      inputUp.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      inputUp.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
      SendInput(1, &inputUp, sizeof(INPUT));
    }

    // Tiny delay between keys to avoid overwhelming the input system
    Sleep(1);
  }
}
