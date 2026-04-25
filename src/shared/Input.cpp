#include "shared/Input.h"
#include "shared/EnhancedLogging.h"
#include "shared/State.h"
#include <cwchar>
#include <fstream>
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
static const int g_gamingKeys[] = {'W', 'A', 'S', 'D'};

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
      g_physicalKeys[VK_LBUTTON].store(
          (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0,
          std::memory_order_relaxed);
      Sleep(1);
    }
    timeEndPeriod(1);
  }).detach();

  LOG_INFO("High-Performance Polling Thread started (TRUE 1ms Hardware Scan)");
}

void RegisterRawMouse(HWND hwnd) {
  RAWINPUTDEVICE rid[2];

  // Mouse
  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x02;
  rid[0].dwFlags = RIDEV_INPUTSINK;
  rid[0].hwndTarget = hwnd;

  // Keyboard
  rid[1].usUsagePage = 0x01;
  rid[1].usUsage = 0x06;
  rid[1].dwFlags = RIDEV_INPUTSINK;
  rid[1].hwndTarget = hwnd;

  if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
    LOG_ERROR("Failed to register raw input devices (Mouse+Keyboard).");
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
  g_wPostUnlock = GetAsyncKeyState('W');
  g_activeFallback = 0;

  // NEW APPROACH: Wait 50ms for natural table thaw after BlockInput
  // Windows often recovers key state automatically after focus returns
  Sleep(50);

  // Read state after 50ms wait
  std::vector<bool> stateAfterWait;
  bool tableThawed = true;
  for (size_t i = 0; i < std::size(g_gamingKeys); ++i) {
    bool current = (GetAsyncKeyState(g_gamingKeys[i]) & 0x8000) != 0;
    stateAfterWait.push_back(current);

    // Check if this key's state matches what we expect
    // If key was held before lock but is physically released now, it should
    // show 0 If it still shows 1, the table might still be frozen
    if (preState[i] && !current) {
      // Good: table shows key as released (thawed)
    } else if (preState[i] && current) {
      // Key was held and still shows held - could be correct or frozen
      // We'll check later with fallbacks
    }
  }

  // If table appears thawed (no obvious frozen states), try simple injection
  bool simpleInjectionNeeded = false;
  std::vector<INPUT> simpleInputs;
  for (size_t i = 0; i < preState.size(); ++i) {
    if (preState[i] && !stateAfterWait[i]) {
      // Key was held before lock but is now released - inject KeyUp
      int vk = g_gamingKeys[i];
      INPUT input = {0};
      input.type = INPUT_KEYBOARD;
      input.ki.wVk = (WORD)vk;
      input.ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
      simpleInputs.push_back(input);
      simpleInjectionNeeded = true;
    }
  }

  if (simpleInjectionNeeded && !simpleInputs.empty()) {
    SendInput((UINT)simpleInputs.size(), simpleInputs.data(), sizeof(INPUT));
    g_activeFallback = 4; // 50ms wait approach
    g_tableRefreshed = true;

    // Update finalState to reflect injection
    for (size_t i = 0; i < preState.size(); ++i) {
      if (preState[i] && !stateAfterWait[i]) {
        stateAfterWait[i] = false; // Key is now released after injection
      }
    }

    // Skip further fallbacks if successful
    g_wPostFlush = GetAsyncKeyState('W');
    for (int i = 0; i < 4; ++i) {
      g_preState[i] = preState[i];
      g_postState[i] = stateAfterWait[i];
    }
    LOG_INFO("[GhostFix] FB=4 50ms-wait | Table thawed naturally");
    return;
  }

  // If 50ms wait didn't work, proceed with original fallbacks
  // FALLBACK 1: Scancode Flush (Shock) - Exclude VK_SPACE to prevent FOV
  // transition loops
  for (int vk : g_gamingKeys) {
    if (vk == VK_SPACE)
      continue; // Skip spacebar to avoid triggering FOV transitions
    INPUT flush[2] = {0};
    flush[0].type = INPUT_KEYBOARD;
    flush[0].ki.wVk = (WORD)vk;
    flush[0].ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    flush[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    flush[1] = flush[0];
    flush[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(2, flush, sizeof(INPUT));
  }
  Sleep(10);

  // Read State after FB1
  std::vector<bool> state1;
  bool refreshed1 = false;
  for (size_t i = 0; i < std::size(g_gamingKeys); ++i) {
    bool current = (GetAsyncKeyState(g_gamingKeys[i]) & 0x8000) != 0;
    state1.push_back(current);
    if (preState[i] != current)
      refreshed1 = true;
  }
  if (refreshed1)
    g_activeFallback = 1;

  // FALLBACK 2: Unconditional KeyUp + Repress (The "Hammer")
  // Only repress keys that are confirmed still HELD (state1[i] == true)
  // Skip VK_SPACE to avoid triggering FOV transitions (same as FB1)
  bool refreshed2 = false;
  if (g_lockTriggerReason != 3) {
    for (size_t i = 0; i < std::size(g_gamingKeys); ++i) {
      int vk = g_gamingKeys[i];
      if (vk == VK_SPACE)
        continue; // Skip spacebar to avoid triggering FOV transitions
      if (preState[i] && state1[i]) {
        INPUT seq[2] = {0};
        seq[0].type = INPUT_KEYBOARD;
        seq[0].ki.wVk = (WORD)vk;
        seq[0].ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        seq[0].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // Force Up
        seq[1] = seq[0];
        seq[1].ki.dwFlags = KEYEVENTF_SCANCODE; // Immediate Down
        SendInput(2, seq, sizeof(INPUT));
        Sleep(1);
      }
    }
    Sleep(5);
  }

  // Read Final State after FB2
  std::vector<bool> finalState;
  for (size_t i = 0; i < std::size(g_gamingKeys); ++i) {
    bool current = (GetAsyncKeyState(g_gamingKeys[i]) & 0x8000) != 0;
    finalState.push_back(current);
    if (!refreshed1 && preState[i] != current)
      refreshed2 = true;
  }
  if (refreshed2)
    g_activeFallback = 2;

  g_tableRefreshed = refreshed1 || refreshed2;
  g_wPostFlush = GetAsyncKeyState('W');

  for (int i = 0; i < 4; ++i) {
    g_preState[i] = preState[i];
    g_postState[i] = finalState[i];
  }

  // FINAL DELTA INJECT
  std::vector<INPUT> outInputs;
  std::string log = "[GhostFix] FB=" + std::to_string(g_activeFallback.load()) +
                    " " + (g_tableRefreshed ? "REFRESHED" : "FROZEN") + " | ";

  for (size_t i = 0; i < preState.size(); ++i) {
    int vk = g_gamingKeys[i];
    // HARDWARE TRUTH: Key is released if finalState says so OR if we captured a
    // Raw KeyUp during the lock
    bool isReleased = !finalState[i] || g_rawKeyUpDetected[vk];

    if (preState[i] && isReleased) {
      INPUT input = {0};
      input.type = INPUT_KEYBOARD;
      input.ki.wVk = (WORD)vk;
      input.ki.wScan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
      outInputs.push_back(input);
      log += std::to_string(vk) + "↑ ";
    }
  }

  // Enhanced diagnostic logging for ghost key failures (Consolidated to Master Log)
  if (!g_tableRefreshed && !outInputs.empty() && g_activeFallback.load() != 3) {
    std::string forensic = "[GHOST_FAIL] FB=" + std::to_string(g_activeFallback.load()) +
                           " Table=FROZEN Keys=[";
    for (size_t i = 0; i < std::size(g_gamingKeys); ++i) {
      int vk = g_gamingKeys[i];
      forensic += (i > 0 ? "," : "") + std::string(1, (char)vk) + ":" + 
                  (preState[i] ? "1" : "0") + "->" + (finalState[i] ? "1" : "0") +
                  (g_rawKeyUpDetected[vk] ? "(R)" : "");
    }
    forensic += "] Wpre=" + std::to_string(g_wPreLock) + 
                " Wpost=" + std::to_string(g_wPostUnlock) +
                " Wflush=" + std::to_string(g_wPostFlush);
    LOG_ERROR(forensic.c_str());
  }

  // Final state sync completed

  if (!outInputs.empty()) {
    // Try primary injection with SendInput
    SendInput((UINT)outInputs.size(), outInputs.data(), sizeof(INPUT));

    // Check if injection succeeded by verifying key states after a short delay
    Sleep(1); // Optimized from 5ms for faster recovery
    bool anyStillHeld = false;
    for (size_t i = 0; i < preState.size(); ++i) {
      if (preState[i] && !finalState[i]) {
        // This key should have been released
        bool current = (GetAsyncKeyState(g_gamingKeys[i]) & 0x8000) != 0;
        if (current) {
          anyStillHeld = true;
          break;
        }
      }
    }

    if (anyStillHeld) {
      // SendInput failed, try keybd_event fallback
      g_activeFallback = 3; // Mark as using keybd_event fallback
      // Log the fallback usage immediately (Consolidated to Master Log)
      std::string fbLog = "[GHOST_FALLBACK] Using keybd_event for: ";
      for (size_t i = 0; i < std::size(g_gamingKeys); ++i) {
        if (preState[i] && !finalState[i]) {
          fbLog += std::string(1, (char)g_gamingKeys[i]) + " ";
        }
      }
      LOG_WARN(fbLog.c_str());

      for (size_t i = 0; i < preState.size(); ++i) {
        if (preState[i] && !finalState[i]) {
          int vk = g_gamingKeys[i];
          // Use keybd_event as alternative to SendInput
          keybd_event((BYTE)vk, 0, KEYEVENTF_KEYUP, 0);
          Sleep(1);
        }
      }
      g_nitroSyncLog = log + "(Injected via keybd_event)";
    } else {
      g_nitroSyncLog = log + "(Injected via SendInput)";
      // If we had a ghost key failure condition but SendInput succeeded,
      // mark as failure (99) for diagnostics
      if (!g_tableRefreshed && !outInputs.empty() &&
          g_activeFallback.load() != 3) {
        g_activeFallback = 99;
      }
    }

    LOG_INFO(g_nitroSyncLog.c_str());
  } else {
    g_nitroSyncLog = log + "(Clean)";
  }

  g_hasSynced = true;

  extern HWND g_hMsgWnd;
  if (g_hMsgWnd) {
    RegisterRawMouse(g_hMsgWnd);
  }
}
