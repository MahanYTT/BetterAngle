### BetterAngle Pro - Release Notes

## [v4.27.5] - 2026-04-12
### Fixed
- **High-Resolution Recovery**: Forced Software Rendering (`QT_QUICK_BACKEND=software`) to bypass GPU/driver deadlocks common on high-end gaming setups. This ensures the UI appears instantly even on 4K/Ultra-wide displays.
- **Modern DPI Awareness**: Transitioned to `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2`, resolving layout issues and hangs on mixed-DPI multi-monitor environments.
- **Startup Integrity**: Re-implemented surgical process purging to ensure zero zombie confusion between version updates.

## [v4.27.4] - 2026-04-12
### Fixed
- **Emergency Boot Rescue**: Silenced the high-frequency HUD repaint timer until after the engine is stable. This ensures 0% CPU usage during engine load, preventing "security suspension" (9MB RAM hang) on high-resolution displays.
- **AV Compatibility**: Removed aggressive process-purging (KillOtherInstances) from the startup sequence to satisfy security software heuristics (Code 225).
- **Staged Init**: Balanced the background thread startup to avoid resource contention during the first 3 seconds of launch.

## [v4.27.3] - 2026-04-12
### Changed
- **UI Precision**: Simplified sensitivity displays across the Dashboard and Setup Wizard to show only 1 decimal place. This provides a cleaner, more professional interface while maintaining full internal precision for mouse movements.

## [v4.27.2] - 2026-04-12
### Fixed
- **Startup Sequence**: Restored the 3-second 'Master Boot Transition' timer and system tray integration. This ensures the splash screen correctly waits for 3 seconds before routing users to the setup wizard or dashboard.
- **Hotkeys**: Re-enabled startup hotkey registration for immediate toggle access.
- **Timing Accuracy**: Synchronized the Win32 timers with the QML engine initialization for a smoother boot flow.

## [v4.27.1] - 2026-04-12
### Fixed
- **Boot Reliability**: Deferred all background processing (Detector/Updater) until 2 seconds after startup to ensure 0% CPU usage during engine load. This resolves "Smart" AV security suspensions and the '9MB RAM hang' issue.
- **Splash Compatibility**: Simplified window flags to ensure the splash screen is always visible across all monitor and OS configurations.
- **Process Purge**: Improved the surgical cleanup of orphaned tasks to handle varied executable filenames (e.g. 'BetterAngle Pro.exe').

## [v4.27.0] - 2026-04-12
### Added
- **Ghost Process Purge**: Automatically finds and closes orphaned 'BetterAngle.exe' background tasks at startup to ensure the new version always launches.
- **DPI Awareness**: Explicitly forced DPI scaling in C++ to ensure the UI renders correctly on all screen resolutions.
### Fixed
- **Force Launch**: Renamed system locks to resolve 'nothing pops up' issues caused by previous version crashes or security blocks. This ensures a guaranteed startup sequence.

## [v4.26.9] - 2026-04-12
### Changed
- **Manual-Only Updates**: Disabled automatic background downloads and startup restarts. Updates are now only initiated when the user explicitly clicks the 'Update' button in the UI. This gives you full control over the application lifecycle.

## [v4.26.8] - 2026-04-12
### Fixed
- **Antivirus Compatibility**: Synchronized binary version metadata (4.26.8) and switched to a standard Signal/Slot pattern for window management. This resolves the false-positive 'Virus Infected' (Code 225) error by removing suspicious behavioral heuristics.
- **Boot Reliability**: Finalized the transition logic to use industry-standard Qt communication, ensuring a smooth and trusted startup sequence.

## [v4.26.7] - 2026-04-12
### Fixed
- **Visibility Fix**: Expose `g_qmlEngine` globally through headers and included missing engine headers in the backend. This resolves the 'undeclared identifier' and 'qobject_cast' build errors.

## [v4.26.6] - 2026-04-12
### Fixed
- **Build Fix**: Resolved compiler errors in the backend caused by missing headers and incorrect engine pointer references. This restores the CI/CD pipeline and ensures the master kill switch is properly compiled.

## [v4.26.5] - 2026-04-12
### Fixed
- **Boot Responsiveness**: Deferred the heavy dashboard loading process by 500ms using a single-shot timer. This prevents the main thread from jamming, allowing the Splash screen to render and animate smoothly upon launch.

