Generating release notes from commit range: v5.1.7..HEAD ### BetterAngle Pro v5.1.8
- Automated build release.

Generating release notes from commit range: v5.1.6..HEAD ### BetterAngle Pro v5.1.7
- Automated build release.

Generating release notes from commit range: v5.1.5..HEAD ### BetterAngle Pro v5.1.6
- Automated build release.

Generating release notes from commit range: v5.1.4..HEAD ### BetterAngle Pro v5.1.5
- Automated build release.

Generating release notes from commit range: v5.1.3..HEAD ### BetterAngle Pro v5.1.4
- Automated build release.

Generating release notes from commit range: v5.1.2..HEAD ### BetterAngle Pro v5.1.3
- Automated build release.

Generating release notes from commit range: v5.1.1..HEAD ### BetterAngle Pro v5.1.2
- Automated build release.

Generating release notes from commit range: v5.0.110..HEAD ### BetterAngle Pro v5.0.111
- Automated build release.

Generating release notes from commit range: v5.0.108..HEAD ### BetterAngle Pro v5.0.109
- Automated build release.

Generating release notes from commit range: v5.0.104..HEAD ### BetterAngle Pro v5.0.105
- Automated build release.

Generating release notes from commit range: v5.0.101..HEAD ### BetterAngle Pro v5.0.102
- Automated build release.

Generating release notes from commit range: v5.0.100..HEAD ### BetterAngle Pro v5.0.101
- Automated build release.

Generating release notes from commit range: v5.0.91..HEAD ### BetterAngle Pro v5.0.92
- Automated build release.

Generating release notes from commit range: v5.0.89..HEAD ### BetterAngle Pro v5.0.90
- Automated build release.

Generating release notes from commit range: v5.0.87..HEAD ### BetterAngle Pro v5.0.88
- Automated build release.

Generating release notes from commit range: v5.0.85..HEAD ### BetterAngle Pro v5.0.86
- Automated build release.

### BetterAngle Pro v5.0.85
- perf: implemented Mathematical Optimizations including SIMD pixel scanning, Sensitivity Baking, and Integer Comparison Theory for ultra-smooth performance.
- fix: synchronized GitHub Actions and Releases version numbering.

### BetterAngle Pro v5.0.83
- Automated build release.

### BetterAngle Pro v5.0.82
- fix: improved updater reliability with a more robust JSON parser and standard User-Agent to ensure smooth updates from GitHub.

### BetterAngle Pro v5.0.82
- Automated build release.

### BetterAngle Pro v5.0.80
- release: baseline stability restoration. This version resets the codebase to the stable v5.0.74 logic, providing a reliable foundation for future manual optimizations.

### BetterAngle Pro v5.0.80
- perf: implemented ultra-fast focus and detector threads using yield/Sleep(0) and HWND caching for zero-delay input locking.

### BetterAngle Pro v5.0.73
- perf: adjusted Glide->Dive lock duration to 700ms.

### BetterAngle Pro v5.0.73
- perf: fine-tuned Glide->Dive lock duration to 567ms.

### BetterAngle Pro v5.0.72
- perf: adjusted input lock durations for faster transitions (Alt-tab: 400ms, Glide->Dive: 500ms, Dive->Glide: 1000ms).

### BetterAngle Pro v5.0.71
- chore: final production release with Ghost-Buster movement sync and optimized scanner. Resolved pipeline sync issues.

### BetterAngle Pro v5.0.69
- fix: resolved GDI handle corruption in high-speed overlay. Fixed issue where debug info and crosshair could disappear after the first frame.

### BetterAngle Pro v5.0.66
- perf: implemented "Super High Speed" optimizations. Unrolled pointer arithmetic in scanner, cached GDI resources in overlay, and restored 1000Hz dedicated focus monitor thread.

### BetterAngle Pro v5.0.65
- chore: restored stable baseline with DPI alignment and high-speed scanner. Fixed build error from focus monitor removal.

### BetterAngle Pro v5.0.64
- chore: reverted experimental zero-latency synchronous locking and anti-ghosting logic to restore system stability. Returned to stable asynchronous input blocking.

### BetterAngle Pro v5.0.63
- fix: upgraded application to Per-Monitor DPI Awareness V2. Ensures physical pixel alignment of the crosshair across multiple monitors with different resolutions and scales.

