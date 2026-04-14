### BetterAngle Pro v4.27.118
- **Universal Branding Overhaul**: Replaced all application assets with the new high-fidelity logo. Updates propagate across the Desktop Shortcut, Tray Icon, Taskbar, and Dashboard.
- **Legacy UI Removal**: Removed the Splash Screen and Setup Wizard as requested to provide a direct-to-dashboard startup experience.
- **Feature Recovery**: Re-applied the Cartesian Coordinate system (+Y moves UP) and the dual-mode Logging subsystem restoration.

### BetterAngle Pro v4.27.115
- **Updater Fallback Fix**: Updated [`src/shared/Updater.cpp`](src/shared/Updater.cpp:147) to report when update checks fail because the GitHub release stream is misconfigured, and changed the manual fallback link in [`ApplyUpdateAndRestart()`](src/shared/Updater.cpp:181) to the full releases page.
- **Version Metadata Sync**: Synced [`VERSION`](VERSION), [`CMakeLists.txt`](CMakeLists.txt:2), and fallback constants in [`include/shared/State.h`](include/shared/State.h:20) to `4.27.115`.

### BetterAngle Pro v4.27.114
- **Windows CI Fix**: Renamed the logger enum members in [`include/shared/EnhancedLogging.h`](include/shared/EnhancedLogging.h:13) to avoid the Win32 `ERROR` macro collision that broke MSVC parsing in the previous release.
- **Build Recovery**: Updated the logger implementation in [`src/shared/EnhancedLogging.cpp`](src/shared/EnhancedLogging.cpp:170) and startup integration in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:512) so the restored logging system compiles cleanly on Windows.
