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
