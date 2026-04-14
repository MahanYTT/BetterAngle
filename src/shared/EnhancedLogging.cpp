#include "shared/EnhancedLogging.h"

#include "shared/State.h"

#include <shlobj.h>
#include <windows.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

std::atomic<LogLevel> g_logLevel(LogLevel::INFO);
std::atomic<bool> g_logToFile(true);
std::atomic<bool> g_logToConsole(false);
std::wstring g_logFilePath;
std::mutex g_logMutex;
std::unique_ptr<std::ofstream> g_logFile;

namespace {

std::string Narrow(const std::wstring &value) {
  if (value.empty())
    return {};

  int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0,
                                 nullptr, nullptr);
  if (size <= 1)
    return {};

  std::string out(static_cast<size_t>(size - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, out.data(), size, nullptr,
                      nullptr);
  return out;
}

std::string BaseName(const char *file) {
  if (!file)
    return "unknown";

  std::string path(file);
  const size_t slash = path.find_last_of("\\/");
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::wstring GetExecutableDirectory() {
  wchar_t modulePath[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
  if (len == 0 || len >= MAX_PATH)
    return L"";

  std::wstring path(modulePath, len);
  const size_t slash = path.find_last_of(L"\\/");
  if (slash == std::wstring::npos)
    return L"";
  return path.substr(0, slash + 1);
}

bool PathExists(const std::wstring &path) {
  const DWORD attrs = GetFileAttributesW(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES;
}

void WriteLogLineUnlocked(const std::string &line) {
  if (g_logToConsole.load()) {
    std::cout << line << std::endl;
  }

  if (!g_logToFile.load())
    return;

  if (g_logFilePath.empty()) {
    g_logFilePath = GetLogFilePath();
  }

  if (g_logFilePath.empty())
    return;

  if (!g_logFile || !g_logFile->is_open()) {
    fs::path path(g_logFilePath);
    g_logFile =
        std::make_unique<std::ofstream>(path, std::ios::out | std::ios::app);
  }

  if (g_logFile && g_logFile->is_open()) {
    (*g_logFile) << line << std::endl;
    g_logFile->flush();
  }
}

void WriteLogLine(const std::string &line) {
  std::lock_guard<std::mutex> lock(g_logMutex);
  WriteLogLineUnlocked(line);
}

void LogWithPrefix(LogLevel level, const char *file, int line,
                   const std::string &prefix, const std::string &message) {
  if (static_cast<int>(level) < static_cast<int>(g_logLevel.load()))
    return;

  std::ostringstream oss;
  oss << '[' << GetCurrentTimeString() << "] [" << LogLevelToString(level)
      << "] [" << BaseName(file) << ':' << line << "] " << prefix;

  if (!message.empty())
    oss << message;

  WriteLogLine(oss.str());
}

std::string VFormatOptional(const char *format, va_list args) {
  if (!format || format[0] == '\0')
    return {};
  return VFormatString(format, args);
}

} // namespace

std::string VFormatString(const char *format, va_list args) {
  if (!format)
    return {};

  va_list argsCopy;
  va_copy(argsCopy, args);
  const int required = vsnprintf(nullptr, 0, format, argsCopy);
  va_end(argsCopy);

  if (required <= 0)
    return {};

  std::vector<char> buffer(static_cast<size_t>(required) + 1);
  vsnprintf(buffer.data(), buffer.size(), format, args);
  return std::string(buffer.data());
}

std::string FormatString(const char *format, ...) {
  va_list args;
  va_start(args, format);
  std::string result = VFormatString(format, args);
  va_end(args);
  return result;
}

std::string GetCurrentTimeString() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

  std::tm tmValue{};
#if defined(_WIN32)
  localtime_s(&tmValue, &time);
#else
  tmValue = *std::localtime(&time);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tmValue, "%Y-%m-%d %H:%M:%S") << '.'
      << std::setfill('0') << std::setw(3) << ms.count();
  return oss.str();
}

std::string LogLevelToString(LogLevel level) {
  switch (level) {
  case LogLevel::TRACE:
    return "TRACE";
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARN:
    return "WARN";
  case LogLevel::ERROR:
    return "ERROR";
  case LogLevel::FATAL:
    return "FATAL";
  }
  return "UNKNOWN";
}

std::wstring GetDebugFolderPath() {
  const std::wstring exeDir = GetExecutableDirectory();
  if (!exeDir.empty() && PathExists(exeDir + L"portable.flag")) {
    return exeDir + L"debug\\";
  }

  std::wstring root = GetAppRootPath();
  if (root.empty())
    return L"";
  return root + L"debug\\";
}

bool EnsureDebugFolderExists() {
  const std::wstring debugPath = GetDebugFolderPath();
  if (debugPath.empty())
    return false;

  CreateDirectoryW(debugPath.c_str(), nullptr);
  const DWORD attrs = GetFileAttributesW(debugPath.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES &&
         (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::wstring GetLogFilePath() {
  if (!EnsureDebugFolderExists())
    return L"";

  SYSTEMTIME st{};
  GetLocalTime(&st);

  wchar_t fileName[128] = {};
  swprintf(fileName, sizeof(fileName) / sizeof(wchar_t),
           L"BetterAngle_%04u-%02u-%02u_%02u-%02u-%02u.log", st.wYear,
           st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

  return GetDebugFolderPath() + fileName;
}

void SetLogLevel(LogLevel level) { g_logLevel.store(level); }

LogLevel GetLogLevel() { return g_logLevel.load(); }

void SetLogToFile(bool enable) { g_logToFile.store(enable); }

void SetLogToConsole(bool enable) { g_logToConsole.store(enable); }

void SetLogFilePath(const std::wstring &path) {
  std::lock_guard<std::mutex> lock(g_logMutex);
  g_logFilePath = path;
  if (g_logFile && g_logFile->is_open())
    g_logFile->close();
  g_logFile.reset();
}

void InitEnhancedLogging() {
  std::lock_guard<std::mutex> lock(g_logMutex);

  EnsureDebugFolderExists();
  if (g_logFilePath.empty())
    g_logFilePath = GetLogFilePath();

  if (!g_logFilePath.empty()) {
    fs::path path(g_logFilePath);
    g_logFile =
        std::make_unique<std::ofstream>(path, std::ios::out | std::ios::app);
  }

  WriteLogLineUnlocked(
      "============================================================");
  WriteLogLineUnlocked("BetterAngle enhanced logging initialized");
  WriteLogLineUnlocked("Log file: " + Narrow(g_logFilePath));
  WriteLogLineUnlocked("Debug folder: " + Narrow(GetDebugFolderPath()));
  WriteLogLineUnlocked(
      "============================================================");
}

void ShutdownEnhancedLogging() {
  std::lock_guard<std::mutex> lock(g_logMutex);
  WriteLogLineUnlocked("BetterAngle enhanced logging shutdown");
  if (g_logFile && g_logFile->is_open())
    g_logFile->close();
  g_logFile.reset();
}

void LogMessage(LogLevel level, const char *file, int line, const char *format,
                ...) {
  va_list args;
  va_start(args, format);
  std::string message = VFormatOptional(format, args);
  va_end(args);
  LogWithPrefix(level, file, line, "", message);
}

void LogWindowEvent(HWND hwnd, const std::string &event, const char *file,
                    int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  std::string message = VFormatOptional(format, args);
  va_end(args);

  std::ostringstream prefix;
  prefix << "[WINDOW event=" << event << " hwnd=0x" << std::hex
         << reinterpret_cast<uintptr_t>(hwnd) << std::dec << "] ";
  LogWithPrefix(LogLevel::DEBUG, file, line, prefix.str(), message);
}

void LogMouseEvent(int x, int y, const std::string &button,
                   const std::string &action, const char *file, int line,
                   const char *format, ...) {
  va_list args;
  va_start(args, format);
  std::string message = VFormatOptional(format, args);
  va_end(args);

  std::ostringstream prefix;
  prefix << "[MOUSE button=" << button << " action=" << action << " x=" << x
         << " y=" << y << "] ";
  LogWithPrefix(LogLevel::DEBUG, file, line, prefix.str(), message);
}

void LogKeyEvent(int vk, const std::string &action, const char *file, int line,
                 const char *format, ...) {
  va_list args;
  va_start(args, format);
  std::string message = VFormatOptional(format, args);
  va_end(args);

  std::ostringstream prefix;
  prefix << "[KEY vk=" << vk << " action=" << action << "] ";
  LogWithPrefix(LogLevel::DEBUG, file, line, prefix.str(), message);
}

void LogWindowDragEvent(HWND hwnd, int x, int y, const std::string &action,
                        const char *file, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  std::string message = VFormatOptional(format, args);
  va_end(args);

  std::ostringstream prefix;
  prefix << "[DRAG hwnd=0x" << std::hex << reinterpret_cast<uintptr_t>(hwnd)
         << std::dec << " action=" << action << " x=" << x << " y=" << y
         << "] ";
  LogWithPrefix(LogLevel::DEBUG, file, line, prefix.str(), message);
}

void LogSystemInfo() {
  OSVERSIONINFOEXW osvi{};
  osvi.dwOSVersionInfoSize = sizeof(osvi);
#pragma warning(push)
#pragma warning(disable : 4996)
  GetVersionExW(reinterpret_cast<OSVERSIONINFOW *>(&osvi));
#pragma warning(pop)

  SYSTEM_INFO si{};
  GetSystemInfo(&si);

  LogMessage(LogLevel::INFO, __FILE__, __LINE__,
             "System info: Windows %lu.%lu build %lu, processors=%lu, arch=%u",
             osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber,
             si.dwNumberOfProcessors, si.wProcessorArchitecture);
}

void LogMemoryUsage() {
  MEMORYSTATUSEX statex{};
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex)) {
    LogMessage(LogLevel::INFO, __FILE__, __LINE__,
               "Memory: load=%lu%% totalPhys=%lluMB availPhys=%lluMB",
               statex.dwMemoryLoad,
               static_cast<unsigned long long>(statex.ullTotalPhys /
                                               (1024ull * 1024ull)),
               static_cast<unsigned long long>(statex.ullAvailPhys /
                                               (1024ull * 1024ull)));
  }
}

void LogThreadInfo() {
  LogMessage(LogLevel::INFO, __FILE__, __LINE__, "Thread info: pid=%lu tid=%lu",
             GetCurrentProcessId(), GetCurrentThreadId());
}

void LogWindowInfo(HWND hwnd) {
  if (!hwnd) {
    LogMessage(LogLevel::WARN, __FILE__, __LINE__,
               "Window info requested for null HWND");
    return;
  }

  RECT rc{};
  GetWindowRect(hwnd, &rc);
  const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
  const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

  LogMessage(LogLevel::INFO, __FILE__, __LINE__,
             "Window info: hwnd=0x%p rect=(%ld,%ld)-(%ld,%ld) style=0x%llx "
             "exStyle=0x%llx visible=%d enabled=%d",
             hwnd, rc.left, rc.top, rc.right, rc.bottom,
             static_cast<long long>(style), static_cast<long long>(exStyle),
             IsWindowVisible(hwnd), IsWindowEnabled(hwnd));
}

void LogProcessInfo() {
  wchar_t modulePath[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

  LogMessage(LogLevel::INFO, __FILE__, __LINE__, "Process info: pid=%lu exe=%s",
             GetCurrentProcessId(), Narrow(modulePath).c_str());
}

void LogStartup() {
  LogMessage(LogLevel::INFO, __FILE__, __LINE__,
             "Startup: version=%s debugMode=%d appRoot=%s debugFolder=%s",
             VERSION_STR, g_debugMode ? 1 : 0, Narrow(GetAppRootPath()).c_str(),
             Narrow(GetDebugFolderPath()).c_str());
  LogMessage(LogLevel::INFO, __FILE__, __LINE__,
             "Runtime state: selectedProfileIdx=%d showCrosshair=%d "
             "forceDiving=%d forceDetection=%d",
             g_selectedProfileIdx, g_showCrosshair ? 1 : 0,
             g_forceDiving ? 1 : 0, g_forceDetection ? 1 : 0);
  LogProcessInfo();
  LogThreadInfo();
  LogSystemInfo();
  LogMemoryUsage();
}
