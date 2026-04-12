### BetterAngle Pro - Release Notes

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
