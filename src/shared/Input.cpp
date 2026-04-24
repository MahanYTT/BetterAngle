#include "shared/Input.h"
#include "shared/EnhancedLogging.h"
#include "shared/State.h"
#include <cwchar>
#include <string>
#include <thread>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>


extern std::string g_nitroSyncLog;

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

#include "shared/EnhancedLogging.h"

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

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

static bool g_pollingRunning = false;

// The "Nitro 5" - Absolute Movement Cluster (v5.5.0)
static const int g_gamingKeys[] = {'W', 'A', 'S', 'D', VK_SPACE};

// Physical Truth Table (v5.1.16)
// Using std::atomic<bool> g_physicalKeys[256] from State.h

void StartPollingThread() {
  std::thread([]() {
    timeBeginPeriod(1); // Force 1ms Windows resolution
    while (g_running) {
      for (int vk : g_gamingKeys) {
        g_physicalKeys[vk].store((GetAsyncKeyState(vk) & 0x8000) != 0,
                                 std::memory_order_relaxed);
      }
      // Also poll LBUTTON for HUD dragging (fixes legacy bug)
      g_physicalKeys[VK_LBUTTON].store((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0,
                                 std::memory_order_relaxed);
      Sleep(1);
    }
    timeEndPeriod(1);
  }).detach();

  LOG_INFO("High-Performance Polling Thread started (TRUE 1ms Hardware Scan)");
}

void RegisterRawMouse(HWND hwnd) {
  RAWINPUTDEVICE rid;
  rid.usUsagePage = 0x01; // Generic Desktop
  rid.usUsage = 0x02;     // Mouse
  rid.dwFlags = RIDEV_INPUTSINK;
  rid.hwndTarget = hwnd;

  if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
    LOG_ERROR("Failed to register raw mouse input device.");
  }
}

int GetRawInputDeltaX(LPARAM lparam) {
  UINT dwSize;
  GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dwSize,
                  sizeof(RAWINPUTHEADER));
  if (dwSize == 0)
    return 0;

  std::vector<BYTE> lpb(dwSize);
  if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb.data(), &dwSize,
                      sizeof(RAWINPUTHEADER)) != dwSize)
    return 0;

  RAWINPUT *raw = (RAWINPUT *)lpb.data();
  if (raw->header.dwType == RIM_TYPEMOUSE) {
    return raw->data.mouse.lLastX;
  }
  return 0;
}

std::vector<bool> GetGamingKeyState() {
  std::vector<bool> state;
  state.reserve(std::size(g_gamingKeys));
  for (int vk : g_gamingKeys) {
    // FRESH SCAN: Ensure snapshot is absolute latest
    state.push_back((GetAsyncKeyState(vk) & 0x8000) != 0);
  }
  return state;
}

void SyncGamingKeysNitro(const std::vector<bool> &preState) {
  // Capture diagnostic pre-flush state
  short wBefore = GetAsyncKeyState('W');
  g_wPostUnlock = wBefore;

  // NITRO FLUSH THEORY:
  // Unregistering and reregistering Raw Input forces Windows to flush and
  // rebuild the async key state table from actual hardware state.
  
  // Step 1: Unregister Raw Input to force table rebuild
  RAWINPUTDEVICE rid = {0};
  rid.usUsagePage = 0x01;
  rid.usUsage = 0x06; // Keyboard
  rid.dwFlags = RIDEV_REMOVE;
  rid.hwndTarget = NULL;
  RegisterRawInputDevices(&rid, 1, sizeof(rid));

  // Step 2: Small wait for table rebuild
  Sleep(2);

  // Step 3: Read fresh post-snapshot (Table should be rebuilt now)
  std::vector<bool> postState;
  for (int vk : g_gamingKeys) {
    postState.push_back((GetAsyncKeyState(vk) & 0x8000) != 0);
  }
  
  short wAfter = GetAsyncKeyState('W');
  g_wPostFlush = wAfter;

  for (int i = 0; i < 5; ++i) {
    g_preState[i] = preState[i];
    g_postState[i] = postState[i];
  }

  // Step 4: Delta compare and inject KeyUp only for changed keys
  std::vector<INPUT> outInputs;
  std::string log = "Nitro Flush [W: " + std::to_string(wBefore) + "->" + std::to_string(wAfter) + "]: ";
  bool restored = false;

  for (size_t i = 0; i < preState.size(); ++i) {
    // If key was held BEFORE lock, but is NOT held now
    if (preState[i] && !postState[i]) {
      int vk = g_gamingKeys[i];
      restored = true;

      INPUT input = {0};
      input.type = INPUT_KEYBOARD;
      input.ki.wVk = (WORD)vk;
      input.ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
      outInputs.push_back(input);

      log += std::to_string(vk) + "↑ ";
    }
  }

  if (restored) {
    SendInput((UINT)outInputs.size(), outInputs.data(), sizeof(INPUT));
    g_nitroSyncLog = log + "(Injected)";
    LOG_INFO(g_nitroSyncLog.c_str());
  } else {
    g_nitroSyncLog = log + "(Clean)";
  }

  // Step 5: Reregister Raw Input immediately after
  // Note: BetterAngle.cpp calls RegisterRawMouse(hMsgWnd) which is what we need
  // We'll rely on the caller or a global handle to reregister
  extern HWND g_hMsgWnd; 
  if (g_hMsgWnd) {
    RegisterRawMouse(g_hMsgWnd);
  }
}
