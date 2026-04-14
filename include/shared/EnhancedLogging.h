#pragma once

#include <string>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

void InitEnhancedLogging();
void ShutdownEnhancedLogging();
void SetLogLevel(LogLevel level);
void LogStartup();
void LogWindowInfo(const wchar_t* label, void* hwnd);
void LogMessage(LogLevel level, const wchar_t* message);

#define LOG_INFO(msg) LogMessage(LogLevel::Info, msg)
#define LOG_WARNING(msg) LogMessage(LogLevel::Warning, msg)
#define LOG_ERROR(msg) LogMessage(LogLevel::Error, msg)
#define LOG_DEBUG(msg) LogMessage(LogLevel::Debug, msg)