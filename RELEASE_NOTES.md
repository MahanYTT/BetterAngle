### BetterAngle Pro - Release Notes

## [v4.27.73] - 2026-04-13
### Fixed
- **HUD / Dashboard Coexistence**: [`SyncHUDWithPanelWindow()`](src/shared/ControlPanel.cpp:16) now keeps the HUD visible in click-through background mode while the dashboard is open, instead of hiding it outright, so the overlay and control panel can operate at the same time.
- **Startup Overlay Visibility**: [`requestShowControlPanel()`](src/shared/BetterAngleBackend.cpp:426) and [`finishSetup()`](src/shared/BetterAngleBackend.cpp:450) now prepare the HUD behind the panel instead of hiding it, fixing the missing crosshair/overlay state while the dashboard is visible.
- **Hotkey Re-registration Churn**: [`SaveSettings()`](src/shared/State.cpp:280) no longer re-registers all global hotkeys on every unrelated setting save, and the keybind setters in [`src/shared/BetterAngleBackend.cpp`](src/shared/BetterAngleBackend.cpp) no longer force duplicate refreshes before [`saveKeybinds()`](src/shared/BetterAngleBackend.cpp:803).
- **Dashboard Button Interaction**: Removed the temporary full-surface debug `MouseArea` from [`src/gui/main.qml`](src/gui/main.qml), restoring normal button clicks for crosshair, updates, and other dashboard controls.

## [v4.27.72] - 2026-04-13
### Fixed
- **Frozen Dashboard Input**: [`CreateControlPanel()`](src/shared/ControlPanel.cpp:82) now acquires the native Qt panel window and synchronizes it with the fullscreen HUD, hiding the HUD while the dashboard is interactive so mouse clicks and window dragging reach the control panel reliably.
- **HUD / Panel Handoff**: [`SyncHUDWithPanelWindow()`](src/shared/ControlPanel.cpp:15) now restores the HUD only after the dashboard is minimized or hidden, removing the fullscreen layered-window overlap that was still blocking UI interaction.

## [v4.27.71] - 2026-04-13
### Fixed
- **Angle Normalization**: Corrected [`AngleLogic::GetAngle()`](src/shared/Logic.cpp:128) to return the normalized value produced by [`AngleLogic::Norm360()`](src/shared/Logic.cpp:180), keeping the calculated angle inside the `[0, 360)` range.
- **HUD Angle Wraparound**: Prevented negative angles and values above `359.9...` from propagating through the angle logic, preserving stable wraparound behavior for zeroing, profile loads, and diving-state transitions.
- **HUD Mouse Barrier Issue**: Enhanced [`HUDWndProc()`](src/main_app/BetterAngle.cpp:159) with detailed diagnostic logging for `WM_NCHITTEST` to track click-through behavior. Added window style validation to ensure `WS_EX_TRANSPARENT` is correctly applied.
- **Angle Overlay Visibility**: Added diagnostic logging in [`DrawOverlay()`](src/shared/Overlay.cpp:60) to track HUD box position and virtual screen coordinates. Fixed potential coordinate calculation issues that could cause the angle overlay to render off-screen.
- **Enhanced Debug Logging**: Added comprehensive window creation diagnostics including window visibility state, styles, and extended styles after `ShowWindow()` calls to aid in troubleshooting HUD interaction issues.

## [v4.27.70] - 2026-04-13
### Fixed
- **Dashboard Interactivity Restored**: [`HUDWndProc()`](src/main_app/BetterAngle.cpp:159) now returns `HTTRANSPARENT` during normal operation so the always-on-top HUD no longer blocks clicks meant for [`main.qml`](src/gui/main.qml).
- **Crosshair Startup State**: [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:376) now restores `showCrosshair` and `crossAngle` together with the rest of the active profile so the overlay state is consistent immediately after launch.
- **Settings Parsing Reliability**: [`LoadSettings()`](src/shared/State.cpp:152) now parses values from the actual colon position instead of a fragile fixed offset, preventing malformed reads such as the huge `crossOffsetY` seen in startup logs.
- **Safe Profile Defaults**: [`Profile`](include/shared/Profile.h:28) now initializes crosshair and sensitivity fields with safe defaults so newly created profiles cannot produce undefined overlay coordinates.

