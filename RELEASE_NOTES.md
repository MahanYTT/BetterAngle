### BetterAngle Pro v4.27.136
- **Animation Polish**: Slowed the crosshair pulse effect by 50% for a smoother, more premium visual experience.
- **Deep Diagnostics**: Overhauled the Debug tab with a "Live Diagnostics" console. It now displays real-time match ratios and active logic states (Diving/Gliding).
- **Hardened Debug Logic**: Ensured all debug telemetry data is correctly synchronized between the core engine and the UI.

### BetterAngle Pro v4.27.135
- **Shortcut Icon Refresh Enforcement**: Re-configured the installer to bundle `icon.ico` as a standalone asset and explicitly link Desktop/Start Menu shortcuts to it. This bypasses the Windows executable icon cache and ensures the new branding is visible immediately upon installation/update.
- **Branding Consistency Fix**: Hardened the installer configuration to resolve persistent old-logo visibility on the user's desktop.

### BetterAngle Pro v4.27.134
- **Maintenance**: Automated technical version alignment and synchronization.

### BetterAngle Pro v4.27.133
- **Hotkey Lifecycle Optimization**: Ensured all hotkey configuration changes (Toggle, ROI, Crosshair, Zero-Point, Debug) are automatically saved to both the user profile and global settings instantly.
- **Workflow Stabilization**: Established a strict "Pull-Rebase-Push" protocol to prevent repository desynchronization and ensure atomic updates.

### BetterAngle Pro v4.27.132
- **Maintenance**: Technical version alignment and synchronization.

### BetterAngle Pro v4.27.131
- **Maintenance**: Auto-increment stabilization.

### BetterAngle Pro v4.27.130
- **Trigger Calibration Consolidation**: Replaced individual Glide and Freefall threshold sliders with a unified "Dive/Glide Match" control.
- **Auto-Save Sensitivity**: Sensitivity changes are now saved instantly to user settings for a smoother configuration experience.
- **UI Streamlining**: Reduced visual clutter by removing legacy threshold controls from the Debug tab.
- **Enhanced Keybind Support**: Integrated user-contributed expansion for F13-F24, Multimedia, and OEM keys.

### BetterAngle Pro v4.27.129
- **Release Payload Optimization**: Removed the standalone `uninstaller.exe` from GitHub Releases to streamline the distribution. All uninstallation logic is now handled internally by the main setup.
- **Deep Uninstall Implementation**: Re-configured the uninstaller to perform a comprehensive system purge. Orchestrated the recursive deletion of both the installation directory and the `%LOCALAPPDATA%\BetterAngle` configuration folder.

### BetterAngle Pro v4.27.128
- **Maintenance**: Automated technical version alignment and synchronization.

### BetterAngle Pro v4.27.127
- **Full Reset on Update**: The application now automatically purges all settings, profiles, and logs from `AppData` when an update is applied. This ensures a clean state and prevents legacy configuration issues from affecting new installs.
- **Improved Update Automation**: Refined the post-update PowerShell script to handle high-reliability directory cleanup.

### BetterAngle Pro v4.27.126
- **Universal Branding Enforcement**: Explicitly set the application-wide icon in the core C++ code, forcing the Windows taskbar and window title bars to show the correct, new branding.
- **High-Resolution Asset Sweep**: Verified and enforced the new blue circular logo across all platform entry points (Installer, Desktop Shortcut, System Tray).

### BetterAngle Pro v4.27.125
- **Universal Branding Enforcement**: Explicitly set the application-wide icon in the core C++ code, forcing the Windows taskbar and window title bars to show the correct, new branding.
- **High-Resolution Asset Sweep**: Verified and enforced the new blue circular logo across all platform entry points (Installer, Desktop Shortcut, System Tray).

### BetterAngle Pro v4.27.124
- **CI/CD Automation Overhaul**: Integrated `bump_version.ps1` into the GitHub Actions pipeline. The system now automatically increments the patch version and generates release notes from commit logs on every push to `main`.
- **Infrastructure Synchronization**: Updated `.github/workflows/msbuild.yml` with full history fetching (`fetch-depth: 0`) and automated commit-back logic.