### BetterAngle Pro v5.0.62
- fix: upgraded application to Per-Monitor DPI Awareness V2. Ensures physical pixel alignment of the crosshair across multiple monitors with different resolutions and scales.

### BetterAngle Pro v5.0.60
- fix: added ReleaseHeldKeys() call before every BlockInput(TRUE) to synthesize KEYUP events for all physically-held keys, eliminating character ghosting/sliding during lock windows.
- perf: implemented zero-latency synchronous input locking across all triggers (Alt-Tab, Dive/Glide transitions), bypassing Windows scheduler delays.
- perf: optimized pixel traversal in the color matching loop using direct pointer arithmetic for maximum speed.


### BetterAngle Pro v5.0.58

### BetterAngle Pro v5.0.57
- Automated build release.

### BetterAngle Pro v5.0.56

### BetterAngle Pro v5.0.55
- Automated build release.

### BetterAngle Pro v5.0.54
- HOTFIX: Re-architected crosshair drawing logic to calculate exact center bounds for the explicitly chosen monitor from the Settings Drop-down (g_screenIndex) rather than defaulting to the center of the total virtual desktop space.

### BetterAngle Pro v5.0.54
- Automated build release.

### BetterAngle Pro v5.0.52

### BetterAngle Pro v5.0.51
- Automated build release.

### BetterAngle Pro v5.0.50

### BetterAngle Pro v5.0.49
- Automated build release.

### BetterAngle Pro v5.0.48

### BetterAngle Pro v5.0.47
- Automated build release.

### BetterAngle Pro v5.0.46
- fix: loading screen fits perfectly, no cropping

### BetterAngle Pro v5.0.45
- fix: bulletproof multi-monitor - sync screenIndex from profile on load, fix ComboBox binding

### BetterAngle Pro v5.0.44
- Automated build release.

### BetterAngle Pro v5.0.43
- feat: implemented robust Multi-Monitor selection support
- feat: added "Active Game Monitor" dropdown to Dashboard settings
- fix: ROI coordinates are now correctly remapped to the selected monitor
- Automated build release.

### BetterAngle Pro v5.0.42
- fix: decimal UI (HUD overlay) and debug overlay now draggable outside Fortnite focus

### BetterAngle Pro v5.0.41
- Automated build release.

### BetterAngle Pro v5.0.40
- feat: added Lock Trigger Reason, Peak Match (2s), ROI Dimensions, Scanner CPU%, and Version to Debug Dashboard and Overlay

### BetterAngle Pro v5.0.39
- Automated build release.

### BetterAngle Pro v5.0.38
- fix: removed Profile row from Debug overlay UI (not needed, profile system is unchanged)

### BetterAngle Pro v5.0.37
- Automated build release.

### BetterAngle Pro v5.0.36
- fix: updated Debug Dashboard wording from "Mouse Hidden" to "Mouse in Fortnite Focus" for better clarity

### BetterAngle Pro v5.0.35
- Automated build release.

### BetterAngle Pro v5.0.34
- feat: increased Dashboard Debug and HUD refresh rate to 100fps (10ms) for real-time flicker detection

### BetterAngle Pro v5.0.33
- Automated build release.

### BetterAngle Pro v5.0.32
- fix: implemented exact state debouncing to fix transition flapping and severe input ghosting during Gliding -> Skydiving

### BetterAngle Pro v5.0.31
- Automated build release.

### BetterAngle Pro v5.0.30
- feat: made the Debug tab in the Dashboard scrollable for better accessibility on smaller screens

### BetterAngle Pro v5.0.29
- Automated build release.

### BetterAngle Pro v5.0.28
- fix: disabled HUD dragging whenever Fortnite is in focus to prevent accidental UI movement during gameplay (maps/menus)

### BetterAngle Pro v5.0.27
- Automated build release.

### BetterAngle Pro v5.0.26
- perf: optimized dive -> glide transition latency (reduced from 5ms to 1ms) for instant input locking

### BetterAngle Pro v5.0.25
- Automated build release.

### BetterAngle Pro v5.0.24
- fix: increased Glide -> Skydiving lock duration from 250ms to 1000ms for a more reliable and solid feel during FOV transitions