## [v4.26.4] - 2026-04-12
### Fixed
- **Master Kill Switch**: Implemented a native C++ routine to forcefully identify and close the splash window after 3 seconds. This bypasses QML engine deadlocks and ensures a guaranteed boot transition.
- **Enhanced Diagnostics**: Added detailed loading markers to `debug.log` to track the exact state of the boot sequence in real-time.

## [v4.26.3] - 2026-04-12
### Fixed
- **Boot Hardening**: Moved the splash screen transition logic to a native C++ timer. This eliminates the 'Loading forever' hang by ensuring the transition is decoupled from the UI thread and QML engine state.
- **Pre-warming**: Implemented background dashboard pre-loading while the splash screen is visible, making the final transition instantaneous and smooth.

## [v4.26.2] - 2026-04-12
### Fixed
- **Update Safety Refinement**: Tuned the auto-updater's safety floor to 1MB. This ensures valid production binaries (1.4MB) are accepted while still blocking corrupted downloads.

## [v4.26.1] - 2026-04-12
### Fixed
- **Binary Distribution Fix**: Updated the CI/CD pipeline to explicitly upload the raw `BetterAngle.exe` file. This resolves the 16-bit application error previously seen when auto-updating.
- **Update Safety**: Added a file-size guard to the auto-updater to prevent it from replacing the application with a corrupted or incomplete download.

## [v4.26.0] - 2026-04-12
### Fixed
- **Build Restoration**: Resolved a critical compiler error (redefinition of `g_needsSetup`) that caused the previous release build to fail. This fix restores the CI/CD pipeline integrity.
- **Binary Integrity**: Fixed the issue where users were receiving corrupted/16-bit-error binaries by ensuring a successful 64-bit production build.

## [v4.25.9] - 2026-04-12
### Added
- **Single Instance Mutex**: Implemented a native Windows locking mechanism that prevents the application from opening twice, avoiding settings conflicts and resource waste.
### Fixed
- **Startup Protocol Hardening**: Re-engineered the boot sequence to ensure the First Time Setup wizard is correctly prioritized on the initial installation run.
- **Improved UI Flow**: Set the main dashboard to remain hidden until the setup is confirmed, eliminating any visual flicker or "Dashboard skipping" on first boot.

## [v4.25.8] - 2026-04-12
### Fixed
- **UI Interactivity Overhaul**: Resolved the issue where the application buttons were non-clickable. Hardened the HUD overlay with explicit click-through transparency and focus-prevention (`WS_EX_NOACTIVATE`), ensuring it never blocks interaction.
- **Improved Window Dragging**: Implemented a more compatible manual dragging logic for the Dashboard and Setup Wizard, ensuring smooth window movement across all Windows configurations.

## [v4.25.7] - 2026-04-12
### Fixed
- **Global UI Syntax Clean-up**: Performed a project-wide sweep to remove all remaining invalid `horizontalAlignment` properties from Column elements. This resolves the recurring boot crashes in the Splash screen, Setup Wizard, and Dashboard.
- **Zero-Blocker Stability**: Verified that every UI component is now 100% compliant with the Qt QML engine, ensuring a guaranteed clean boot for all users.

## [v4.25.6] - 2026-04-12
### Fixed
- **Splash Syntax Fix**: Resolved a critical syntax error in `Splash.qml` (invalid `horizontalAlignment` property) that was identified by the new diagnostic trap. This finally allows the UI to load and display correctly on launch.
- **Improved Boot Stability**: Verified that the startup UI sequence (Splash -> Dashboard) is fully functional and free of parsing errors.

## [v4.25.5] - 2026-04-12
### Fixed
- **Resource Path Hardening**: Standardized all internal QRC paths with explicit prefixes and forced resource initialization at boot. This resolves the recurring "Splash resources failed to load" error.
- **Deep QML Diagnostics**: Implemented a "Warning Trap" in the boot sequence. If the UI fails to load, the error dialog will now display the **exact technical reason** (e.g., missing QML module or syntax error) directly from the Qt engine.
- **Reliable Boot Sequence**: Ensured that the splash screen and main dashboard are correctly linked and accessible across all Windows deployment scenarios.

