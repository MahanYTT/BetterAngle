#include "shared/Input.h"
#include "shared/State.h"
#include "shared/EnhancedLogging.h"
#include <cwchar>
#include <tlhelp32.h>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>

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

// The "Iron-Tight 8" - Absolute Movement Cluster (v5.1.19)
static const int g_gamingKeys[] = {
    'W', 'A', 'S', 'D', 
    VK_SPACE, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL
};

// Physical Truth Table (v5.1.16)
// Using std::atomic<bool> g_physicalKeys[256] from State.h

void StartPollingThread() {
  std::thread([]() {
    timeBeginPeriod(1); // Force 1ms Windows resolution
    while (g_running) {
      for (int vk : g_gamingKeys) {
        g_physicalKeys[vk].store((GetAsyncKeyState(vk) & 0x8000) != 0, std::memory_order_relaxed);
      }
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

void SyncGamingKeysNitro(const std::vector<bool>& initialState) {
  // Capture final diagnostic state (for HUD reference only)
  g_wPostUnlock = GetAsyncKeyState('W');

  // ABSOLUTE RESTORATION (v5.2.0)
  // We no longer probe hardware state post-lock because the table is frozen.
  // Instead, we UNCONDITIONALLY force-release every key that was held in the pre-lock snapshot.
  // This guarantees that any key released during the lock is caught, while keys still physically 
  // held will simply trigger a new DOWN event immediately when the user continues moving.

  std::vector<INPUT> outInputs;
  std::string log = "v5.2 Restore: ";
  bool restored = false;

  for (size_t i = 0; i < initialState.size(); ++i) {
    if (initialState[i]) { // Key was held BEFORE lock
      int vk = g_gamingKeys[i];
      restored = true;

      INPUT input = {0};
      input.type = INPUT_KEYBOARD;
      input.ki.wVk = (WORD)vk;
      input.ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
      outInputs.push_back(input);
      
      log += std::to_string(vk) + " ";
    }
  }

  if (restored) {
    SendInput((UINT)outInputs.size(), outInputs.data(), sizeof(INPUT));
    g_nitroSyncLog = log + "FORCE-RELEASED";
    LOG_INFO(g_nitroSyncLog.c_str());
  } else {
    g_nitroSyncLog = "Sync: Clean (No keys were held)";
  }

  // Diagnostic: Final check after force-restoration
  // Note: GetAsyncKeyState might STILL be stale here until a physical event occurs
  g_wPostFlush = GetAsyncKeyState('W');
}
