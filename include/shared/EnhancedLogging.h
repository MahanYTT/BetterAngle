#pragma once

#include <atomic>
#include <cstdarg>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

struct HWND__;
typedef HWND__ *HWND;

enum class LogLevel {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5
};

#define LOG_TRACE(...)                                                         \
  LogMessage(LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...)                                                         \
  LogMessage(LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)                                                          \
  LogMessage(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)                                                          \
  LogMessage(LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  LogMessage(LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...)                                                         \
  LogMessage(LogLevel::FATAL, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_WINDOW_EVENT(hwnd, event, ...)                                     \
  LogWindowEvent(hwnd, event, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MOUSE_EVENT(x, y, button, action, ...)                             \
  LogMouseEvent(x, y, button, action, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_KEY_EVENT(vk, action, ...)                                         \
  LogKeyEvent(vk, action, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WINDOW_DRAG_EVENT(hwnd, x, y, action, ...)                         \
  LogWindowDragEvent(hwnd, x, y, action, __FILE__, __LINE__, __VA_ARGS__)

void InitEnhancedLogging();
void ShutdownEnhancedLogging();
void SetLogLevel(LogLevel level);
LogLevel GetLogLevel();
void SetLogToFile(bool enable);
void SetLogToConsole(bool enable);
void SetLogFilePath(const std::wstring &path);

void LogMessage(LogLevel level, const char *file, int line, const char *format,
                ...);
void LogWindowEvent(HWND hwnd, const std::string &event, const char *file,
                    int line, const char *format = "", ...);
void LogMouseEvent(int x, int y, const std::string &button,
                   const std::string &action, const char *file, int line,
                   const char *format = "", ...);
void LogKeyEvent(int vk, const std::string &action, const char *file, int line,
                 const char *format = "", ...);
void LogWindowDragEvent(HWND hwnd, int x, int y, const std::string &action,
                        const char *file, int line, const char *format = "",
                        ...);

void LogSystemInfo();
void LogMemoryUsage();
void LogThreadInfo();
void LogWindowInfo(HWND hwnd);
void LogProcessInfo();
void LogStartup();

std::wstring GetDebugFolderPath();
bool EnsureDebugFolderExists();
std::wstring GetLogFilePath();

std::string LogLevelToString(LogLevel level);
std::string GetCurrentTimeString();
std::string FormatString(const char *format, ...);
std::string VFormatString(const char *format, va_list args);

extern std::atomic<LogLevel> g_logLevel;
extern std::atomic<bool> g_logToFile;
extern std::atomic<bool> g_logToConsole;
extern std::wstring g_logFilePath;
extern std::mutex g_logMutex;
extern std::unique_ptr<std::ofstream> g_logFile;