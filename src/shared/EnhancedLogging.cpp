#include "shared/EnhancedLogging.h"
#include <comdef.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>
#include <psapi.h>
#include <shlobj.h>
#include <thread>
#include <tlhelp32.h>
#include <vector>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Global log level - default to TRACE for maximum verbosity
LogLevel g_currentLogLevel = LOG_TRACE;

static std::mutex g_logMutex;
static std::ofstream g_logFile;
static bool g_logInitialized = false;

void InitEnhancedLogging() {
  std::lock_guard<std::mutex> lock(g_logMutex);
  if (g_logInitialized)
    return;

  // Get log path
  wchar_t appDataPath[MAX_PATH];
  if (SUCCEEDED(
          SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
    std::wstring logDir =
        std::wstring(appDataPath) + L"\\BetterAngle Pro\\Logs\\";
    CreateDirectoryW(logDir.c_str(), NULL);

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_s(&now_tm, &now_time_t);

    std::wstringstream wss;
    wss << logDir << L"BetterAngle_Debug_" << (now_tm.tm_year + 1900) << L"-"
        << (now_tm.tm_mon + 1) << L"-" << now_tm.tm_mday << L"_"
        << now_tm.tm_hour << L"-" << now_tm.tm_min << L"-" << now_tm.tm_sec
        << L".log";

    g_logFile.open(wss.str(), std::ios::out | std::ios::app);
    if (g_logFile.is_open()) {
      g_logInitialized = true;

      // Write header
      g_logFile << "==========================================" << std::endl;
      g_logFile << "BetterAngle Pro - Enhanced Debug Log" << std::endl;
      g_logFile << "Started: " << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
                << std::endl;
      g_logFile << "Log Level: TRACE (Maximum Verbosity)" << std::endl;
      g_logFile << "==========================================" << std::endl
                << std::endl;

      // Flush immediately
      g_logFile.flush();
    }
  }
}

std::string GetLogLevelString(LogLevel level) {
  switch (level) {
  case LOG_TRACE:
    return "TRACE";
  case LOG_DEBUG:
    return "DEBUG";
  case LOG_INFO:
    return "INFO";
  case LOG_WARN:
    return "WARN";
  case LOG_ERROR:
    return "ERROR";
  case LOG_FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

void LogEnhanced(LogLevel level, const char *file, int line,
                 const std::string &message) {
  if (level < g_currentLogLevel)
    return;

  std::lock_guard<std::mutex> lock(g_logMutex);

  // Ensure logging is initialized
  if (!g_logInitialized) {
    InitEnhancedLogging();
  }

  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm;
  localtime_s(&now_tm, &now_time_t);

  // Extract filename from path
  std::string filename = file;
  size_t lastSlash = filename.find_last_of("\\/");
  if (lastSlash != std::string::npos) {
    filename = filename.substr(lastSlash + 1);
  }

  // Format log entry
  std::stringstream ss;
  ss << std::put_time(&now_tm, "[%Y-%m-%d %H:%M:%S] ") << "["
     << GetLogLevelString(level) << "] "
     << "[" << filename << ":" << line << "] " << message;

  std::string logEntry = ss.str();

  // Write to file
  if (g_logFile.is_open()) {
    g_logFile << logEntry << std::endl;
    g_logFile.flush();
  }

  // Also write to debug output
  OutputDebugStringA((logEntry + "\n").c_str());

  // Write to console if available
  std::cout << logEntry << std::endl;
}

// Backward compatibility
void LogStartup(const std::string &msg) {
  LogEnhanced(LOG_INFO, __FILE__, __LINE__, "[Legacy] " + msg);
}

void LogWindowInfo(HWND hWnd, const std::string &context) {
  if (!hWnd || !IsWindow(hWnd)) {
    LOG_WARN_MSG(context + ": Invalid window handle");
    return;
  }

  char className[256] = {0};
  char windowText[1024] = {0};
  GetClassNameA(hWnd, className, sizeof(className));
  GetWindowTextA(hWnd, windowText, sizeof(windowText));

  RECT rect = {0};
  GetWindowRect(hWnd, &rect);

  LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
  LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

  BOOL isVisible = IsWindowVisible(hWnd);
  BOOL isEnabled = IsWindowEnabled(hWnd);
  HWND foreground = GetForegroundWindow();
  HWND active = GetActiveWindow();

  std::stringstream ss;
  ss << context << " - Window Info:" << std::endl
     << "  Class: " << className << std::endl
     << "  Text: " << windowText << std::endl
     << "  Rect: (" << rect.left << "," << rect.top << ")-(" << rect.right
     << "," << rect.bottom << ")" << std::endl
     << "  Size: " << (rect.right - rect.left) << "x"
     << (rect.bottom - rect.top) << std::endl
     << "  Style: 0x" << std::hex << style << std::dec << std::endl
     << "  ExStyle: 0x" << std::hex << exStyle << std::dec << std::endl
     << "  Visible: " << (isVisible ? "Yes" : "No") << std::endl
     << "  Enabled: " << (isEnabled ? "Yes" : "No") << std::endl
     << "  IsForeground: " << (hWnd == foreground ? "Yes" : "No") << std::endl
     << "  IsActive: " << (hWnd == active ? "Yes" : "No") << std::endl
     << "  WS_EX_TRANSPARENT: "
     << ((exStyle & WS_EX_TRANSPARENT) ? "Yes" : "No") << std::endl
     << "  WS_EX_TOPMOST: " << ((exStyle & WS_EX_TOPMOST) ? "Yes" : "No")
     << std::endl
     << "  WS_EX_NOACTIVATE: " << ((exStyle & WS_EX_NOACTIVATE) ? "Yes" : "No")
     << std::endl
     << "  WS_EX_LAYERED: " << ((exStyle & WS_EX_LAYERED) ? "Yes" : "No")
     << std::endl
     << "  WS_EX_TOOLWINDOW: " << ((exStyle & WS_EX_TOOLWINDOW) ? "Yes" : "No");

  LOG_DEBUG_MSG(ss.str());
}

void LogQMLState(const std::string &component, const std::string &state) {
  std::stringstream ss;
  ss << "QML State - " << component << ": " << state;
  LOG_TRACE_MSG(ss.str());
}

void LogMemoryUsage(const std::string &context) {
  PROCESS_MEMORY_COUNTERS_EX pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc,
                           sizeof(pmc))) {
    std::stringstream ss;
    ss << context << " - Memory Usage:" << std::endl
       << "  WorkingSet: " << (pmc.WorkingSetSize / 1024 / 1024) << " MB"
       << std::endl
       << "  PeakWorkingSet: " << (pmc.PeakWorkingSetSize / 1024 / 1024)
       << " MB" << std::endl
       << "  Pagefile: " << (pmc.PagefileUsage / 1024 / 1024) << " MB"
       << std::endl
       << "  PeakPagefile: " << (pmc.PeakPagefileUsage / 1024 / 1024) << " MB"
       << std::endl
       << "  PrivateUsage: " << (pmc.PrivateUsage / 1024 / 1024) << " MB";
    LOG_DEBUG_MSG(ss.str());
  }
}

void LogSystemInfo() {
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);

  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);
  GlobalMemoryStatusEx(&memStatus);

  std::stringstream ss;
  ss << "System Info:" << std::endl
     << "  CPU Cores: " << sysInfo.dwNumberOfProcessors << std::endl
     << "  Page Size: " << (sysInfo.dwPageSize / 1024) << " KB" << std::endl
     << "  Total RAM: " << (memStatus.ullTotalPhys / 1024 / 1024 / 1024)
     << " GB" << std::endl
     << "  Available RAM: " << (memStatus.ullAvailPhys / 1024 / 1024 / 1024)
     << " GB" << std::endl
     << "  Memory Load: " << (int)memStatus.dwMemoryLoad << "%";

  LOG_INFO_MSG(ss.str());
}

