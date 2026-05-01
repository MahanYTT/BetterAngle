### BetterAngle Pro v5.5.99
- Automated build release.

### BetterAngle Pro v5.5.98
- **Ghost-Walk Diagnostic Overlay**: Added four new rows to the debug overlay (Column 0, rows 13?16) that surface the live signals needed to root-cause the remaining ghost-walk cases without rebuilds.
  - **Typematic ? W**: ms gap between successive hardware Make events on `W`. Healthy fast-repeat sits around ~33 ms; values above ~60 ms (or `(press W)` when no recent press) mean the 200 ms post-restore collection window is too short to see typematic, causing the correction logic to false-fire a KEYUP and force a double-press.
  - **SafetyNet Fires**: count of `WM_USER+42` invocations (one per FOV transition).
  - **Sync Skips**: count of `SyncGamingKeysNitro` re-entrancy skips. Any nonzero value is a missed ghost-fix cycle from rapid back-to-back transitions.
  - **Corrections**: total Raw-Input corrections fired, plus the last corrected key and seconds since. If this increments during a transition where the character keeps walking, `SendInput` is not reaching Fortnite (EAC filtering suspected).
- **Test 4 Logging**: Added two `LOG_INFO` lines so the EAC-filtering test can be confirmed from `debug.log` directly:
  - `SafetyNet KEYUP fired for WASD` ? at the top of the `WM_USER+42` handler.
  - `Correction KEYUP for vk=%d` ? immediately after the correction `SendInput` in `SyncGamingKeysNitro`.
  - Reading the log: SafetyNet **and** Correction firing while the character still walks ? `SendInput` is being filtered (EAC). SafetyNet firing but no Correction during ghost walk ? typematic mistiming (the 200 ms window decided the user was still holding when they weren't).
- Diagnostic-only release; no behavioural changes to the lock pipeline, Shock & Restore, Fake Death, or correction logic.

### BetterAngle Pro v5.5.97
- Automated build release.

### BetterAngle Pro v5.5.96
- Automated build release.

### BetterAngle Pro v5.5.95
- Automated build release.

### BetterAngle Pro v5.5.94
- **UI Bug Fix**: Resolved a visual bug in the GDI+ debug overlay where the "Input State" text and "Version" text were overlapping due to a Y-coordinate collision.
- Automated build release.

### BetterAngle Pro v5.5.93
- **Performance Impact Diagnostics Update**: Removed the static GPU usage readout to maintain the clean UI layout while ensuring zero risk from anti-cheat systems. The Performance Impact panel now exclusively focuses on per-process CPU usage and RAM footprint.
- Automated build release.

### BetterAngle Pro v5.5.92
- Automated build release.

### BetterAngle Pro v5.5.91
- **Performance Impact Diagnostics**: Added a live Performance Impact tracking table to the Dashboard under the Debug menu. Tracks per-process CPU usage and RAM footprint natively via Windows API.
- Automated build release.

### BetterAngle Pro v5.5.90
- **Diagnostic Upgrade (Fake Death)**: Implemented an experimental fix for the Ghost Walk bug that tears down and reconstructs the Windows raw input pipeline mid-transition to flush stale `KeyUp` buffers. Includes a hard-coded safety net that manually emits `KeyUp` events for WASD on every transition.
- Automated build release.

### BetterAngle Pro v5.5.89
- **Dynamic Crosshair Centering**: The crosshair now perfectly centers itself on the active Fortnite game window, natively fixing misalignment issues for users playing in Windowed or Custom Stretched resolutions. 
- Automated build release.

### BetterAngle Pro v5.5.88
- **Input System Upgrade**: Completely rewrote the global hotkey engine to use manual hardware polling instead of Windows `RegisterHotKey`. This officially adds full support for mapping custom functions (Dashboard, Crosshair, etc.) to extra mouse buttons (Mouse4/Mouse5) which was previously blocked by the OS.
- Automated build release.

### BetterAngle Pro v5.5.87
- **Stability Fixes**: Resolved IsFortniteForeground data race and eliminated BlockInput thread queuing to prevent mouse freezes.
- **UI Enhancements**: Fixed taskbar disappearing on Alt-Tab and mouse freezes when the in-game map is opened.
- **Engine Safety**: Fixed undefined behavior in Qt initialization and invalid monitor fallback logic.
- Automated build release.

### BetterAngle Pro v5.5.86
- Improved detector and overlay logic for smoother performance
- Automated build release.

### BetterAngle Pro v5.5.85
- Automated build release.

### BetterAngle Pro v5.5.84
- Automated build release.

### BetterAngle Pro v5.5.83
- Automated build release.

### BetterAngle Pro v5.5.82
- Automated build release.

### BetterAngle Pro v5.5.80
- Automated build release.

### BetterAngle Pro v5.5.79
- Automated build release.

### BetterAngle Pro v5.5.78
- Automated build release.

### BetterAngle Pro v5.5.77
- Automated build release.

### BetterAngle Pro v5.5.76
- Automated build release.

### BetterAngle Pro v5.5.75
- Automated build release.

### BetterAngle Pro v5.5.74
- Automated build release.

### BetterAngle Pro v5.5.73
- Automated build release.

### BetterAngle Pro v5.5.72
- Automated build release.

### BetterAngle Pro v5.5.66
- Automated build release.

### BetterAngle Pro v5.5.65
- **Burst Restore Loop**: Implemented 3-pulse typematic simulation to force game engine acknowledgement.
- **Enhanced Thaw Wait**: Increased post-Shock delay to 100ms to guarantee hardware-to-OS synchronization.
- **Typematic Interleaving**: Added 10ms gaps between pulses to span across multiple engine polling ticks.

### BetterAngle Pro v5.5.64
- **Fixed Build Errors**: Resolved undeclared identifier and syntax errors in Overlay.cpp and Input.cpp.
- **Improved Overlay Stability**: Hardened DrawRow calls with explicit string conversions.

### BetterAngle Pro v5.5.63
- **Self-Correcting GhostFix**: Added a Verification step to confirm successful input restoration.
- **Enhanced Reliability**: Added SendInput error handling and diagnostic retry logic.
- **Latency Tracking**: Measured GhostFix execution time for performance tuning.

### BetterAngle Pro v5.5.62
- **Shock & Restore Engine**: Replaces unreliable delta-sync with unconditional release and hardware-thaw restoration.
- **Alt-Tab Lock Guard**: Extended BlockInput to focus switches to prevent FOV drift.
- **Serialized Execution**: Prevents rapid transition overlaps from corrupting input state.

### BetterAngle Pro v5.5.61
- Automated build release.

### BetterAngle Pro v5.5.60
- **Serialized Lock Guard**: Prevents missed GhostFixes during rapid transitions (back-to-back glide/dive).
- **Inventory Safety**: Skips key injection when Fortnite is not focused and enforces a strict whitelist for WASD + Space only.

### BetterAngle Pro v5.5.57
- Automated build release.

### BetterAngle Pro v5.5.56
- Automated build release.

### BetterAngle Pro v5.5.55
- Automated build release.

### BetterAngle Pro v5.5.54
- Automated build release.

### BetterAngle Pro v5.5.23
- **BlockInput Keyboard Ghosting Fixes**:
  - Skip BlockInput for alt-tab transitions (200ms cooldown only)
  - Added 50ms wait for natural async key state table thaw
  - Exclude spacebar from fallbacks to prevent FOV transition loops
  - Added keybd_event fallback for security software blocking SendInput
  - Enhanced ghostkey_fail.log diagnostics
  - Thread-safe BlockInput calls with mutex protection

### BetterAngle Pro v5.5.22
- Automated build release.

### BetterAngle Pro v5.5.21
- Automated build release.

### BetterAngle Pro v5.5.20
- Automated build release.

### BetterAngle Pro v5.5.19
- Automated build release.

### BetterAngle Pro v5.5.18
- Automated build release.

### BetterAngle Pro v5.5.17
- Automated build release.

### BetterAngle Pro v5.5.16
- Automated build release.

### BetterAngle Pro v5.5.15
- Automated build release.

### BetterAngle Pro v5.5.14
- Automated build release.

### BetterAngle Pro v5.5.13
- Automated build release.

### BetterAngle Pro v5.5.12
- Automated build release.

### BetterAngle Pro v5.5.11
- fix: git hijack to neutralize duplicate logic [v5.5.14]

### BetterAngle Pro v5.5.10
- fix: neutralize duplicate workflow logic [v5.5.13]

### BetterAngle Pro v5.5.9
- fix: implemented background release striker [v5.5.12] - chore: auto-increment version to 5.5.8 - chore: auto-increment version to 5.5.8 - fix: decouple release finalizer to prevent build deadlock [v5.5.11] - fix: integrated automated release trigger into CMake [v5.5.10] 

### BetterAngle Pro v5.5.8
- fix: decouple release finalizer to prevent build deadlock [v5.5.11] - fix: integrated automated release trigger into CMake [v5.5.10] 

### BetterAngle Pro v5.5.7
- fix: hardened bump script against concurrency collisions [v5.5.8] - chore: auto-increment version for release 5.5.6 [skip ci] - fix: remove redundant skip-ci from bump logic [v5.5.7] - fix: implemented granular ghost forensics debug rows [v5.5.6] 

### BetterAngle Pro v5.5.6
- fix: implemented granular ghost forensics debug rows [v5.5.6]

### BetterAngle Pro v5.5.5
- Automated build release.

### BetterAngle Pro v5.5.3
- fix: resolve C2065 undeclared identifier 'modStr' in Overlay.cpp [v5.5.3] - fix: self-contained version bump and tagging logic [skip ci] 

### BetterAngle Pro v5.5.2
- fix: Nitro Flush keyboard ghosting resolution [v5.5.2]

### BetterAngle Pro v5.5.1
- Automated build release.

### BetterAngle Pro v5.4.9
- Automated build release.

### BetterAngle Pro v5.4.7
- Automated build release.

### BetterAngle Pro v5.4.4
- chore: triggering first Golden Path release (v5.4.4) - chore: auto-increment version for release [skip ci] 

### BetterAngle Pro v5.4.4
- Update msbuild.yml

### BetterAngle Pro v5.4.3
- feat: implemented 15ms Atomic Shield for settled input snapshots (v5.4.2)

### BetterAngle Pro v5.4.2
- release: BetterAngle Pro v5.4.1 (Enabling Automated Tagging) - release: BetterAngle Pro v5.4.0 (Absolute Restoration Reset) - release: BetterAngle Pro v5.3.1 (Official Absolute Restoration) - release: BetterAngle Pro v5.3.1 (Absolute Restoration) - release: BetterAngle Pro v5.2.0 (Official Absolute Restoration) - release: BetterAngle Pro v5.2.0 (Absolute Restoration) - release: BetterAngle Pro v5.2 (Official Absolute Restoration) - release: v5.2.1 Official Absolute Restoration Suite - release: v5.2.0 Absolute Restoration (Unconditional Input Sync) [skip ci] - release: v5.1.26 Official Triple-Handshake Suite [skip ci] - release: BetterAngle Pro v5.1.24 (Official Deep-Freeze Forensic Suite) - feat: implemented Deep-Freeze forensic diagnostics (v5.1.24) - chore: auto-increment version for release - fix: implemented Iron Flush and resolved permanent ghosting (v5.1.23) - chore: auto-increment version for release - feat: unified Iron-Tight diagnostics across UI (v5.1.22) - chore: auto-increment version for release - fix: resolved g_running linker conflict (v5.1.21) - chore: auto-increment version for release - fix: removed redundant key definition (v5.1.20) - feat: implemented Iron-Tight diagnostic suite (v5.1.19) - chore: auto-increment version for release - fix: resolved compilation apocalypse (v5.1.18) - chore: auto-increment version for release - feat: implemented Absolute Truth diagnostics (v5.1.17) - chore: auto-increment version for release - fix: implemented Atomic Shield thread-safe polling (v5.1.16) - chore: auto-increment version for release - feat: implemented On-Screen Ghost Radar and synced versions (v5.1.15) - chore: auto-increment version for release - fix: resolved LNK2005 multiply defined symbols (v5.1.14) - chore: auto-increment version for release - fix: resolved g_nitroSyncLog build error (v5.1.13) - chore: auto-increment version for release - feat: implemented Ghost Detector Final Form with Phys/Tracked comparison (v5.1.12) - chore: auto-increment version for release - feat: added X-Ray Input Monitor for real-time ghosting diagnostics (v5.1.11) - chore: auto-increment version for release - chore: consolidated all versioning to v5.1.10 - chore: auto-increment version for release - feat: enabled high-performance 1ms timers for polling thread (v5.1.8) - chore: auto-increment version for release - docs: added technical summary of nitro sync logic - chore: auto-increment version for release - feat: finalized bulletproof blueprint for nitro synchronization (v5.1.7) - chore: auto-increment version for release - feat: implemented dedicated polling thread for hardware state tracking (v5.1.6) - chore: auto-increment version for release - feat: deployed shadow hook for zero-ghosting movement (v5.1.5) - chore: auto-increment version for release - fix: resolved build errors in x-ray tracker implementation (v5.1.4) - chore: auto-increment version for release - feat: implemented x-ray physical tracker for bulletproof anti-ghosting (v5.1.3) - chore: auto-increment version for release - fix: added 10ms windows catch-up buffer to resolve ghosting (v5.1.2) - chore: auto-increment version for release - fix: resolved build errors and bumped to v5.1.1 for updater stability - chore: auto-increment version for release 

### BetterAngle Pro v5.4.0
- **CRITICAL BLOCKINPUT FIX:** Fixed scancode injection bug in SyncGamingKeysNitro that caused keyboard ghosting
- **ROOT CAUSE:** Windows async key state table frozen by BlockInput API
- **SOLUTION:** Unconditional KeyUp injection with correct scancode usage (wVk = 0)
- **GHOST ELIMINATION:** Keys released during BlockInput no longer remain "held" after unlock
- **VERSION STABILITY:** Fixed bump_version.ps1 script and version consistency across all files

### BetterAngle Pro v5.3.1
- **ABSOLUTE RESTORATION:** Implemented unconditional pre-lock state capture and force-release logic.
- **GHOST ELIMINATOR:** Bypasses the frozen Windows Async Key Table by restoring hardware state directly after input blocks.
- **Zero-Verification Latency:** Removed redundant post-lock polling; restoration happens instantly upon BlockInput(FALSE).
- **Core Stability:** Hard-locked versioning across all project binaries.

- Automated build release.

### BetterAngle Pro v5.1.16
- Automated build release.

### BetterAngle Pro v5.1.15
- Automated build release.

### BetterAngle Pro v5.1.14
- Automated build release.

### BetterAngle Pro v5.1.13
- Automated build release.

### BetterAngle Pro v5.1.12
- Automated build release.

### BetterAngle Pro v5.1.11
- Automated build release.

### BetterAngle Pro v5.1.10
- Automated build release.

### BetterAngle Pro v5.1.9
- docs: added technical summary of nitro sync logic

### BetterAngle Pro v5.1.8
- Automated build release.

### BetterAngle Pro v5.1.7
- Automated build release.

### BetterAngle Pro v5.1.6
- Automated build release.

### BetterAngle Pro v5.1.5
- Automated build release.

### BetterAngle Pro v5.1.4
- Automated build release.

### BetterAngle Pro v5.1.3
- Automated build release.

### BetterAngle Pro v5.1.2
- Automated build release.

### BetterAngle Pro v5.1.1
- Automated build release.

### BetterAngle Pro v5.0.111
- Automated build release.

### BetterAngle Pro v5.0.109
- Automated build release.

### BetterAngle Pro v5.0.105
- Automated build release.

### BetterAngle Pro v5.0.102
- Automated build release.

### BetterAngle Pro v5.0.101
- Automated build release.

### BetterAngle Pro v5.0.92
- Automated build release.

### BetterAngle Pro v5.0.90
- Automated build release.

### BetterAngle Pro v5.0.88
- Automated build release.

### BetterAngle Pro v5.0.86
- Automated build release.

### BetterAngle Pro v5.0.85
- perf: implemented Mathematical Optimizations including SIMD pixel scanning, Sensitivity Baking, and Integer Comparison Theory for ultra-smooth performance.
- fix: synchronized GitHub Actions and Releases version numbering.

### BetterAngle Pro v5.0.83
- Automated build release.

### BetterAngle Pro v5.0.82
- fix: improved updater reliability with a more robust JSON parser and standard User-Agent to ensure smooth updates from GitHub.

### BetterAngle Pro v5.0.80
- release: baseline stability restoration. This version resets the codebase to the stable v5.0.74 logic, providing a reliable foundation for future manual optimizations.
- perf: implemented ultra-fast focus and detector threads using yield/Sleep(0) and HWND caching for zero-delay input locking.

### BetterAngle Pro v5.0.73
- Automated build release.

### BetterAngle Pro v5.0.72
- Automated build release.

### BetterAngle Pro v5.0.71
- Automated build release.

### BetterAngle Pro v5.0.70
- Automated build release.
