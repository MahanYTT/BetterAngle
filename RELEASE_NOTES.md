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