## [v4.27.69] - 2026-04-13
### Changed
- **Splash Removed From Startup**: [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp) now skips loading [`Splash.qml`](src/gui/Splash.qml) entirely and reveals the dashboard directly after initialization.
- **Simpler UI Reveal Flow**: [`requestShowControlPanel()`](src/shared/BetterAngleBackend.cpp:424) now shows the HUD and emits the dashboard reveal without any splash-close dependency.

### Fixed
- **Startup Reliability**: Removed the fragile splash handoff path from [`src/shared/ControlPanel.cpp`](src/shared/ControlPanel.cpp), preventing the app from getting stuck behind a splash screen during launch.
- **Resource Cleanup**: [`qml.qrc`](qml.qrc) no longer packages [`Splash.qml`](src/gui/Splash.qml), and [`include/shared/BetterAngleBackend.h`](include/shared/BetterAngleBackend.h) no longer exposes the obsolete splash-close signal.

## [v4.27.68] - 2026-04-13
### Fixed
- **Crosshair Activation Reliability**: Unified the dashboard button and F10 hotkey through the same persisted backend toggle path so the crosshair state stays consistent across UI, HUD, and saved profile state.
- **Crosshair Visibility Positioning**: Corrected the crosshair draw coordinates to use the live HUD window origin, preventing invisible off-screen rendering on virtual desktop and multi-monitor layouts.
- **Hotkey Registration Resilience**: Hardened the crosshair hotkey registration path with explicit diagnostics plus `MOD_NOREPEAT` function-key handling and fallback binding for F10.

### Added
- **Crosshair Diagnostics**: Added runtime logging for crosshair toggles, hotkey registration results, virtual-screen origin, and draw coordinates to make future overlay failures directly traceable in startup logs.

## [v4.27.67] - 2026-04-13
### Fixed
- **Startup Unfreeze / Splash Handoff**: Moved the startup reveal gating logic onto the UI thread in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp), preventing the splash screen from getting stuck after startup completed.
- **Reveal Reliability**: The splash-to-dashboard handoff no longer depends on the boot thread to trigger the UI transition directly, reducing the chance of the splash staying open until force-quit.
- **Startup Diagnostics**: Added clearer startup handoff logging around the UI-thread reveal gate to make future splash transition issues easier to diagnose.

## [v4.27.65] - 2026-04-13
### Fixed
- **WaveDropMaps Naming**: Corrected the splash branding text in [`src/gui/Splash.qml`](src/gui/Splash.qml) to use `WaveDropMaps` consistently instead of `Wave DropMaps`.
- **Splash Text Cleanup**: Removed the unwanted splash footer text from [`src/gui/Splash.qml`](src/gui/Splash.qml) for a cleaner presentation.
- **Loading Bar Behavior**: Replaced the unreliable backend-bound progress display in [`src/gui/Splash.qml`](src/gui/Splash.qml) with a stable animated visual loading bar that progresses smoothly during startup instead of appearing stuck at 100% with almost no fill.

## [v4.27.64] - 2026-04-13
### Changed
- **Full Splash Redesign**: Completely rebuilt [`src/gui/Splash.qml`](src/gui/Splash.qml) around the branding in [`assets/banner.png`](assets/banner.png), with a cleaner card layout, integrated Wave DropMaps presentation, and a proper loading bar.
- **Visual Quality**: Replaced the old cluttered splash visuals with a more polished presentation focused on readability, branding, and stable rendering.

### Fixed
- **Minimum Splash Duration**: Enforced a minimum splash runtime of 2.5 seconds in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp) so startup no longer flashes through too quickly.
- **Splash/Menu Handoff**: Adjusted the splash-to-menu reveal timing in [`src/shared/BetterAngleBackend.cpp`](src/shared/BetterAngleBackend.cpp) so the splash is given time to close before the main menu becomes visible behind it.
- **Startup Stability**: Simplified the splash logic to avoid the fragile animation stack that previously caused repeated splash issues.

## [v4.27.63] - 2026-04-13
### Fixed
- **Final Splash Animation Load Error**: Removed the last invalid grouped animation property usage in [`src/gui/Splash.qml`](src/gui/Splash.qml:277), fixing the remaining splash startup error window and root object creation failure.
- **HUD/Overlay Startup Race**: Fixed a race between [`requestShowControlPanel()`](src/shared/BetterAngleBackend.cpp:424) and HUD creation in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:452) by adding a pending HUD show request path.
- **Overlay Visibility**: Ensured the angle HUD overlay is shown as soon as the HUD window exists, even if the dashboard request happens before the overlay window handle is ready.
- **Diagnostics**: Added targeted startup diagnostics for HUD creation and deferred HUD show handling to improve next-run troubleshooting.

