### BetterAngle Pro - Release Notes

## [v4.27.56] - 2026-04-12
### Fixed
- **GDI+ Header Order Final Verification**: Ensured windows.h is included before gdiplus.h in all source files (BetterAngle.cpp, ThresholdWizard.cpp, Startup.cpp) with exact whitespace matching to resolve compilation errors with Windows SDK 10.0.26100.0.
- **Build Success**: GitHub Actions builds now pass without GDI+ compilation errors after correcting include order across all three files.

## [v4.27.55] - 2026-04-12
### Fixed
- **GDI+ Header Order Final Fix**: Corrected include order in all source files (BetterAngle.cpp, ThresholdWizard.cpp, Startup.cpp) to ensure windows.h is included before gdiplus.h, resolving compilation errors with Windows SDK 10.0.26100.0.
- **Build Stability**: Ensured GitHub Actions builds pass without GDI+ compilation errors by fixing header inclusion sequence across the codebase.

## [v4.27.54] - 2026-04-12
### Fixed
- **GDI+ Compilation Errors**: Fixed Windows SDK 10.0.26100.0 compatibility issues by correcting include order (windows.h before gdiplus.h) and adding objidl.h include.
- **Build System**: Resolved GitHub Actions build failures caused by GDI+ header conflicts in newer Windows SDK versions.

## [v4.27.53] - 2026-04-12
### Fixed
- **Infinite Splash Loading & High CPU Usage**: Fixed multiple infinite animation loops in splash screen causing 10% CPU usage on high-end systems. Added animation cleanup when splash closes.
- **Performance Optimization**: Reduced boot thread delay from 2.5s to 0.5s for faster splash closing. Throttled canvas repaints and optimized animation frequencies.
- **Animation Cleanup**: All infinite animations (wave, pulse, glow, line) now properly stop when `closeSplashRequested` signal is received, reducing CPU load after splash closes.

### Improved
- **Taskbar Presence**: Ensured splash and main UI always appear in taskbar when open, even when minimized or not showing content.
- **Portable Debugging**: Enhanced startup logging to create debug folder in portable mode for troubleshooting.

## [v4.27.52] - 2026-04-12
### Added
- **Portable Mode Support**: Added detection of 'portable.flag' file to store all application data in 'Data/' folder within executable directory.
- **Clean Uninstallation**: Installer now removes all AppData traces during uninstallation (BetterAngle and BetterAngle Pro folders).

### Changed
- **Release Artifacts**: Removed standalone BetterAngle.exe from GitHub releases. Now only provides:
  - `BetterAngle_Setup.exe` - Full installer with clean uninstall
  - `BetterAngle_Portable.zip` - Portable version with self-contained data storage

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
- **Portable Mode**: Added detection of 'portable.flag' file to store all application data in 'Data/' folder within executable directory.
- **Clean Uninstallation**: Installer now removes all AppData traces during uninstallation (BetterAngle and BetterAngle Pro folders).

## [v4.27.47] - 2026-04-12
### Fixed
- **Splash Screen Hang**: Fixed infinite loading caused by missing `closeSplashRequested` signal emission in QML.
- **Taskbar Visibility**: Removed Qt.Tool flag from splash window and added proper window title to ensure taskbar presence.
- **Debug Logging**: Added portable-aware startup logging that creates debug folder in portable mode for troubleshooting.

## [v4.27.46] - 2026-04-12
### Added
- **Stronger Diagnostics**: Enhanced logging in ControlPanel.cpp to track splash-to-dashboard transition.
- **Performance Optimization**: Reduced boot thread delay and improved animation cleanup.

## [v4.27.45] - 2026-04-12
### Fixed
- **Program Launch Issues**: Fixed invalid LogStartup() call in Splash.qml and missing debug folder in portable builds.
- **Taskbar Presence**: Made splash and main UI always appear in taskbar when open.

## [v4.27.44] - 2026-04-12
### Changed
- **Release Structure**: Updated GitHub Actions workflow to remove standalone executable from release artifacts.
- **Installer Cleanup**: Added [UninstallDelete] section to installer.iss to clean AppData during uninstallation.

## [v4.27.43] - 2026-04-12
### Added
- **Portable Mode**: Added portable.flag detection and Data directory creation in GitHub Actions.
- **Self-Contained Data**: Portable version now keeps everything inside folder and doesn't save data elsewhere.

## [v4.27.42] - 2026-04-12
### Fixed
- **Version Conflict**: Resolved merge conflicts between v4.27.49 and v4.27.50, updated to v4.27.51.
- **GitHub Actions**: Ensured proper tagging and release creation.

## [v4.27.41] - 2026-04-12
### Improved
- **Code Organization**: Cleaned up and organized code to use fewer tokens while maintaining 100% functionality.
- **Bug Fixes**: Addressed program getting stuck and splash screen issues identified during analysis.