void LogThreadInfo(const std::string &context) {
  DWORD currentThreadId = GetCurrentThreadId();
  DWORD currentProcessId = GetCurrentProcessId();

  std::stringstream ss;
  ss << context << " - Thread Info:" << std::endl
     << "  Process ID: " << currentProcessId << std::endl
     << "  Thread ID: " << currentThreadId << std::endl
     << "  Hardware Concurrency: " << std::thread::hardware_concurrency();

  LOG_DEBUG_MSG(ss.str());
}

void LogRegistryOperation(const std::string &operation, const std::string &key,
                          bool success) {
  std::stringstream ss;
  ss << "Registry " << operation << " - Key: " << key
     << " - Result: " << (success ? "SUCCESS" : "FAILED");
  if (success) {
    LOG_DEBUG_MSG(ss.str());
  } else {
    LOG_ERROR_MSG(ss.str());
  }
}

void LogFileOperation(const std::string &operation, const std::string &path,
                      bool success) {
  std::stringstream ss;
  ss << "File " << operation << " - Path: " << path
     << " - Result: " << (success ? "SUCCESS" : "FAILED");
  if (success) {
    LOG_DEBUG_MSG(ss.str());
  } else {
    LOG_ERROR_MSG(ss.str());
  }
}

void LogCOMOperation(const std::string &operation, HRESULT hr) {
  std::stringstream ss;
  ss << "COM Operation: " << operation << " - HRESULT: 0x" << std::hex << hr
     << std::dec;
  if (SUCCEEDED(hr)) {
    LOG_DEBUG_MSG(ss.str());
  } else {
    _com_error err(hr);
    ss << " - Error: " << err.ErrorMessage();
    LOG_ERROR_MSG(ss.str());
  }
}

void LogGDIResource(const std::string &context, int count) {
  std::stringstream ss;
  ss << context << " - GDI Resources: " << count;
  LOG_TRACE_MSG(ss.str());
}

void LogDirectXState(const std::string &context, HRESULT hr) {
  std::stringstream ss;
  ss << "DirectX State - " << context;
  if (hr != S_OK) {
    ss << " - HRESULT: 0x" << std::hex << hr << std::dec;
    _com_error err(hr);
    ss << " - Error: " << err.ErrorMessage();
    LOG_ERROR_MSG(ss.str());
  } else {
    LOG_DEBUG_MSG(ss.str());
  }
}

void LogWindowDragEvent(const std::string &context, int x, int y,
                        bool started) {
  std::stringstream ss;
  ss << "Window Drag Event - " << context;
  ss << " - " << (started ? "STARTED" : "ENDED");
  ss << " at (" << x << ", " << y << ")";
  LOG_TRACE_MSG(ss.str());
}