### BetterAngle Pro v4.27.123
- **MOC Linker Fix**: Added [`include/shared/BetterAngleBackend.h`](include/shared/BetterAngleBackend.h) to [`PROJECT_SOURCES`](CMakeLists.txt:13) so CMake's AUTOMOC processes the `Q_OBJECT` macro and generates the required `moc_BetterAngleBackend.cpp`, resolving 10 unresolved external symbol linker errors.

### BetterAngle Pro v4.27.122
- **LogWindowInfo Signature Fix**: Updated the two [`LogWindowInfo()`](include/shared/EnhancedLogging.h:35) call sites in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:620) to pass the required `label` parameter matching the two-argument signature, fixing the last CI compile failure.
- **Diagnostic Logging Preserved**: The input-gate state logging in [`MsgWndProc()`](src/main_app/BetterAngle.cpp:222) remains intact for root-cause confirmation once CI is green.

### BetterAngle Pro v4.27.121
- **Final Diagnostic Build Fix**: Updated the remaining [`LogWindowInfo()`](include/shared/EnhancedLogging.h:35) call sites in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:619) and [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:642) to pass the required label parameter, fixing the last CI compile failure in the diagnostic branch.
- **Diagnostic Logging Preserved**: The input-gate state logging in [`MsgWndProc()`](src/main_app/BetterAngle.cpp:222) remains intact so the next green release can finally produce the evidence needed to isolate the broken normal-mode gate.

### BetterAngle Pro v4.27.120
- **Qt Package Fix for GitHub Actions**: Corrected [`find_package()`](CMakeLists.txt:11) and [`target_link_libraries()`](CMakeLists.txt:32) to use Qt6 `QuickControls2` instead of the nonexistent `Controls2` package/target names, fixing the CI configure failure.
- **Diagnostic Build Continuation**: This preserves the input-gate diagnostic logging in [`MsgWndProc()`](src/main_app/BetterAngle.cpp:222) while restoring a green GitHub Actions release path.

### BetterAngle Pro v4.27.119
- **Diagnostic Input-Gate Logging**: Added state-change logging in [`MsgWndProc()`](src/main_app/BetterAngle.cpp:222) to report whether normal angle accumulation is being blocked by [`IsFortniteForeground()`](src/shared/Input.cpp:120), [`IsCursorCurrentlyVisible()`](src/shared/Input.cpp:127), or [`g_debugMode`](src/main_app/BetterAngle.cpp:229).
- **Diagnostic Build Fix**: Corrected the new gate log call in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:241) to use the current narrow-string [`LogMessage()`](include/shared/EnhancedLogging.h:52) signature so the diagnostic release compiles in GitHub Actions.

### BetterAngle Pro v4.27.115
- **Updater Fallback Fix**: Updated [`src/shared/Updater.cpp`](src/shared/Updater.cpp:147) to report when update checks fail because the GitHub release stream is misconfigured, and changed the manual fallback link in [`ApplyUpdateAndRestart()`](src/shared/Updater.cpp:181) to the full releases page.
- **Version Metadata Sync**: Synced [`VERSION`](VERSION), [`CMakeLists.txt`](CMakeLists.txt:2), and fallback constants in [`include/shared/State.h`](include/shared/State.h:20) to `4.27.115`.

### BetterAngle Pro v4.27.114
- **Windows CI Fix**: Renamed the logger enum members in [`include/shared/EnhancedLogging.h`](include/shared/EnhancedLogging.h:13) to avoid the Win32 `ERROR` macro collision that broke MSVC parsing in the previous release.
- **Build Recovery**: Updated the logger implementation in [`src/shared/EnhancedLogging.cpp`](src/shared/EnhancedLogging.cpp:170) and startup integration in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:512) so the restored logging system compiles cleanly on Windows.
- **Version Metadata Sync**: Synced [`VERSION`](VERSION), [`CMakeLists.txt`](CMakeLists.txt:2), and fallback constants in [`include/shared/State.h`](include/shared/State.h:20) to `4.27.114`.