### BetterAngle Pro v5.0.23
- perf: achieved absolute zero-latency focus detection by removing all internal caching logic from the foreground window check

### BetterAngle Pro v5.0.22
- perf: reduced Alt-Tab detection latency from 1000ms to 50ms for near-instant input locking when returning to the game window

### BetterAngle Pro v5.0.21
- revert: emergency rollback to stable BlockInput system (V5.0.18 baseline) due to hook detection issues.

### BetterAngle Pro v5.0.20
- Automated build release.

### BetterAngle Pro v5.0.19
- fix: replaced BlockInput with low-level hardware hooks (WH_KEYBOARD_LL/WH_MOUSE_LL) (ROLLED BACK)

### BetterAngle Pro v5.0.18
- perf: optimized debug overlay with high-frequency focus/cursor caches for real-time responsiveness

### BetterAngle Pro v5.0.17
- Automated build release.

### BetterAngle Pro v5.0.16
- perf: lightning-fast focus detection (10ms) to ensure angle updates stop instantly when tabbing out

### BetterAngle Pro v5.0.15
- Automated build release.

### BetterAngle Pro v5.0.14
- fix: freeze inputs for 1.65s on alt-tab into Fortnite to prevent angle inaccuracies

### BetterAngle Pro v5.0.13
- Automated build release.

### BetterAngle Pro v5.0.12
- Automated build release.

### BetterAngle Pro v5.0.1
- Automated build release.

### BetterAngle Pro v5.0.0
- Official Major Version 5 Release
- Consolidated all recent UI performance optimizations
- Finalized org-wide repository synchronization
- Stabilized crosshair positioning and HUD synchronization

Generating release notes from commit range: v4.27.375..HEAD ### BetterAngle Pro v4.27.376
- Update LICENSE

Generating release notes from commit range: v4.27.374..HEAD ### BetterAngle Pro v4.27.375
- chore: update GitHub links to wavedropmaps-org org

### BetterAngle Pro v4.27.374
- chore: update GitHub repository references to point to wavedropmaps-org organization
- Revise README for BetterAngle Pro and hardware specs
- Automated build release via GitHub Actions.

### BetterAngle Pro v4.27.373
- build: bump version to 4.27.372 and trigger release

### BetterAngle Pro v4.27.372
- fix(updater): keep user configs intact by preventing deletion of AppData directory during update.
- feat(ui): improve initial boot by shortening splash screen to 2.5s and hiding HUD until fully loaded.
- Automated build release via GitHub Actions.

Generating release notes from commit range: v4.27.370..HEAD ### BetterAngle Pro v4.27.371
- fix(updater): prevent deletion of AppData during update

Generating release notes from commit range: v4.27.369..HEAD ### BetterAngle Pro v4.27.370
- revert: scroll bar changes due to QML root error

Generating release notes from commit range: v4.27.368..HEAD ### BetterAngle Pro v4.27.369
- feat: add left-side scroll bars to GENERAL, CROSSHAIR, and DEBUG tabs

Generating release notes from commit range: v4.27.367..HEAD ### BetterAngle Pro v4.27.368
- feat: safe switch for ROI selection - prompts show during selection regardless of Fortnite focus

Generating release notes from commit range: v4.27.365..HEAD ### BetterAngle Pro v4.27.367
- fix: remove duplicate variable declarations in DetectorThread
- chore: auto-increment version for release
- feat: ROI detection and selection prompts now require Fortnite focus

Generating release notes from commit range: v4.27.365..HEAD ### BetterAngle Pro v4.27.366
- feat: ROI detection and selection prompts now require Fortnite focus

Generating release notes from commit range: v4.27.363..HEAD ### BetterAngle Pro v4.27.365
- revert: decimal UI changes back to original behavior
- fix: properly show HUD after loading screen

Generating release notes from commit range: v4.27.361..HEAD ### BetterAngle Pro v4.27.364
- revert: decimal UI changes back to original behavior
- fix: restore HUD visibility during loading screen

### BetterAngle Pro v4.27.363
- fix: properly show HUD after loading screen using SetWindowPos instead of ShowWindow
- fix: hide HUD during loading screen to prevent visual glitch
- chore: bump version to v4.27.362 with HUD loading fix
- fix: hide HUD (crosshair overlay) until loading screen completes