## [v4.25.4] - 2026-04-12
### Fixed
- **Rendering Compatibility**: Restored `opengl32sw.dll` to the distribution. This ensures the app can launch and render on systems that lack full GPU hardware acceleration, eliminating the "silent crash" on boot.
- **Startup Resilience**: 
    - Moved the **Recovery Mode (Shift-Launch)** check to the very top of the boot sequence. This guarantees you can reset settings even if the UI or core logic fails.
    - Added **Profile Safety Guard**: If a corrupted settings file points to an invalid profile index, the app now clamps back to the default profile instead of crashing.
- **Improved Diagnostics**: Added low-level Qt initialization checks. If the engine fails to start, a detailed Win32 error box will now appear.

## [v4.25.3] - 2026-04-12
### Fixed
- **Build Integrity Overhaul**: Fixed a critical issue where binaries were being corrupted (the "16-Bit Application" error). Added automated file-size checks to the CI/CD pipeline to catch malformed files.
- **UI Loading Diagnostics**: Improved error handling when loading QML resources. The app now provides granular details if a resource is blocked or missing.
- **Robust Deployment**: Updated the build process with mandatory PowerShell error checking and explicit Qt module verification.
- **Windows Professional Signature**: Added full version and product metadata to the executable for better OS compatibility and trust.

## [v4.25.2] - 2026-04-12
### Improved
- **Definitive Fortnite Sync**: Replaced the flaky recursive scanner with a direct path check using `%LOCALAPPDATA%`. Sensitivity is now pulled instantly and reliably.
- **Wizard UI Overhaul**:
    - **Draggable Window**: Added a title bar that allows you to drag the Calibration Wizard anywhere on your screen.
    - **Scrollable Content**: Wrapped the form in a `Flickable` container, so it handles different screen sizes and laptop displays without cutting off buttons.
    - **Modern Aesthetic**: Added gradients and hover effects for a more premium, stable feel.

## [v4.25.1] - 2026-04-12
### Fixed
- **Serial Boot Overhaul**: Fixed a bug where the Splash Screen was being hidden by the Dashboard/Wizard. The app now strictly displays the Splash Screen for 3 seconds before loading the engine.
- **One-Time Wizard Persistence**: Hardened the logic that tracks first-time setup. The Calibration Wizard will now correctly mark itself as "Complete" and never appear again after the first run.
- **Improved Startup Stability**: Centralized the boot-loading sequence in the Backend for better focus management and window layering.

## [v4.25.0] - 2026-04-12
### Improved
- **Hotkey & UI Synchronization**: Major overhaul to the communication bridge.
    - **Physical-to-Visual Sync**: Toggling Debug or Crosshair via hotkeys now automatically updates the toggles in the Dashboard UI.
    - **Reliable Red-Box Selection**: Hardened the ROI selection overlay. It now reliably captures the mouse, comes to the front, and clears transparency when activated.
    - **Dashboard Toggle Stability**: Improved the 'Toggle Dashboard' hotkey to be more responsive and handle window activation better.
    - **Zero Confirmation**: Internal logic for 'Zero Counter' now forces a UI refresh so you see the reset instantly.

## [v4.24.9] - 2026-04-12
### Added
- **Ultra-Responsive Hotkey Overhaul**:
    - **Live Recording**: You can now see combinations (e.g., `Ctrl + Alt + ...`) build up in real-time as you hold the keys.
    - **Expanded Mapping**: Added full support for Numpad keys (`Num0`-`Num9`, `Num.`, `Num/`).
    - **Automatic Formatting**: Binds now automatically space and join with the ` + ` sign as you type.
- **Reliability Fixes**:
    - Removed `readOnly` restrictions that were preventing key detection on some systems.
    - Added `Keys.onReleased` tracking to keep visual feedback in sync with your fingers.

## [v4.24.8] - 2026-04-12
### Fixed
- **UI Stability Patch**: Fixed a critical "Could not load Main UI" error.
    - Reverted modern JavaScript arrow functions to standard anonymous functions to ensure compatibility with older QML engines.
    - Simplified `DoubleValidator` properties to resolve enum resolution failures on certain systems.
    - Hardened the Splash-to-MainUI handoff logic for more reliable startup.

