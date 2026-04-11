### BetterAngle Pro v4.22.7
- **Build System Patch:** Resolved a compilation error in `BetterAngle.cpp` by adding the missing `<filesystem>` header. This ensures the SHIFT-Reset settings recovery feature works as intended.

### BetterAngle Pro v4.22.6
- **FIX: Critical Startup Failure:** Resolved a logic conflict where the application was skipping the Dashboard/Wizard loading if the Splash screen was enabled. UI elements are now guaranteed to load.
- **NEW: Recovery Mode (SHIFT + Start):** Holding the **SHIFT** key while launching the application will now offer to reset all settings to defaults. Use this if the app is crashing or failing to start.
- **Robust Initialization:** Upgraded the application startup logic to use more reliable Win32 process arguments, preventing "ghost" background processes.

### BetterAngle Pro v4.22.5
- **Build System Patch:** Resolved a compilation error in `BetterAngle.cpp` by restoring the missing screen dimension identifiers (`screenW`, `screenH`) required for multi-monitor HUD initialization.
- **Uninstaller Hotfix:** Corrected a build failure in the uninstaller by adding the missing `<shlobj.h>` header, enabling successful desktop shortcut removal.

### BetterAngle Pro v4.22.4
- **Multi-Monitor Visibility Fix:** Implemented dynamic monitor centering for the Splash, Wizard, and Dashboard screens, ensuring they no longer appear off-screen on multi-monitor setups.
- **Enhanced Uninstaller:** The uninstaller now fully removes the application state, including AppData profiles, hidden setup markers, and the Desktop shortcut.
- **Startup Engine Optimization:** Refined the QML initialization sequence to prevent race conditions during the Splash-to-Wizard transition.

### BetterAngle Pro v4.22.3
- **Build System Patch:** Resolved a compilation error in `BetterAngle.cpp` by adding the missing `<QTimer>` header, ensuring correct handling of deferred initialization.
- **Hotfix:** Synchronized core framework headers to resolve CI/CD pipeline compilation errors.

### BetterAngle Pro v4.22.2
- **Documentation Sync:** Updated `RELEASE_NOTES.md` and repository versioning to align with the core multi-monitor and keybind functionality updates.

### BetterAngle Pro v4.22.1
- **Multi-Monitor Crosshair Visibility:** Recalibrated crosshair centering to use the Primary Monitor's center instead of the entire desktop span, preventing it from being hidden between screens.
- **Hotkey Reliability Fix:** Resolved a "tug-of-war" bug where background timers were resetting the crosshair state 60 times a second.
- **Double-Toggle Prevention:** Cleaned up redundant key listeners to ensure a single F10 press only toggles the crosshair once.
- **Persistence Hardening:** All crosshair and UI toggles now save immediately to the active profile on change.

### BetterAngle Pro v4.22.0
- **Keybind Customization Fix:** Resolved a focus bug where the input field would lose focus prematurely when pressing modifier keys (Ctrl/Shift/Alt).
- **Usability Enhancement:** Added custom interaction zones (MouseArea) to all keybind fields for much more reliable click-to-edit behavior.
- **Visual Feedback:** All keybind fields now provide real-time feedback ("Press key...") and highlight in gold when active.

### BetterAngle Pro v4.21.3
- **Build System Patch:** Replaced `main()` with `WinMain()` in the uninstaller utility to resolve `LNK2019` entry point errors during compilation.
- **CI/CD Reliability:** Verified `WIN32` executable flags for all binary targets to ensure headless execution on Windows.

### BetterAngle Pro v4.21.2
- **Dynamic Bind Refresh:** Keybinds now re-apply instantly without requiring a software restart.
- **Async Startup Sequence:** Refined the splash-to-dashboard transition to ensure the main window only appears after assets have fully loaded.
- **Installer Stability:** Patched setup wizard crashes by enforcing AppData directory creation prior to file deployment.
- **Remote Build Pipeline:** Offloaded all compilation to GitHub Actions for secure, environment-consistent binaries.
- **Professional Uninstaller:** Added a standalone cleanup utility to completely remove all application data and user profiles.

### BetterAngle Pro v4.21.1
...
