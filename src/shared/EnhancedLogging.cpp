#include "shared/EnhancedLogging.h"

#include <windows.h>
#include <fstream>
#include <mutex>
#include <sstream>

namespace {
std::wofstream g_logFile;
std::mutex g_logMutex;
LogLevel g_currentLevel = LogLevel::Info;

const wchar_t* LevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return L"DEBUG";
    case LogLevel::Info:
        return L"INFO";
    case LogLevel::Warning:
        return L"WARNING";
    case LogLevel::Error:
        return L"ERROR";
    default:
        return L"UNKNOWN";
    }
}

bool ShouldLog(LogLevel level) {
    return static_cast<int>(level) >= static_cast<int>(g_currentLevel);
}

std::wstring GetLogPath() {
    wchar_t tempPath[MAX_PATH] = {};
    DWORD len = GetTempPathW(MAX_PATH, tempPath);
    if (len == 0 || len >= MAX_PATH) {
        return L"BetterAngle.log";
    }
    return std::wstring(tempPath) + L"BetterAngle.log";
}
}

void InitEnhancedLogging() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile.is_open()) {
        g_logFile.open(GetLogPath(), std::ios::out | std::ios::app);
    }
}

void ShutdownEnhancedLogging() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile.flush();
        g_logFile.close();
    }
}

void SetLogLevel(LogLevel level) {
    g_currentLevel = level;
}

void LogMessage(LogLevel level, const wchar_t* message) {
    if (!message || !ShouldLog(level)) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_logMutex);

    std::wstringstream line;
    line << L"[" << LevelToString(level) << L"] " << message << L"\n";

    OutputDebugStringW(line.str().c_str());

    if (g_logFile.is_open()) {
        g_logFile << line.str();
        g_logFile.flush();
    }
}

void LogStartup() {
    LogMessage(LogLevel::Info, L"BetterAngle startup initialized.");
}

void LogWindowInfo(const wchar_t* label, void* hwnd) {
    std::wstringstream ss;
    ss << (label ? label : L"Window") << L": 0x" << std::hex
       << reinterpret_cast<unsigned long long>(hwnd);
    LogMessage(LogLevel::Info, ss.str().c_str());
}
