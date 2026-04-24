### BetterAngle Pro v5.4.0
- **CRITICAL BLOCKINPUT FIX:** Fixed scancode injection bug in SyncGamingKeysNitro that caused keyboard ghosting
- **ROOT CAUSE:** Windows async key state table frozen by BlockInput API
- **SOLUTION:** Unconditional KeyUp injection with correct scancode usage (wVk = 0)
- **GHOST ELIMINATION:** Keys released during BlockInput no longer remain "held" after unlock
- **VERSION STABILITY:** Fixed bump_version.ps1 script and version consistency across all files

### BetterAngle Pro v5.3.1
- **ABSOLUTE RESTORATION:** Implemented unconditional pre-lock state capture and force-release logic.
- **GHOST ELIMINATOR:** Bypasses the frozen Windows Async Key Table by restoring hardware state directly after input blocks.
- **Zero-Verification Latency:** Removed redundant post-lock polling; restoration happens instantly upon BlockInput(FALSE).
- **Core Stability:** Hard-locked versioning across all project binaries.

- Automated build release.

### BetterAngle Pro v5.1.16
- Automated build release.

### BetterAngle Pro v5.1.15
- Automated build release.

### BetterAngle Pro v5.1.14
- Automated build release.

### BetterAngle Pro v5.1.13
- Automated build release.

### BetterAngle Pro v5.1.12
- Automated build release.

### BetterAngle Pro v5.1.11
- Automated build release.

### BetterAngle Pro v5.1.10
- Automated build release.

### BetterAngle Pro v5.1.9
- docs: added technical summary of nitro sync logic

### BetterAngle Pro v5.1.8
- Automated build release.

### BetterAngle Pro v5.1.7
- Automated build release.

### BetterAngle Pro v5.1.6
- Automated build release.

### BetterAngle Pro v5.1.5
- Automated build release.

### BetterAngle Pro v5.1.4
- Automated build release.

### BetterAngle Pro v5.1.3
- Automated build release.

### BetterAngle Pro v5.1.2
- Automated build release.

### BetterAngle Pro v5.1.1
- Automated build release.

### BetterAngle Pro v5.0.111
- Automated build release.

### BetterAngle Pro v5.0.109
- Automated build release.

### BetterAngle Pro v5.0.105
- Automated build release.

### BetterAngle Pro v5.0.102
- Automated build release.

### BetterAngle Pro v5.0.101
- Automated build release.

### BetterAngle Pro v5.0.92
- Automated build release.

### BetterAngle Pro v5.0.90
- Automated build release.

### BetterAngle Pro v5.0.88
- Automated build release.

### BetterAngle Pro v5.0.86
- Automated build release.

### BetterAngle Pro v5.0.85
- perf: implemented Mathematical Optimizations including SIMD pixel scanning, Sensitivity Baking, and Integer Comparison Theory for ultra-smooth performance.
- fix: synchronized GitHub Actions and Releases version numbering.

### BetterAngle Pro v5.0.83
- Automated build release.

### BetterAngle Pro v5.0.82
- fix: improved updater reliability with a more robust JSON parser and standard User-Agent to ensure smooth updates from GitHub.

### BetterAngle Pro v5.0.80
- release: baseline stability restoration. This version resets the codebase to the stable v5.0.74 logic, providing a reliable foundation for future manual optimizations.
- perf: implemented ultra-fast focus and detector threads using yield/Sleep(0) and HWND caching for zero-delay input locking.

### BetterAngle Pro v5.0.73
- Automated build release.

### BetterAngle Pro v5.0.72
- Automated build release.

### BetterAngle Pro v5.0.71
- Automated build release.

### BetterAngle Pro v5.0.70
- Automated build release.

### BetterAngle Pro v5.0.69
- Automated build release.

### BetterAngle Pro v5.0.68
- Automated build release.

### BetterAngle Pro v5.0.67
- Automated build release.

### BetterAngle Pro v5.0.66
- Automated build release.

### BetterAngle Pro v5.0.65
- Automated build release.

### BetterAngle Pro v5.0.64
- Automated build release.

### BetterAngle Pro v5.0.63
- Automated build release.

