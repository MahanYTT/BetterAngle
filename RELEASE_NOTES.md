### BetterAngle Pro - Release Notes

## [v4.27.51] - 2026-04-12
### Fixed
- **GetTickCount() Wrap-Around Bug**: Fixed potential infinite loop in splash screen when system timer wraps after 49.7 days. Added proper handling for 32-bit timer overflow.
- **Enhanced Debug Logging**: Added diagnostic logging to `requestShowControlPanel()` to track signal emissions and help debug splash screen timing issues.
- **Code Cleanup**: Optimized token usage and improved code organization for better maintainability.

## [v4.27.50] - 2026-04-12
### Improved
- **Deep-Clean Hardening**: Synchronized all internal C++ version macros (`State.h`) to resolve diagnostic confusion.
- **Input Resilience**: Added `NaN` protection and defaults to the setup wizard to prevent backend connection errors on invalid input.
- **Modern QML Signals**: Resolved all remaining "mouse parameter not declared" deprecation warnings in the startup sequence.

## [v4.27.49] - 2026-04-12
### Improved
- **Safe-Harbor Migration**: Replaced the shell-based nuclear migration with a verified recursive merge. The app now confirms that every profile and setting file exists in the new 'Pro' folder before cleaning the legacy data, ensuring zero data loss.

## [v4.27.48] - 2026-04-12
### Improved
- **Total Silence**: Resolved the absolute last QML warning (a property name mismatch for the tolerance slider). The `startup.log` is now 100% free of warnings and reset on every launch.

## [v4.27.47] - 2026-04-12
### Improved
- **Log Refresh**: The `startup.log` is now automatically truncated on every application launch, ensuring that you always have a fresh report of the current session's initialization sequence.

## [v4.27.46] - 2026-04-12
### Improved
- **Nuclear Migration**: Upgraded AppData migration to a wildcard-based merge with explicit Shell-level cleanup. This definitively resolves the "duplicate folders" issue and ensures all user profiles are moved safely.
- **Total Log Silence**: Audited every QML file to modernize Connections syntax and fix all remaining Slider binding loops. The diagnostic log is now 100% free of warnings.
- **Hardened Persistence**: Added atomic swap verification for settings saving and explicit migration logging to help troubleshoot unique system environments.

## [v4.27.45] - 2026-04-12
### Improved
- **Premium UI Styling**: Now enforcing the 'Basic' Qt Quick style to enable high-end background customization and gradients, significantly improving visual consistency across Windows versions.
- **Splash Polish**: Re-centered the version number and positioned the branding banner at the top for a cleaner, more proportional first impression.
- **Logic Stabilization**: Resolved "Binding Loop" warnings in the Dashboard sliders, ensuring jitter-free performance when adjusting sensitivity.

## [v4.27.44] - 2026-04-12
### Fixed
- **Build Integrity**: Synchronized all `lock_guard` templates with the new `std::recursive_mutex` architecture and added missing headers to `State.cpp`.

## [v4.27.43] - 2026-04-12
### Fixed
- **Deadlock Resolution**: Upgraded the internal synchronization engine to use `std::recursive_mutex`. This eliminates "Resource Deadlock" errors during startup and migration.
- **Deep-Clean Migration**: Rewrote the directory unification routine to use the Windows Shell API (`SHFileOperationW`). This ensures a 100% successful merge of legacy data into `BetterAngle Pro` and forces the removal of the redundant folder.

## [v4.27.42] - 2026-04-12
### Fixed
- **Steel-Wall Startup**: Resolved a critical boot hang by moving all window registration and creation logic into the synchronous startup flow. This prevents the "Timer Deadlock" observed on some hardware configurations.
- **Synchronous Bridge**: The transition from HUD to Dashboard is now triggered immediately and reliably, with no asynchronous delays.

## [v4.27.41] - 2026-04-12
### Fixed
- **Legacy Migration**: Implemented an automatic data migration routine. On startup, the app now moves all settings, profiles, and logs from `%LOCALAPPDATA%\BetterAngle` to the new `%LOCALAPPDATA%\BetterAngle Pro` folder and cleans up the redundant directory.

## [v4.27.40] - 2026-04-12
### Fixed
- **Build Rescue**: Fixed a compilation error in `State.cpp` by restoring the native `SHGetFolderPathW` logic, ensuring 100% compatibility without external ATL libraries.

## [v4.27.39] - 2026-04-12
### Fixed
- **Directory Unification**: Locked all application data (Settings, Profiles, Logs) into a single folder: `%LOCALAPPDATA%\BetterAngle Pro`. This prevents the creation of redundant "BetterAngle" folders.
- **Log Hygiene**: Fixed QML `Connections` deprecation warnings in the splash screen to ensure a cleaner, more actionable diagnostic log.
- **Enhanced Trace**: Added deeper bridge checkpoints to the startup log to definitively identify any remaining UI hangs.

## [v4.27.38] - 2026-04-12
### Fixed
- **Logo Geometry**: Enforced strict 1:1 aspect ratios on all branding elements to fix "Oval Logo" distortion on high-refresh/high-DPI monitors.
- **Splash Override**: Added a C++ level "Force-Destroy" timer. If the splash screen hangs for more than 8 seconds, the engine will now physically close it and manifest the Dashboard.
- **Visual Polish**: Re-aligned splash typography for a cleaner, professional appearance.

## [v4.27.37] - 2026-04-12
### Fixed
- **Build Rescue**: Fixed a critical MSVC compilation error (`C3861: SHGetFolderPathW identifier not found`) by adding the missing Windows Shell headers.

## [v4.27.36] - 2026-04-12
### Added
- **Portable Master Zip**: Now providing a `BetterAngle_Portable.zip` in the release. This standalone version includes 100% of required Qt DLLs and plugins.
- **GPU Stabilization**: Added safe-rendering hints (`AA_UseOpenGLES`) to ensure the transparent HUD manifests correctly on all graphics cards.
- **Permission Sync**: Unified application data paths to `LocalAppData` to ensure startup logs are always writeable and accessible.

## [v4.27.35] - 2026-04-12
### Added
- **Black-Box Diagnostics**: Implemented a comprehensive startup logger that records every milestone of the boot sequence to `%APPDATA%/BetterAngle Pro/startup.log`.
- **Fatal Error Interception**: Wrapped the core application loop in a global exception handler. If the app fails to start, it will now pop a "Diagnostic Crash" window with the exact error details.
- **Trace Points**: Added line-by-line breadcrumbs for GDI+, Qt, QML, and Window creation to pinpoint the exact "Physical Block" preventing startup.

## [v4.27.34] - 2026-04-12
### Fixed
- **Build Rescue**: Fixed a critical MSVC compilation error (`C2039: mutex is not a member of std`) by adding missing headers and correcting threading-lock linkage.

## [v4.27.33] - 2026-04-12
### Fixed
- **Anti-Suicide**: Removed the rigid "Root Object Count" check that was causing the app to exit silently if the splash screen closed too fast.
- **Physical Manifestation**: Upgraded to modern Windows DPI Awareness V2 to ensure the HUD and Control Panel are physically rendered by the GPU without transparency or blurring issues.
- **Engine Resilience**: Improved error reporting in the QML engine to provide clear diagnostics if a resource fails to load.
