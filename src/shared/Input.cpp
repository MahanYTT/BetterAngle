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

// The "Essential 5" - Core Movement Cluster (v5.5.59)
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
  // GUARD: Prevent back-to-back rapid transitions from interrupting a
  // fix sequence that is mid-execution (caused missed fixes when
  // transitions fired ~1 second apart).
  if (g_ghostFixInProgress.exchange(true)) {
    LOG_WARN("GhostFix: Already in progress — skipping duplicate.");
    return;
  }

  LOG_INFO("GhostFix: Starting Shock & Restore sequence...");
  g_wPostUnlock = GetAsyncKeyState('W');

  // SHOCK & RESTORE THEORY (v5.5.62):
  // BlockInput freezes the Windows Async Key State Table. The old delta-compare
  // (pre vs post) was broken because post was read from the frozen table,
  // making it always equal to pre — so KeyUp was never injected and ghost keys
  // persisted.
  //
  // Fix: UNCONDITIONALLY release all keys held before the lock ("Shock"), wait
  // for the table to thaw, then re-press keys still physically held
  // ("Restore"). The thaw wait ensures at least one hardware repeat event (33ms
  // typematic interval) arrives if the key is still physically held, so
  // GetAsyncKeyState is trustworthy after the wait.

  // Store preState for forensics overlay
  for (size_t i = 0; i < preState.size() && i < 4; ++i) {
    g_preState[i] = preState[i];
  }

  std::vector<INPUT> releaseInputs;
  std::string log =
      "Shock&Restore [W:" + std::to_string(g_wPostUnlock.load()) + "]: ";

  extern std::atomic<bool> g_fortniteFocusedCache;

  // Step 1: UNCONDITIONAL SHOCK — Release every key that was held before lock.
  // ALWAYS run the shock — ghost keys must be cleared even when tabbed out,
  // otherwise they persist and the character moves when the user tabs back in.
  // We do NOT read post/phys from the frozen table to decide — we just release.
  for (size_t i = 0; i < preState.size(); ++i) {
    if (preState[i]) {
      int vk = g_gamingKeys[i];

      // WHITELIST CHECK: STRICTLY W, A, S, D, SPACE ONLY
      bool whitelisted = false;
      for (int wvk : g_gamingKeys)
        if (vk == wvk)
          whitelisted = true;
      if (!whitelisted)
        continue;

      UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
      if (scanCode == 0)
        continue;

      INPUT input = {0};
      input.type = INPUT_KEYBOARD;
      input.ki.wVk = (WORD)vk;
      input.ki.wScan = (WORD)scanCode;
      input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
      releaseInputs.push_back(input);
      log += std::to_string(vk) + "↑ ";
    }
  }

  if (!releaseInputs.empty()) {
    SendInput((UINT)releaseInputs.size(), releaseInputs.data(), sizeof(INPUT));
  }

  // Step 2: THAW — Wait for async key state table to settle after Shock.
  // 50ms guarantees at least one keyboard hardware repeat (33ms typematic)
  // arrives if the user is still physically holding the key.
  Sleep(50);

  // Step 3: RESTORE — Re-press keys the user is STILL physically holding.
  // Only check keys we Shocked (preState=true); others need no intervention
  // because their state was never artificially modified.
  // Only restore if Fortnite is focused — the game isn't reading input when
  // tabbed out, so repressing would be wasted and could cause side effects.
  if (g_fortniteFocusedCache.load()) {
    std::vector<INPUT> restoreInputs;
    for (size_t i = 0; i < preState.size(); ++i) {
      if (preState[i]) {
        int vk = g_gamingKeys[i];
        bool stillHeld = (GetAsyncKeyState(vk) & 0x8000) != 0;

        // Update postState for forensics overlay (WASD only, index 0-3)
        if (i < 4)
          g_postState[i] = stillHeld;

        if (stillHeld) {
          UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
          if (scanCode == 0)
            continue;

          INPUT input = {0};
          input.type = INPUT_KEYBOARD;
          input.ki.wVk = (WORD)vk;
          input.ki.wScan = (WORD)scanCode;
          input.ki.dwFlags = KEYEVENTF_SCANCODE; // KeyDown
          restoreInputs.push_back(input);
          log += std::to_string(vk) + "↓ ";
        }
      }
    }

    if (!restoreInputs.empty()) {
      SendInput((UINT)restoreInputs.size(), restoreInputs.data(),
                sizeof(INPUT));
      log += "(Restored)";
    } else {
      log += "(Clean)";
    }
  } else {
    log += "(No Restore - Not Focused)";
  }

  g_tableRefreshed = true;
  g_wPostFlush = GetAsyncKeyState('W');
  g_nitroSyncLog = log;
  LOG_INFO(g_nitroSyncLog.c_str());
  g_hasSynced = true;
  g_ghostFixInProgress = false;

  // Re-register Raw Input for normal app operation
  extern HWND g_hMsgWnd;
  if (g_hMsgWnd)
    RegisterRawMouse(g_hMsgWnd);
}
