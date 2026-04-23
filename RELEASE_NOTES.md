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
