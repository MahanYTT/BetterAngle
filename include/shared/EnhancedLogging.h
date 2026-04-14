#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <memory>
#include <windows.h>

// Handle Windows ERROR macro collision
#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// Global level control
extern std::atomic<LogLevel> g_logLevel;

// Global functions
void InitEnhancedLogging();
void ShutdownEnhancedLogging();
void SetLogLevel(LogLevel level);
void LogStartup();
void LogWindowInfo(const wchar_t* label, HWND hwnd);

// Core logging functions (Overloaded for ANSI and Wide strings)
void LogMessage(LogLevel level, const char* file, int line, const char* format, ...);
void LogMessage(LogLevel level, const char* file, int line, const wchar_t* format, ...);

// Macros
#define LOG_TRACE(fmt, ...) LogMessage(LogLevel::Trace, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LogMessage(LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LogMessage(LogLevel::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LogMessage(LogLevel::Warning, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LogMessage(LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) LogMessage(LogLevel::Fatal, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

class EnhancedLogger {
public:
    static EnhancedLogger& Instance();

    void Initialize(const std::wstring& logPath);
    void Log(LogLevel level, const char* file, int line, const std::string& message);
    void Log(LogLevel level, const char* file, int line, const std::wstring& message);
    void Flush();

private:
    EnhancedLogger() = default;
    ~EnhancedLogger();

    std::ofstream m_stream;
    std::mutex m_mutex;
    bool m_initialized = false;

    std::string LevelToString(LogLevel level) const;
    std::string TimestampNow() const;
    std::string ToNarrow(const std::wstring& wstr);
};