### BetterAngle Pro v4.27.362
- fix: hide HUD (crosshair overlay) until loading screen completes

### BetterAngle Pro v4.27.361
- fix: optimize logo rendering quality and launch v4.27.360
- fix: stabilize restored v4.27.325 and launch v4.27.355
- feat: add close button to dashboard and launch v4.27.325
- fix: resolve finishBooting compilation error and launch v4.27.320
- fix: perfectly synchronize HUD crosshair with loading screen and launch v4.27.315
- fix: resolve black loading screen by converting loading_3 to true PNG
- fix: include loading_3.png asset and launch v4.27.301
- fix: resolve black loading screen and launch v4.27.300
- fix: resolve black loading screen by registering loading_3 in QRC
- chore: launch v4.27.295 with full stabilization and 3-variant maps
- chore: release v4.27.291 with 3-variant maps and HUD sync
- chore: auto-increment version for v4.27.290 release
- chore: auto-increment version for release
- fix: stabilize original Release 282 (purge incompatible QML syntax)

### BetterAngle Pro v4.27.315
- fix: synchronized HUD crosshair visibility with C++ boot sequence
- fix: ensured decimal UI (crosshair) only loads after splash screen finish

### BetterAngle Pro v4.27.310
- fix: resolved black loading screens by converting map assets to valid PNG format

### BetterAngle Pro v4.27.300
- fix: 100% stable restoration of core Release 282 features
- fix: purged incompatible QML syntax to resolve dashboard opening errors

### BetterAngle Pro v4.27.291
- feat: re-enabled 3rd landscape loading variant
- fix: synchronized HUD crosshair visibility with C++ boot sequence

### BetterAngle Pro v4.27.290
- fix: 100% stable restoration of core Release 282 features
- fix: purged incompatible QML syntax to resolve dashboard opening errors

Generating release notes from commit range: v4.27.283..HEAD ### BetterAngle Pro v4.27.283
- fix: stabilize original Release 282 (purge incompatible QML syntax)

Generating release notes from commit range: v4.27.281..HEAD ### BetterAngle Pro v4.27.282
- feat: implement randomized loading screen rotation with 2 variants

Generating release notes from commit range: v4.27.280..HEAD ### BetterAngle Pro v4.27.281
- Add resize handles to frameless window

Generating release notes from commit range: v4.27.278..HEAD ### BetterAngle Pro v4.27.280
- Fix compilation error for g_keybindAssignmentActive
- chore: auto-increment version for release