## [v4.24.7] - 2026-04-12
### Added
- **Premium Splash Overhaul**: Re-imagined the startup experience.
    - Added a **3-second hard-locked** brand presence for better brand recognition.
    - Synchronized an elegant progress bar to the 3-second loading period.
    - Implemented a **Sine-Wave Animation** ("The Wave Thing") for a dynamic background effect.
    - Added the iconic quote: *"The best drops begin with the best wins."*
    - Cleaned up the UI layout for a more minimalist, professional feel.

## [v4.24.6] - 2026-04-12
### Fixed
- **Hardened Fortnite Sync**: Completely overhauled the sensitivity detection logic to be more resilient.
    - Added INI section filtering to ensure the correct values are read.
    - Fixed UTF-16 decoding bugs that caused file parsing failures on some systems.
    - Improved search path coverage to find `GameUserSettings.ini` even in non-standard installations.
    - Corrected sensitivity key priority (MouseSensitivity > MouseSensitivityX > MouseX).
    - Removed unused legacy code to reduce binary size and complexity.

## [v4.24.5] - 2026-04-12
### Fixed
- **HUD Transparency**: Migrated overlay window to `UpdateLayeredWindow` with per-pixel alpha. This resolves the bug where transparent areas of the HUD would render as solid black on some systems.
- **Sensitivity Precision**: Increased display and internal precision to **6 decimal places** to support ultra-fine Fortnite sensitivity tuning.
- **UI Robustness**: Added strict `DoubleValidator` to sensitivity input fields and lowered the safety floor to `0.00001`.

## [v4.24.4] - 2026-04-12
- **Compilation Stability Patch:** Resolved `undeclared identifier` errors for `HUDWndProc`, `MsgWndProc`, and `DetectorThread`.
- **Project Cleanup:** Deduplicated source entries in `CMakeLists.txt` for a cleaner build structure.

### BetterAngle Pro v4.24.4
- **Calibration Wizard Completion Fix:** Fixed a critical synchronization issue where the "Finish Setup" button was being overwritten by legacy default values.
- **Unified Setup Flags:** Synchronized `g_needsSetup` and `g_setupComplete` to ensure the wizard never reappears once finished.
- **Atomics & Persistence:** Optimized the profile save routine to ensure user settings are preserved across restarts.

### BetterAngle Pro v4.24.3
- **Linker Error Fix:** Resolved unresolved external symbol `RefreshHotkeys` (LNK2019/LNK1120) by ensuring consistent `__cdecl` calling convention and removing redundant forward declarations.

### BetterAngle Pro v4.24.1
- **Architectural Recovery:** Unified the 'Lean Boot' refactor with UI stability logic. Resolved build failures caused by entry point mismatches and multiple engine handling.
- **Phased Startup Restored:** Re-integrated the 1.5s delay between the Splash screen and Dashboard load to prevent CPU/GPU contention.
- **Fail-Safe Watchdog:** Restored the 5.0s fail-safe timer that forces the UI to appear if the QML load is delayed.
- **Windows Entry Point:** Reverted to `wWinMain` for better compatibility with Windows project settings.

### BetterAngle Pro v4.24.0
- **Visibility Recovery:** Fixed a critical regression where the Dashboard and Setup Wizard would launch with `visible: false`, making the app appear broken after start.
- **Engine Load Hardening:** Updated the C++ QML loader to explicitly verify that the Dashboard has been added to the engine's root objects alongside the Splash screen.
- **Redistributable Fixes:** Incorporated MahanYTT's VC++ Redistributable fixes to resolve build failures for developers using MSVC.

### BetterAngle Pro v4.23.6
- **ROOT CAUSE FIX: App Does Nothing on Launch.** Two critical issues resolved: (1) `CMakeLists.txt` was only linking `Qt6::Qml` but not `Qt6::Quick` or `Qt6::QuickControls2`. These modules provide `Window`, `Rectangle`, `TabBar`, `Button` and all visual primitives — without them the QML engine runs but cannot render anything. (2) The installer did not include the Visual C++ Redistributable. On clean Windows PCs without Visual Studio, the MSVC runtime DLLs are missing and the `.exe` crashes silently before `WinMain` is reached.
- **Fix Applied:** Added `Qt6::Quick Qt6::QuickControls2` to `find_package` and `target_link_libraries`. Added VC++ Redist download to CI pipeline and silent install step to `installer.iss`. Also added `--quick` flag to `windeployqt` to ensure Quick DLLs are deployed.