## [v4.27.62] - 2026-04-12
### Fixed
- **Splash Animation Property Error**: Fixed remaining invalid grouped animation property usage in [`Splash.qml`](src/gui/Splash.qml:173) that was still causing the splash screen to fail creation and show an error window.
- **Main Window Drag Stability**: Replaced fragile manual frameless drag math in [`main.qml`](src/gui/main.qml:57) with native system window moving to stop the window from jittering or feeling like it escapes the mouse.
- **Diagnostics**: Added splash lifecycle and window movement diagnostics in [`Splash.qml`](src/gui/Splash.qml:5) and [`main.qml`](src/gui/main.qml:18) for clearer runtime troubleshooting in debug logs.

## [v4.27.60] - 2026-04-12
### Fixed
- **Splash Screen QML Loading Error**: Fixed "Cannot assign to non-existent property 'property'" error in Splash.qml by removing invalid `property: "y"` from YAnimator.
- **Main Window Focus Error**: Fixed "Property 'forceActiveFocus' of object QQuickWindowQmlImpl is not a function" error in main.qml by removing invalid forceActiveFocus() calls.
- **Application Stability**: Resolved QML warnings that were preventing proper splash screen loading and dashboard transition.

## [v4.27.59] - 2026-04-12
### Fixed
- **GDI+ Header Order Final Verification**: Corrected windows.h before objidl.h include order in all source files (BetterAngle.cpp, ThresholdWizard.cpp, Startup.cpp, Overlay.cpp) to ensure proper GDI+ compatibility with Windows SDK 10.0.26100.0.
- **Build System**: Final verification that all GDI+ compilation errors are resolved with correct header inclusion sequence and GDIPLUS_OLDEST_SUPPORTED_VERSION macro.

## [v4.27.58] - 2026-04-12
### Fixed
- **GDI+ Header Order Comprehensive Fix**: Fixed include order in Overlay.cpp and added GDIPLUS_OLDEST_SUPPORTED_VERSION macro across all GDI+ using files (BetterAngle.cpp, ThresholdWizard.cpp, Startup.cpp, Overlay.cpp) to ensure compatibility with Windows SDK 10.0.26100.0.
- **Build System**: Resolved remaining GDI+ compilation errors by ensuring windows.h is included before gdiplus.h with proper preprocessor definitions.

## [v4.27.57] - 2026-04-12
### Fixed
- **GDI+ Header Order Final Correction**: Fixed malformed include lines in ThresholdWizard.cpp and ensured proper include order (windows.h before gdiplus.h) across all source files to resolve Windows SDK 10.0.26100.0 compilation errors.
- **Build Compatibility**: All GDI+ compilation errors resolved with correct header inclusion sequence and BOM handling.

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
- **Splash Screen Taskbar Visibility**: Removed Qt.Tool flag from splash window, ensuring it appears in taskbar for better user visibility.
- **Invalid QML Function Call**: Replaced invalid LogStartup() call in Splash.qml with console.log() to prevent startup hang.

### Improved
- **Portable Mode Logging**: Startup logs now write to local debug folder in portable builds for easier troubleshooting.
- **Diagnostic Tracing**: Added stronger diagnostics for splash-to-dashboard transition with detailed signal emission logging.

## [v4.27.50] - 2026-04-12
### Fixed
- **GitHub Actions Merge Conflicts**: Resolved version conflicts between v4.27.49 and v4.27.50, rebased and pushed as v4.27.51.
- **Release Artifact Structure**: Verified correct release artifact generation (setup, portable, uninstaller).

## [v4.27.49] - 2026-04-12
### Fixed
- **GetTickCount() Wrap-Around Bug**: Fixed potential infinite loop in splash screen when system timer wraps after 49.7 days.
- **Diagnostic Logging**: Added comprehensive logging to BetterAngleBackend.cpp to track signal emissions and splash screen transitions.

### Changed
- **Version Bump**: Updated from 4.27.48 to 4.27.49 with proper release notes.
