#ifndef ENHANCEDLOGGING_H
#define ENHANCEDLOGGING_H

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

// Forward declarations for Windows types
struct HWND__;
typedef HWND__ *HWND;
typedef long HRESULT;

// Define S_OK if not already defined
#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif

// Log levels
enum LogLevel {
  LOG_TRACE = 0,
  LOG_DEBUG = 1,
  LOG_INFO = 2,
  LOG_WARN = 3,
  LOG_ERROR = 4,
  LOG_FATAL = 5
};

// Global log level (can be adjusted at runtime)
extern LogLevel g_currentLogLevel;

// Enhanced logging macros with file and line information
#define LOG_TRACE_MSG(msg) LogEnhanced(LOG_TRACE, __FILE__, __LINE__, msg)
#define LOG_DEBUG_MSG(msg) LogEnhanced(LOG_DEBUG, __FILE__, __LINE__, msg)
#define LOG_INFO_MSG(msg) LogEnhanced(LOG_INFO, __FILE__, __LINE__, msg)
#define LOG_WARN_MSG(msg) LogEnhanced(LOG_WARN, __FILE__, __LINE__, msg)
#define LOG_ERROR_MSG(msg) LogEnhanced(LOG_ERROR, __FILE__, __LINE__, msg)
#define LOG_FATAL_MSG(msg) LogEnhanced(LOG_FATAL, __FILE__, __LINE__, msg)

// Function declarations
void InitEnhancedLogging();
void LogEnhanced(LogLevel level, const char *file, int line,
                 const std::string &message);
void LogWindowInfo(HWND hWnd, const std::string &context);
void LogQMLState(const std::string &component, const std::string &state);
void LogMemoryUsage(const std::string &context);
void LogSystemInfo();
void LogThreadInfo(const std::string &context);
void LogRegistryOperation(const std::string &operation, const std::string &key,
                          bool success);
void LogFileOperation(const std::string &operation, const std::string &path,
                      bool success);
void LogCOMOperation(const std::string &operation, HRESULT hr);
void LogGDIResource(const std::string &context, int count);
void LogDirectXState(const std::string &context, HRESULT hr = S_OK);
void LogWindowDragEvent(const std::string &context, int x, int y, bool started);

// Backward compatibility
void LogStartup(const std::string &msg);

#endif // ENHANCEDLOGGING_H