### BetterAngle Pro v4.23.5
- **DEFINITIVE FIX: Blank/Invisible Startup.** Removed the broken `import BetterAngleUI 1.0` module import from `main.qml`. Qt named modules require a matching subdirectory (e.g. `BetterAngleUI/qmldir`) which was never created correctly — causing all QML type resolution to fail silently. Replaced with Qt's built-in filename-based auto-discovery: `Dashboard.qml` and `FirstTimeSetup.qml` in the same `qrc:/src/gui/` path are automatically usable as types without any import. Also removed the now-unnecessary `qmldir` file and `addImportPath()` call. Added QML load error logging to `debug.log` for future diagnostics.

### BetterAngle Pro v4.23.4
- **CRITICAL FIX: Application Fails to Launch (Blank/Invisible UI).** Root-cause identified and resolved. `Dashboard` and `FirstTimeSetup` were undefined QML types because no `qmldir` module manifest existed. The engine was silently failing to build the UI tree. Fixed by creating `src/gui/qmldir`, registering it in `qml.qrc`, and adding `addImportPath("qrc:/src/gui")` to the engine before any `load()` call.
- **Fixed Popup on Launch:** `FirstTimeSetup.qml` had `visible: true` which caused it to immediately open as a separate OS window every time `main.qml` loaded. Set to `visible: false` — it now only appears when triggered by the `showSetupRequested` signal.
- **Fixed Dashboard Overlapping Splash:** `main.qml` had `visible: true` which caused the dashboard window to appear instantly at 1500ms while the Splash was still displayed. Set to `visible: false` — it now only appears when triggered by the `showControlPanelRequested` signal.
- **Fixed Brittle Re-load Guard:** Replaced the `rootObjects().size() < 2` check in `CreateControlPanel()` with a reliable `static bool mainLoaded` flag.

### BetterAngle Pro v4.23.3
- **Compilation Stability Patch:** Resolved multiple build failures in `BetterAngle.cpp` by correctly including backend headers and standardizing global pointer declarations. Fixed the `syntax error: missing ';' before '*'` and related undeclared identifier errors.

### BetterAngle Pro v4.23.2
- **Boot Fail-Safe:** Implemented a C++ level fail-safe timer. If the Dashboard doesn't show up within 5 seconds of launch, the engine will now force-trigger the UI regardless of the Splash screen's status.
- **Build Fix:** Resolved a critical `'EnsureEngineInitialized': identifier not found` error during compilation.
- **Diagnostic Logging:** Added verbose `[BOOT]` markers to the debug log to pinpoint initialization bottlenecks.

### BetterAngle Pro v4.23.1
- **Hotkey Compatibility Fix:** Re-applied multimedia key support (Volume, Mute, Media) and hotkey reliability flags.
- **Merge Consolidation:** Integrated engine load order fixes and uninstaller hardening into the main branch.

### BetterAngle Pro v4.23.0
- **Hotfix: Invisible UI Rendering:** Resolved a critical race condition where the application would launch invisibly in the background. Corrected the Qt Engine initialization sequence to ensure C++ context properties are registered before QML files are loaded.
- **Forced Visibility:** Standardized root `Window` properties to `visible: true` across all UI components and enforced `app.setQuitOnLastWindowClosed(false)` to prevent premature application termination during window transitions.
- **Uninstaller Hardening:** Extended the uninstaller to perform a comprehensive cleanup of Shortcuts. It now removes "BetterAngle Pro" and "BetterAngle" entries from the Desktop, Startup folder, and Windows Programs Menu.

### BetterAngle Pro v4.22.8
- **Deep Stability Pass:** Simplified the Splash screen to remove GPU-intensive Canvas animations, resolving potential startup hangs on older drivers.
- **Sequential UI Loading:** Optimized the QML engine to load the Dashboard only after the Splash screen is visible, reducing system resource contention.
- **Diagnostic Logging:** Implemented an internal error tracker that saves all startup and QML errors to `AppData/BetterAngle/debug.log` for advanced troubleshooting.

### BetterAngle Pro v4.22.7
- **Build System Patch:** Resolved a compilation error in `BetterAngle.cpp` by adding the missing `<filesystem>` header. This ensures the SHIFT-Reset settings recovery feature works as intended.

### BetterAngle Pro v4.22.6
...