### BetterAngle Pro v5.0.62
- Automated build release.

### BetterAngle Pro v5.0.61
- Automated build release.

### BetterAngle Pro v5.0.60
- Automated build release.

### BetterAngle Pro v5.0.59
- Automated build release.

### BetterAngle Pro v5.0.58
- Automated build release.

### BetterAngle Pro v5.0.57
- Automated build release.

### BetterAngle Pro v5.0.56
- Automated build release.

### BetterAngle Pro v5.0.55
- Automated build release.

### BetterAngle Pro v5.0.54
- Automated build release.

### BetterAngle Pro v5.0.53
- Automated build release.

### BetterAngle Pro v5.0.52
- Automated build release.

### BetterAngle Pro v5.0.51
- Automated build release.

### BetterAngle Pro v5.0.50
- Automated build release.

### BetterAngle Pro v5.0.49
- Automated build release.

### BetterAngle Pro v5.0.48
- Automated build release.

### BetterAngle Pro v5.0.47
- Automated build release.

### BetterAngle Pro v5.0.46
- Automated build release.

### BetterAngle Pro v5.0.45
- Automated build release.

### BetterAngle Pro v5.0.44
- Automated build release.

### BetterAngle Pro v5.0.43
- Automated build release.

### BetterAngle Pro v5.0.42
- Automated build release.

### BetterAngle Pro v5.0.41
- Automated build release.

### BetterAngle Pro v5.0.40
- Automated build release.

### BetterAngle Pro v5.0.39
- Automated build release.

### BetterAngle Pro v5.0.38
- Automated build release.

### BetterAngle Pro v5.0.37
- Automated build release.

### BetterAngle Pro v5.0.36
- Automated build release.

### BetterAngle Pro v5.0.35
- Automated build release.

### BetterAngle Pro v5.0.34
- Automated build release.

### BetterAngle Pro v5.0.33
- Automated build release.

### BetterAngle Pro v5.0.32
- Automated build release.

### BetterAngle Pro v5.0.31
- Automated build release.

### BetterAngle Pro v5.0.30
- Automated build release.

### BetterAngle Pro v5.0.29
- Automated build release.

### BetterAngle Pro v5.0.28
- Automated build release.

### BetterAngle Pro v5.0.27
- Automated build release.

### BetterAngle Pro v5.0.26
- Automated build release.

### BetterAngle Pro v5.0.25
- Automated build release.

### BetterAngle Pro v5.0.24
- Automated build release.

### BetterAngle Pro v5.0.23
- Automated build release.

### BetterAngle Pro v5.0.22
- Automated build release.

### BetterAngle Pro v5.0.21
- Automated build release.

### BetterAngle Pro v5.0.20
- Automated build release.

### BetterAngle Pro v5.0.19
- Automated build release.

### BetterAngle Pro v5.0.18
- Automated build release.

### BetterAngle Pro v5.0.17
- Automated build release.

### BetterAngle Pro v5.0.16
- Automated build release.

### BetterAngle Pro v5.0.15
- Automated build release.

### BetterAngle Pro v5.0.14
- Automated build release.

### BetterAngle Pro v5.0.13
- Automated build release.

### BetterAngle Pro v5.0.12
- Automated build release.

### BetterAngle Pro v5.0.11
- Automated build release.

### BetterAngle Pro v5.0.10
- Automated build release.

### BetterAngle Pro v5.0.9
- Automated build release.

### BetterAngle Pro v5.0.8
- Automated build release.

### BetterAngle Pro v5.0.7
- Automated build release.

### BetterAngle Pro v5.0.6
- Automated build release.

### BetterAngle Pro v5.0.5
- Automated build release.

### BetterAngle Pro v5.0.4
- Automated build release.

### BetterAngle Pro v5.0.3
- Automated build release.

### BetterAngle Pro v5.0.2
- Automated build release.

### BetterAngle Pro v5.0.1
- Automated build release.

### BetterAngle Pro v5.0.0
- Major version 5.0.0 release
- Complete rewrite with modern C++17 architecture
- Enhanced detection algorithms with SIMD optimization
- Improved GUI with Qt6 framework
- Robust versioning and release automation
