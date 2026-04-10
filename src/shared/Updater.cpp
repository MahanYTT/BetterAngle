// Updater.cpp — BetterAngle Pro
// Uses WinHTTP throughout for proper HTTPS + redirect support (GitHub CDN requires this).
// URLDownloadToFileW was dropped because it silently fails on GitHub release redirects.
#include "shared/Updater.h"
#include "shared/State.h"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <string>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

#ifndef VERSION_STR
#define VERSION_STR "4.20.1"
#endif
#define APP_VERSION_STR VERSION_STR

// The single canonical download URL — GitHub resolves /releases/latest/download/ automatically
static const wchar_t* DOWNLOAD_URL =
    L"https://github.com/MahanYTT/BetterAngle/releases/latest/download/BetterAngle.exe";

// ─────────────────────────────────────────────────────────────────────────────
// Internal: WinHTTP GET that writes the response body to |destPath|.
// Follows redirects, handles HTTPS correctly. Returns true on HTTP 200.
// ─────────────────────────────────────────────────────────────────────────────
static bool WinHttpGetToFile(const std::wstring& url, const std::wstring& destPath) {
    // Parse URL
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[512] = {}, path[2048] = {};
    uc.lpszHostName    = host; uc.dwHostNameLength    = 512;
    uc.lpszUrlPath     = path; uc.dwUrlPathLength     = 2048;

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    HINTERNET hSess = WinHttpOpen(
        L"BetterAngle-Updater/" APP_WSTR_Y(V_MAJ) L"." APP_WSTR_Y(V_MIN),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return false;

    // Set a reasonable timeout (15 s connect, 60 s recv)
    DWORD connectTimeout = 15000, recvTimeout = 60000;
    WinHttpSetOption(hSess, WINHTTP_OPTION_CONNECT_TIMEOUT,  &connectTimeout, sizeof(DWORD));
    WinHttpSetOption(hSess, WINHTTP_OPTION_RECEIVE_TIMEOUT,  &recvTimeout,    sizeof(DWORD));

    HINTERNET hConn = WinHttpConnect(hSess, host, uc.nPort, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return false; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", path, NULL,
                                        WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return false; }

    // Follow redirects automatically
    DWORD redir = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hReq, WINHTTP_OPTION_REDIRECT_POLICY, &redir, sizeof(redir));

    bool ok = WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
              WinHttpReceiveResponse(hReq, NULL);

    if (ok) {
        // Verify HTTP 200
        DWORD status = 0, sz = sizeof(DWORD);
        WinHttpQueryHeaders(hReq,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
        ok = (status == 200);
    }

    if (ok) {
        std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) ok = false;

        if (ok) {
            char buf[16384];
            DWORD read = 0;
            while (true) {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(hReq, &avail) || avail == 0) break;
                DWORD chunk = (avail > sizeof(buf)) ? (DWORD)sizeof(buf) : avail;
                if (!WinHttpReadData(hReq, buf, chunk, &read) || read == 0) break;
                out.write(buf, read);
            }
            out.close();
            // Sanity-check: a valid exe must be at least 64 KB
            LARGE_INTEGER sz2 = {};
            HANDLE hFile = CreateFileW(destPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                       NULL, OPEN_EXISTING, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                GetFileSizeEx(hFile, &sz2);
                CloseHandle(hFile);
            }
            if (sz2.QuadPart < 65536) {
                DeleteFileW(destPath.c_str());
                ok = false;
            }
        }
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: WinHTTP GET that returns the response body as a string.
// Used by CheckForUpdates to fetch the GitHub releases API JSON.
// ─────────────────────────────────────────────────────────────────────────────
static bool WinHttpGetString(const std::wstring& url, std::string& out) {
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[512] = {}, path[2048] = {};
    uc.lpszHostName    = host; uc.dwHostNameLength    = 512;
    uc.lpszUrlPath     = path; uc.dwUrlPathLength     = 2048;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    HINTERNET hSess = WinHttpOpen(
        L"BetterAngle-Updater/" APP_WSTR_Y(V_MAJ) L"." APP_WSTR_Y(V_MIN),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return false;

    DWORD connectTimeout = 10000, recvTimeout = 15000;
    WinHttpSetOption(hSess, WINHTTP_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(DWORD));
    WinHttpSetOption(hSess, WINHTTP_OPTION_RECEIVE_TIMEOUT, &recvTimeout,    sizeof(DWORD));

    HINTERNET hConn = WinHttpConnect(hSess, host, uc.nPort, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return false; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", path, NULL,
                                        WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return false; }

    // GitHub API requires User-Agent — already set via WinHttpOpen above.
    bool ok = WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
              WinHttpReceiveResponse(hReq, NULL);

    if (ok) {
        char buf[4096];
        DWORD read = 0;
        while (true) {
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(hReq, &avail) || avail == 0) break;
            DWORD chunk = (avail > sizeof(buf)) ? (DWORD)sizeof(buf) : avail;
            if (!WinHttpReadData(hReq, buf, chunk, &read) || read == 0) break;
            out.append(buf, read);
        }
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return ok && !out.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

bool CheckForUpdates() {
    g_isCheckingForUpdates = true;
    g_updateAvailable      = false;

    std::string json;
    bool got = WinHttpGetString(
        L"https://api.github.com/repos/MahanYTT/BetterAngle/releases/latest", json);

    if (!got || json.empty()) {
        g_isCheckingForUpdates = false;
        return false;
    }

    // Extract "tag_name": "vX.Y.Z"
    const std::string prefix = "\"tag_name\": \"";
    size_t pos = json.find(prefix);
    if (pos == std::string::npos) {
        g_isCheckingForUpdates = false;
        return false;
    }

    size_t start = pos + prefix.size();
    size_t end   = json.find('"', start);
    std::string tag = json.substr(start, end - start);   // e.g. "v4.20.5"

    g_latestVersionOnline = tag;
    g_latestName = L"GitHub Release (" +
                   std::wstring(tag.begin(), tag.end()) + L")";

    // Strip leading 'v' for comparison
    std::string remote = tag;
    if (!remote.empty() && (remote[0] == 'v' || remote[0] == 'V'))
        remote = remote.substr(1);

    std::string local = APP_VERSION_STR;

    g_updateAvailable      = (remote != local);
    g_isCheckingForUpdates = false;
    return g_updateAvailable;
}

// dest is a FULL absolute path (caller must supply it via GetModuleFileName).
bool DownloadUpdate(const std::wstring& /*urlHint*/, const std::wstring& dest) {
    // Always use the canonical /releases/latest/download/ URL.
    // WinHTTP will follow the GitHub → CDN redirect chain correctly.
    return WinHttpGetToFile(DOWNLOAD_URL, dest);
}

void ApplyUpdateAndRestart() {
    // Resolve the full path of the running exe so the batch uses absolute paths.
    wchar_t exeBuf[MAX_PATH] = {};
    GetModuleFileNameW(NULL, exeBuf, MAX_PATH);
    std::wstring exeFull = exeBuf;

    size_t lastSlash = exeFull.find_last_of(L"\\/");
    std::wstring exeDir  = exeFull.substr(0, lastSlash + 1);  // includes trailing slash
    std::wstring exeName = exeFull.substr(lastSlash + 1);       // e.g. "BetterAngle.exe"

    std::wstring tmpFull = exeDir + L"update_tmp.exe";
    std::wstring batFull = exeDir + L"ba_update.bat";

    // Write the helper batch using narrow strings (cmd.exe is fine with ANSI paths)
    std::string exeA(exeFull.begin(), exeFull.end());
    std::string tmpA(tmpFull.begin(), tmpFull.end());
    std::string batA(batFull.begin(), batFull.end());
    std::string dirA(exeDir.begin(),  exeDir.end());
    std::string nameA(exeName.begin(), exeName.end());

    std::ofstream bat(batFull);
    bat << "@echo off\n";
    bat << "timeout /t 2 /nobreak >nul\n";
    bat << "taskkill /F /IM \"" << nameA << "\" >nul 2>&1\n";
    bat << "timeout /t 1 /nobreak >nul\n";
    bat << "del /F /Q \"" << exeA << "\"\n";
    bat << "move /Y \"" << tmpA << "\" \"" << exeA << "\"\n";
    bat << "start \"\" \"" << exeA << "\"\n";
    bat << "del \"%~f0\"\n";
    bat.close();

    // Run the batch hidden and exit immediately — the batch will wait for us to die
    ShellExecuteA(NULL, "open", batA.c_str(), NULL, dirA.c_str(), SW_HIDE);
    ExitProcess(0);
}