### BetterAngle Pro v4.21.10
- **Async Framework Optimization:** Implemented `QTimer::singleShot` deferral for heavy backend initialization, ensuring the Splash Screen renders instantaneously without being blocked by resource loading.
- **QML Setup Workflow:** Replaced the legacy Win32 setup dialog with a modern, high-fidelity `FirstTimeSetup.qml` component featuring strict OS window flags to prevent "hide-to-tray" confusion during initial calibration.
- **Safety Exit Policy:** Configured the Setup Wizard to quit the application entirely if closed prematurely, ensuring a consistent and clean state for first-time users.

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
