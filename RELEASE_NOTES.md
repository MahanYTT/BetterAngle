### BetterAngle Pro v5.5.37
- Automated build release.

### BetterAngle Pro v5.5.36
- Automated build release.

### BetterAngle Pro v5.5.35
- Automated build release.

### BetterAngle Pro v5.5.34
- Automated build release.

### BetterAngle Pro v5.5.33
- Automated build release.

### BetterAngle Pro v5.5.32
- Automated build release.

### BetterAngle Pro v5.5.31
- Automated build release.

### BetterAngle Pro v5.5.30
- Automated build release.

### BetterAngle Pro v5.5.29
- Automated build release.

### BetterAngle Pro v5.5.28
- **Nitro Flush Hardening & Architectural Isolation**:
  - Decoupled Alt-Tab focus lock path from FOV transition logic.
  - Implemented 500ms transition cooldown to prevent feedback loops.
  - Purged VK_SPACE from sync array to eliminate double-press issues.
  - Hardened "Hammer" (Fallback 2) to only repress keys confirmed as held.
  - Corrected out-of-bounds array access in diagnostic logging loops.
  - Fixed HUD default position to center horizontally on first run.
  - Consolidated overlay code and removed redundant source files.


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