Generating release notes from commit range: v4.27.277..HEAD ### BetterAngle Pro v4.27.279
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.276..HEAD ### BetterAngle Pro v4.27.278
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.275..HEAD ### BetterAngle Pro v4.27.277
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.274..HEAD ### BetterAngle Pro v4.27.276
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.273..HEAD ### BetterAngle Pro v4.27.275
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.272..HEAD ### BetterAngle Pro v4.27.274
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.271..HEAD ### BetterAngle Pro v4.27.273
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.270..HEAD ### BetterAngle Pro v4.27.272
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.269..HEAD ### BetterAngle Pro v4.27.271
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.268..HEAD ### BetterAngle Pro v4.27.270
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.267..HEAD ### BetterAngle Pro v4.27.269
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.266..HEAD ### BetterAngle Pro v4.27.268
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.265..HEAD ### BetterAngle Pro v4.27.267
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.264..HEAD ### BetterAngle Pro v4.27.266
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.263..HEAD ### BetterAngle Pro v4.27.265
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.262..HEAD ### BetterAngle Pro v4.27.264
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.261..HEAD ### BetterAngle Pro v4.27.263
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.260..HEAD ### BetterAngle Pro v4.27.262
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.259..HEAD ### BetterAngle Pro v4.27.261
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.258..HEAD ### BetterAngle Pro v4.27.260
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.257..HEAD ### BetterAngle Pro v4.27.259
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.256..HEAD ### BetterAngle Pro v4.27.258
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.255..HEAD ### BetterAngle Pro v4.27.257
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.254..HEAD ### BetterAngle Pro v4.27.256
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.253..HEAD ### BetterAngle Pro v4.27.255
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.252..HEAD ### BetterAngle Pro v4.27.254
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.251..HEAD ### BetterAngle Pro v4.27.253
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.250..HEAD ### BetterAngle Pro v4.27.252
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.249..HEAD ### BetterAngle Pro v4.27.251
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.248..HEAD ### BetterAngle Pro v4.27.250
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.247..HEAD ### BetterAngle Pro v4.27.249
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.246..HEAD ### BetterAngle Pro v4.27.248
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.245..HEAD ### BetterAngle Pro v4.27.247
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.244..HEAD ### BetterAngle Pro v4.27.246
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.243..HEAD ### BetterAngle Pro v4.27.245
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.242..HEAD ### BetterAngle Pro v4.27.244
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.241..HEAD ### BetterAngle Pro v4.27.243
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.240..HEAD ### BetterAngle Pro v4.27.242
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.239..HEAD ### BetterAngle Pro v4.27.241
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.238..HEAD ### BetterAngle Pro v4.27.240
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.237..HEAD ### BetterAngle Pro v4.27.239
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.236..HEAD ### BetterAngle Pro v4.27.238
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.235..HEAD ### BetterAngle Pro v4.27.237
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.234..HEAD ### BetterAngle Pro v4.27.236
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.233..HEAD ### BetterAngle Pro v4.27.235
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.232..HEAD ### BetterAngle Pro v4.27.234
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.231..HEAD ### BetterAngle Pro v4.27.233
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.230..HEAD ### BetterAngle Pro v4.27.232
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.229..HEAD ### BetterAngle Pro v4.27.231
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.228..HEAD ### BetterAngle Pro v4.27.230
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.227..HEAD ### BetterAngle Pro v4.27.229
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.226..HEAD ### BetterAngle Pro v4.27.228
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.225..HEAD ### BetterAngle Pro v4.27.227
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.224..HEAD ### BetterAngle Pro v4.27.226
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.223..HEAD ### BetterAngle Pro v4.27.225
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.222..HEAD ### BetterAngle Pro v4.27.224
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.221..HEAD ### BetterAngle Pro v4.27.223
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.220..HEAD ### BetterAngle Pro v4.27.222
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.219..HEAD ### BetterAngle Pro v4.27.221
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.218..HEAD ### BetterAngle Pro v4.27.220
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.217..HEAD ### BetterAngle Pro v4.27.219
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.216..HEAD ### BetterAngle Pro v4.27.218
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.215..HEAD ### BetterAngle Pro v4.27.217
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.214..HEAD ### BetterAngle Pro v4.27.216
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.213..HEAD ### BetterAngle Pro v4.27.215
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.212..HEAD ### BetterAngle Pro v4.27.214
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.211..HEAD ### BetterAngle Pro v4.27.213
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.210..HEAD ### BetterAngle Pro v4.27.212
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.209..HEAD ### BetterAngle Pro v4.27.211
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.208..HEAD ### BetterAngle Pro v4.27.210
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.207..HEAD ### BetterAngle Pro v4.27.209
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.206..HEAD ### BetterAngle Pro v4.27.208
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.205..HEAD ### BetterAngle Pro v4.27.207
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.204..HEAD ### BetterAngle Pro v4.27.206
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.203..HEAD ### BetterAngle Pro v4.27.205
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.202..HEAD ### BetterAngle Pro v4.27.204
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.201..HEAD ### BetterAngle Pro v4.27.203
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.200..HEAD ### BetterAngle Pro v4.27.202
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.199..HEAD ### BetterAngle Pro v4.27.201
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.198..HEAD ### BetterAngle Pro v4.27.200
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.197..HEAD ### BetterAngle Pro v4.27.199
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.196..HEAD ### BetterAngle Pro v4.27.198
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.195..HEAD ### BetterAngle Pro v4.27.197
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.194..HEAD ### BetterAngle Pro v4.27.196
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.193..HEAD ### BetterAngle Pro v4.27.195
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.192..HEAD ### BetterAngle Pro v4.27.194
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4.27.191..HEAD ### BetterAngle Pro v4.27.193
- fix: resolve missing include for std::vector

Generating release notes from commit range: v4
