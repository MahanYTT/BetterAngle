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
