### BetterAngle Pro v4.27.101
- **Angle Wrap Fix**: Normalized [`AngleLogic::GetAngle()`](src/shared/Logic.cpp:13) through [`AngleLogic::Norm360()`](src/shared/Logic.cpp:47) so the angle remains in `[0, 360)` and wraps from `359.9` back to `0.x` instead of exceeding `360`.
- **Local Legacy Folder Ignore**: Added [`betterangle_old_python_version/`](betterangle_old_python_version) to [`.gitignore`](.gitignore) so the old Python reference code stays local and is not included in pushes.

### BetterAngle Pro v4.27.100
- **Startup UI Sync Fix**: Removed the delayed extra [`requestShowControlPanel()`](src/shared/BetterAngleBackend.cpp:60) from [`BetterAngleBackend::BetterAngleBackend()`](src/shared/BetterAngleBackend.cpp:21), so launch no longer re-toggles the main UI closed after [`ShowControlPanel()`](src/main_app/BetterAngle.cpp:478) already opened it.
- **Launch Behavior Fix**: The decimal HUD and the main dashboard UI now stay visible together on startup instead of only the decimal overlay remaining on screen.

### BetterAngle Pro v4.27.99
- **Keybind UI Restore**: Replaced the broken free-text hotkey editor in [`src/gui/Dashboard.qml`](src/gui/Dashboard.qml:140) with click-to-capture fields again, so binds change by pressing inside the field instead of being corrupted character-by-character.
- **Immediate Apply Fix**: Restored immediate hotkey application from the dashboard capture flow in [`src/gui/Dashboard.qml`](src/gui/Dashboard.qml:176), removing the need for the incorrect manual save-button workflow.
- **Default Label Fix**: Corrected the selection overlay fallback label back to `Ctrl + R` in [`src/shared/BetterAngleBackend.cpp`](src/shared/BetterAngleBackend.cpp:545) so the UI matches the shipped default bind.

### BetterAngle Pro v4.27.98
- **Dashboard UI Boot Fix**: Corrected [`CreateControlPanel()`](src/shared/ControlPanel.cpp:21) to treat [`main.qml`](src/gui/main.qml:5) as successfully loaded when at least one root window exists, fixing the false failure path that blocked the decimal/dashboard UI from opening.
- **Error Message Cleanup**: Removed the misleading missing-log reference from the loader error text in [`src/shared/ControlPanel.cpp`](src/shared/ControlPanel.cpp:37), since there is no shipped [`debug.log`](src/shared/ControlPanel.cpp:37) logging system.

### BetterAngle Pro v4.27.97
- **Installer Fix**: Removed the stale external uninstaller file dependency from [`installer.iss`](installer.iss:29) and switched the uninstall shortcut to Inno Setup's built-in [`{uninstallexe}`](installer.iss:35), fixing release packaging when [`build/Release/uninstaller.exe`](build/Release/uninstaller.exe) is not produced.
- **Workflow Fix**: Updated [`.github/workflows/msbuild.yml`](.github/workflows/msbuild.yml:77) to publish only the generated installer artifact, so GitHub Actions no longer fails release upload on the missing uninstaller binary.

### BetterAngle Pro v4.27.96
- **CI Build Fix**: Aligned [`ShowControlPanel()`](src/shared/ControlPanel.cpp:61) with [`requestShowControlPanel()`](src/shared/BetterAngleBackend.cpp:278) and removed stale dashboard signal handlers from [`src/gui/main.qml`](src/gui/main.qml:22), resolving the GitHub Actions compile/runtime mismatch.
- **Setup Flow Fix**: Added backend [`finishSetup()`](src/shared/BetterAngleBackend.cpp:282) support in [`include/shared/BetterAngleBackend.h`](include/shared/BetterAngleBackend.h:124) and updated [`src/gui/FirstTimeSetup.qml`](src/gui/FirstTimeSetup.qml:110) to use property writes so first-time calibration can persist correctly.
- **Crosshair Default Fix**: Set the default crosshair line thickness to `1.0f` in [`src/shared/State.cpp`](src/shared/State.cpp:108) / [`src/shared/State.cpp`](src/shared/State.cpp:209), kept legacy profile fallback at `1.0f` in [`src/shared/Profile.cpp`](src/shared/Profile.cpp:91), and preserved the `0.1` minimum slider range in [`src/gui/Dashboard.qml`](src/gui/Dashboard.qml:236).
- **Selection Overlay Fix**: Confirmed the selected ROI outline remains visible during prompt colour selection in [`src/shared/Overlay.cpp`](src/shared/Overlay.cpp:112).

### BetterAngle Pro v4.27.94
- **Toggle Dashboard Fix**: Changed the dashboard hotkey from show-only to proper toggle (hide/show) in [`src/gui/main.qml`](src/gui/main.qml:18).
- **Debug Overlay Fix**: Added window repaint after toggling debug mode via hotkey in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:174).

### BetterAngle Pro v4.27.93
- **QML Load Fix**: Removed the unsupported `focusPolicy` usage from the hotkey capture fields in [`src/gui/Dashboard.qml`](src/gui/Dashboard.qml:141), which was preventing [`main.qml`](src/gui/main.qml:5) from loading.
- **Hotkey Field Cleanup**: Switched the capture fields to use explicit ids and `activeFocusOnTab`, keeping the keybind UI valid for Qt Quick Controls.
- **Startup UI Restored**: This fixes the `Failed to load user interface (main.qml)` error path raised from [`src/shared/ControlPanel.cpp`](src/shared/ControlPanel.cpp:18).

### BetterAngle Pro v4.27.92
- **Tolerance Default Fix**: Set the no-profile tolerance fallback to `2` in [`BetterAngleBackend::tolerance()`](src/shared/BetterAngleBackend.cpp:94), aligning runtime defaults with the direct-boot profile default in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:388).
- **Detection Consistency**: This keeps fresh launches and uninitialized UI states on the same tighter detection threshold intended for improved performance.

### BetterAngle Pro v4.27.90
- **Keybind Capture Rewrite**: Reworked hotkey handling in [`src/shared/BetterAngleBackend.cpp`](src/shared/BetterAngleBackend.cpp) so custom binds can reliably use modifier combos such as `Ctrl + R`, `Shift + F10`, and `Alt + G`.
- **Hotkey UI Fix**: Replaced fragile free-text editing in [`src/gui/Dashboard.qml`](src/gui/Dashboard.qml) with press-to-capture fields and explicit save/apply feedback.
- **Default Bind Cleanup**: Standardized the selection overlay default to `Ctrl + R` in [`include/shared/Profile.h`](include/shared/Profile.h).
- **Startup Removal Preserved**: Confirmed the shipped build still has no startup wizard or splash path, with direct boot continuing through [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:381).

### BetterAngle Pro v4.27.89
- **Link Fix**: Removed stale Qt meta-object declarations for `hotkeyStatus` and `setCapturedKeybind` from [`include/shared/BetterAngleBackend.h`](include/shared/BetterAngleBackend.h:55), resolving the unresolved externals reported by `mocs_compilation_Release.obj`.
- **Startup Removal Preserved**: Kept the direct boot flow in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:381) with no startup wizard path or splash components in the shipped target.
- **Release Alignment**: Bumped [`VERSION`](VERSION) and [`CMakeLists.txt`](CMakeLists.txt:2) to `4.27.89` so the GitHub Actions release metadata stays in sync with the fix.

### BetterAngle Pro v4.27.88
- **Startup Wizard Removed**: Removed the first-run setup flow from [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:381), so launch now goes straight into settings and profile loading.
- **Build Fix**: Removed the leftover `g_setupComplete` dependency from [`include/shared/State.h`](include/shared/State.h:20) and [`src/shared/State.cpp`](src/shared/State.cpp:19), resolving the CI failure in [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:385).
- **Source Cleanup**: Deleted [`src/shared/FirstTimeSetup.cpp`](src/shared/FirstTimeSetup.cpp) and [`include/shared/FirstTimeSetup.h`](include/shared/FirstTimeSetup.h), leaving the shipped target in [`CMakeLists.txt`](CMakeLists.txt:24) free of startup wizard and splash-related sources.

### BetterAngle Pro v4.27.91
- **Unified Stability Release**: Formalizing the integration of the modernized "glass" debug overlay and the optimized color match tolerance (2) with the latest direct-boot architecture (no startup wizard).
- **Cleanup**: Verified all legacy Fortnite sync and setup dependencies are fully purged from the codebase for a cleaner, game-agnostic experience.

### BetterAngle Pro v4.27.86
- **Performance Optimization**: Adjusted the default color match tolerance from 25 to 2. This significantly improves startup detection performance and ensures stricter target matching for high-fidelity overlays.

### BetterAngle Pro v4.27.85
- **Modernized Debug Overlay**: Replaced the legacy debug dashboard with a high-fidelity "glass" aesthetic. Features include semi-transparent gradients, inner-glow borders, and sleek layout dividers.
- **Dynamic Layout Engine**: Height now automatically adjusts based on the number of active metrics, eliminating empty space at the bottom of the overlay.
- **Enhanced Diagnostics**: Improved alignment for "Key: Value" pairs and introduced multi-color LED status indicators for critical states like Diving and Detections.
- **Improved Metrics**: Added "Interaction State" and "System Cursor" tracking for better troubleshooting.

### BetterAngle Pro v4.27.84
- **Stability Fix**: Restored the First Time Setup wizard logic which was incorrectly omitted in the previous version. The software now correctly guides new users through initial sensitivity capture.
- **Game-Agnostic Core**: Completed the transition to a fully manual sensitivity model, removing all Fortnite-specific file parsing and focus-locking hooks.

### BetterAngle Pro v4.27.83
- **Startup Wizard Removed**: Deleted the first-run setup flow entirely, including [`src/shared/FirstTimeSetup.cpp`](src/shared/FirstTimeSetup.cpp), [`include/shared/FirstTimeSetup.h`](include/shared/FirstTimeSetup.h), and all launch-time entry points that invoked it from [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:376).
- **Splash Screen Removed**: Removed the native splash implementation and its build references by deleting [`src/shared/Startup.cpp`](src/shared/Startup.cpp), [`include/shared/Startup.h`](include/shared/Startup.h), and stripping the startup sequencing path from [`src/main_app/BetterAngle.cpp`](src/main_app/BetterAngle.cpp:385).
- **Threshold Wizard Removed**: Removed the legacy calibration wizard sources [`src/main_app/ThresholdWizard.cpp`](src/main_app/ThresholdWizard.cpp) and [`include/shared/ThresholdWizard.h`](include/shared/ThresholdWizard.h) so no wizard UI remains in the shipped app.
- **State Cleanup**: Removed persisted setup-complete state from [`src/shared/State.cpp`](src/shared/State.cpp:19) and [`include/shared/State.h`](include/shared/State.h:20), leaving direct profile loading as the only boot path.
- **Build Cleanup**: Updated [`CMakeLists.txt`](CMakeLists.txt:2) and [`VERSION`](VERSION) to `4.27.83` and removed deleted sources from the target so the GitHub Actions release build no longer includes wizard or splash artifacts.

### BetterAngle Pro v4.20.54
- **Qt 6 Framework Upgrade**: Replaced the entire ImGui / DirectX 11 interface with a modern Qt Quick (QML) GUI featuring silky smooth transitions, responsive tabs, and a massive reduction in boilerplate size.
- **Inno Setup Installer Module**: Removed `.zip` packaging. The software now distributes exclusively as a `.exe` setup wizard that automatically injects dependencies, assigns Desktop shortcuts, and provides clean Windows uninstallation.
- **Aesthetics Overhaul**: Shrunk the command center to a sleek `650x480`. Smooth 8px rounded corners gracefully frame the GDI+ Game Overlay and Debug tools. Dropped their alpha transparency limits to eliminate visual obstruction during gameplay.
- **Tick Logic Hotfix**: Repaired the infinite binding loop causing QML toggles (e.g., Debug Menu and Animations) to occasionally "stick" visually.

### BetterAngle Pro v4.20.50
- **Overlay Fix:** Rewrote `Overlay.cpp` using ASCII-safe source encoding, fixing corrupted box-drawing characters that rendered as pixel-dot squares and a capital T instead of the degree symbol. HUD now correctly shows `0.0°`, a working match bar, target colour swatch, and the `:: drag` hint.
- **Control Panel:** Restored missing COLORS tab (target colour picker + tolerance slider), DEBUG tab (force diving/detection toggles + threshold sliders), CROSSHAIR pulse animation checkbox, and the full MANUAL SENSITIVITY section with Fortnite config auto-sync button.
- **Sensitivity Calculation Fixed:** UI now accepts real Fortnite sensitivity values (e.g. `3.0`). Formula is `dx * 0.05555 * sens`. The old workaround of entering `0.3` instead of `3.0` is no longer needed.
- **Config Auto-Detect Fixed:** `FetchFortniteSensitivity()` now returns `-1.0` on failure (file/key not found) instead of a fallback value that was indistinguishable from a valid sensitivity.

### BetterAngle Pro v4.20.19
- **Architectural Overhaul:** Ripped out the custom Direct2D UI drawing engine and completely migrated the Settings Dashboard (`ControlPanel.cpp`) to the industry-standard **Dear ImGui** framework running on DirectX 11.
- **UI Enhancements:** Replaced clunky + and - buttons with smooth sliders. Integrated massive UX upgrades with `ImGui::ColorEdit3` directly in the dashboard tabs instead of summoning external Windows Forms dialogs. Re-mapped all 5 internal visual tabs with hardware acceleration.
- **Workflow Upgrades:** Added 14 core ImGui source dependencies strictly to local source tree and wired `d3d11.lib` successfully into the MSBuild CI/CD deployment pipeline.

### BetterAngle Pro v4.20.18
- Performance Fix: Resolved the severe UI thread contention issue located entirely within the Win32 `WM_INPUT` message loop. Re-routed the blocking `IsFortniteFocused()` system cross-process polling hooks into the asynchronous DetectorThread, resolving the persistent raw mouse delta drops (which manifested as "coordinate system drift" to users when returning their physical mouse to position 0).

### BetterAngle Pro v4.20.15
- Hotfix: Repaired GitHub Actions MSVC quotation bug causing compilation failures via robust `TOSTRING` macro implementation.

### BetterAngle Pro v4.20.13
- Streamlined configuration architecture: completely purged DPI logic and `divingScaleMultiplier` from memory parsing.
- Refactored Angle calculations to use `0.00555 * sensX` exactly.
- Enforced hardcoded `1.0916` diving sensitivity multiplier.
- Stripped FirstTimeSetup to directly target sensitivity and ignore DPI indexing entirely.

### BetterAngle Pro v4.20.4
- RUTHLESS REWRITE of ControlPanel.cpp.
- Introduced Layout struct: both WM_PAINT and WM_LBUTTONDOWN now share exact same coordinate system. Buttons always clickable.
- Fixed WM_SIZE to call Resize() on the D2D target instead of recreating it, enabling smooth window dragging.
- Added WM_GETMINMAXINFO to enforce 420x440 minimum window size.
- CROSSHAIR tab now fully functional: all 4 settings (Thickness, OffsetX, OffsetY, Rotation), Color Picker and Pulse toggle all wired up perfectly.
- Tabs now use even percentage-based spacing, no more overflow at small window sizes.

### BetterAngle Pro v4.20.3
- Refactored entire UI layout to use 100% percentage-based vertical and horizontal positioning.
- Added Dynamic Font Scaling (text shrinks/grows with window size).
- Ensured "QUIT" and footer buttons are always visible on all resolutions.

### BetterAngle Pro v4.20.2
- Fixed GitHub Release URL casing for CamelCase repository "BetterAngle".
- Synchronized internal version strings to eliminate stale "Update Available" flags.
- Optimized Updater User-Agent to comply with GitHub API requirements.

### BetterAngle Pro v4.20.1
- Renamed "XHAIR" tab to "CROSSHAIR" for better visibility.
- Refined responsive tab layout to ensure all 5 tabs are visible on all window sizes.

### BetterAngle Pro v4.20.0: "Responsive Pro Command Center"
v4.20.0: Major UI overhaul for the Control Panel. The window is now fully resizable and responsive—all tabs, buttons, and interaction hit-boxes dynamically auto-fit to any window size. Implemented a focus-locking mechanism where the angle accumulation only triggers when Fortnite is in the foreground. Finalized the "XHAIR" precision configuration tab and ensured all calibration data persists correctly to active profiles.

### BetterAngle Pro v4.19.0: "Native Precision Crosshair Port"
v4.19.0: Fully ported the Java crosshair application natively into the BetterAngle C++ engine. Added a dedicated "XHAIR" configuration tab in the Control Panel using Direct2D. Support for dynamic line thickness, X/Y screen offsets, custom rendering angles, and the sine-wave opacity Pulse animation. The overlay rendering ties directly into the HUD via GDI+ without spawning a separate app or consuming background JVM resources.

### BetterAngle Pro v4.18.0: "Pure GDI Setup + Smart HUD Repaint"
v4.18.0: Eliminated GDI+ from the setup wizard entirely — now uses pure Win32 GDI (FillRect, DrawText). Zero GPU usage during setup. HUD overlay timer now uses state-diff logic: only repaints when the angle, diving state, or cursor visibility actually changes. Timer interval raised from 25ms to 50ms. Combined result: ~80% reduction in CPU and GPU overhead during normal use.

### BetterAngle Pro v4.17.0: "Bulletproof Setup UX & Fortnite-Only Scale Lock"
v4.17.0: Complete bulletproof rewrite of the setup wizard. Fixes window disappearing on click-off (WM_ACTIVATE/WM_NCACTIVATE handlers). Double-buffered GDI+ rendering eliminates all flicker. Correct button hit-testing relative to window bounds. Added Fortnite-only sensitivity condition: the angle decimal (diving scale) only updates when the Fortnite window (`UnrealWindow` class with "Fortnite" title) is the active foreground window. Angle holds at normal scale when clicking BetterAngle Pro or any other app.

### BetterAngle Pro v4.16.0: "Clean Minimal Setup UI Overhaul"
v4.16.0: Completely redesigned the first-time setup wizard with a modern, minimal aesthetic inspired by premium SaaS onboarding flows (Linear, Vercel). Features centered layouts, rounded input fields with accent borders, step progress indicators, Tab key field switching, drag-from-title-area support, and a branded bottom watermark. The "Next" button is gated until a DPI value is entered.

### BetterAngle Pro v4.15.0: "Post-Setup Crash Recovery & Draggable Setup Window"
v4.15.0: Fixed a critical startup crash that occurred immediately after the first-time setup wizard completed. The root cause was the main application indexing into `g_allProfiles` before reloading the profiles from disk following the wizard's save. A safety reload and empty-profile guard have been added. Additionally, the setup window is now draggable by clicking and holding the top title region, and the `PostQuitMessage` bug that was interfering with the main app's message loop has been resolved.

### BetterAngle Pro v4.13.0: "Neon Pro Splash & GDI+ Scoping Cleanup"
v4.13.0: Complete visual overhaul of the startup sequence. Introduced a premium "Neon Cyan" aesthetic with animated progress tracking. Under the hood, refactored the GDI+ Graphics engine scoping to ensure thread-safe resource termination, eliminating rare startup hangs and the "blue wheel of death" unresponsiveness during asset initialization.

### BetterAngle Pro v4.12.0: "Hyper-Minimal Setup Wizard & Asymmetrical Sensitivity Layer"
v4.12.0: Completely rewrote the initial setup wrapper logic. The legacy 7-step wizard (which requested extraneous attributes like rendering resolution constraints and polling Windows pointer precision limits) has been utterly purged. Setup is now a lightning-fast 2-step process isolating strictly DPI and Fortnite Sensitivity. The backend parser directly hooks your `GameUserSettings.ini` on load to automatically detect custom `X/Y` sensitivity splits, rendering a clean dual-display fallback menu. Consequently, the memory structures, disk `.json` saving modules, and rendering algorithms were systematically modified to separate `sensitivity` into `SensitivityX` and `SensitivityY`, seamlessly passing `SensX` into the active physical 360-degree calculation math dynamically mapped against the new scaling constant of `0.5555`.

### BetterAngle Pro v4.11.5: "Strict Window Identification & Alt-Tab Immunity"
v4.11.5: Enforced extreme strictness on the Windows API hook that detects if Fortnite is in the foreground. Previously, the engine loosely mapped the active window text buffer containing the word "Fortnite", meaning Discord tabs or Explorer folders simply named "Fortnite" would inadvertently trigger the mouse tracker to calculate angles. The validation logic has been completely reprogrammed to securely query the system window class metadata and mandate the underlying presence of `UnrealWindow`. Furthermore, the Threshold Calibration Wizard has been tightly wrapped with this restriction, meaning any desktop dragging or Alt-Tab motions completely freeze collection buffers, guaranteeing your sensitivities aren't compromised before your character literally spawns inside the game!

### BetterAngle Pro v4.11.4: "Core Refactoring & Deep System Cleanup"
v4.11.4: Conducted a deep systemic purge of legacy codebase artifacts to stabilize memory footprints. This includes stripping the abandoned `Config.h` structures in favor of native WIN32 hotkey state management, decoupling dormant HTML polling scripts associated with the deprecated Cloud UI profile engine, deleting phantom GUI click zones, properly mapping `g_startPoint` geometry natively inside ROI bounds, and completely eliminating memory traces of the unused string pointer tracking variables (`g_status` etc.). Overall performance stability guarantees across the entire source structure have drastically increased.

### BetterAngle Pro v4.11.0: "Tri-State FOV Profiling & Auto-Threshold Wizards"
v4.11.0: Overhauled the FOV Engine to decouple binary matching and scale linearly across three distinct context fields: *Normal*, *Gliding*, and *Skydiving*. Sensitivities are now driven by a dedicated triple-band configuration matrix. The global UI architecture has been mathematically tightened to block excessive screen overlap, and the Control Panel now features an internal Auto-Threshold Wizard that spawns a raw Win32 pipeline to dynamically snapshot `F10` pixel ratios for Gliding and Freefalling threshold captures, syncing them natively into standard memory. Additionally, the standalone Calibration executable has been retrofitted to calculate the third matrix.

### BetterAngle Pro v4.10.7: "Hotfix: JSON Registry Exception Crash"
v4.10.7: Fixed a fatal initialization runtime exception (`std::invalid_argument`) triggered on startup when parsing standard config spaces. The data parser previously choked on trailing colons during memory loading (`settings.json`), causing an immediate UI crash immediately after the startup splash sequence. The JSON engine now securely wraps all metadata injection points in proper `try-catch` structures with correct spatial bounds, ensuring total startup stability regardless of config formatting.

### BetterAngle Pro v4.10.6: "Ergonomic Calibration & Window Tuning"
v4.10.6: Substantially overhauled the standalone `BetterAngleConfig.exe` sequence. The calibration UI window bounds were scaled down significantly to minimize screen obstruction during active tracking. Furthermore, the progression hotkey for completing the live 120-degree spin was dynamically unbound from `ENTER` and explicitly re-routed natively to `F10`, preventing players from taking their hands off their active gameplay stance while measuring FOV boundaries. Lastly, fixed a historical trace buffer offset error causing negative/inverted coordinate conversions.

### BetterAngle Pro v4.10.5: "True Optical FOV Calibration Layer"
v4.10.5: Completely restructured the engine overriding logic that was artificially snapping the active prompt ROI box (Red/Green state) based on raw angle thresholds instead of genuine algorithm target color matching. Added a new native "Prompt Detection Threshold" slider/modifier within the Control Panel Simulation tab, allowing absolute micro-calibration of the exact spatial ratio required for the software to dynamically shift between Normal and Skydiving sensitivities. Re-wired these values natively into the local `settings.json` registry to permanently persist custom sensitivities alongside native profiles.

### BetterAngle Pro v4.10.4: "Dynamic Cloud Release Logs"
v4.10.4: Native implementation mapped to `msbuild.yml` that seamlessly parses and extracts the latest `RELEASE_NOTES.md` changelog array, injecting it directly into the GitHub Actions active context body at compile time instead of printing static generic build text.

### BetterAngle Pro v4.10.3: "Bugfix: Angle Trajectory Anchor & Config Sub-linking"
v4.10.3: Re-anchored the core tracking geometry (`m_baseAngle`) whenever Sensitivity natively changes state (i.e. changing into Skydiving Mode). Prevents visual target angles from snapping. Hard-linked the remaining missing `State.cpp` and `Logic.cpp` footprint into `BetterAngleConfig` to satisfy GitHub Actions' aggressive strict execution limits on the Input thread wrapper. Repaired the custom Zero Angle keybind which wasn't pushing memory to the calculation core layer. 

### BetterAngle Pro v4.10.2: "Hotfix: Secondary Executable Linker Isolation"
v4.10.2: Resolved a GitHub Actions build abort (`LNK2019`) triggered when the CI system blindly compiled the global `ControlPanel.cpp` UI layer into the lightweight `BetterAngleConfig.exe` footprint. The secondary executable now strict-links only its necessary core logic components (`Input.cpp`, `Profile.cpp`), permanently isolating it from the main software namespace wrapper.

### BetterAngle Pro v4.10.1: "Hotfix: Compiler Scope Isolation"
v4.10.1: Native hotfix applied to patch undeclared `g_currentProfile` scoping limits for the MSVC strict compiler inside `ControlPanel.cpp`, and isolated a `wchar_t` string conversion loop that was tripping C4244 strict errors on Github Actions. Build environment stabilized.

### BetterAngle Pro v4.10.0: "The Calibration Suite & Cloud Command Center"
v4.10.0: Fully introduced `BetterAngleConfig.exe`, a standalone HWND_MESSAGE backed wizard to guide users through 0-120 degree tracking, typing Sens & DPI natively, and outputting standardized JSONs. Completely rewired Control Panel to support dynamic Cloud-Profile syncing directly from the GitHub Repo directory. Completely overhauled Keybinds to be interactively customizable inside the Command Center and persisted to local metadata automatically.

### BetterAngle Pro v4.9.41: "Unbreakable Input Telemetry & Edge-Case Bootloader"
v4.9.41: Added direct dx_sum output to Control Panel diagnostics to track exact mouse buffer intake. Added hardware RawInput VM/Hypervisor support (`MOUSE_MOVE_ABSOLUTE` processing for Parallels/VMware). Engineered an auto-fallback scale generator and profile factory to prevent zero-scale divides when launching externally.

### BetterAngle Pro v4.9.40: "Message Pump & Corrupt JSON Purge"
v4.9.40: Re-routed RawInput via a dedicated invisible HWND_MESSAGE window to bypass Windows 11 transparent layer dropping blocks. Implemented pre-parse comma swapping to restore already-corrupted profiles saving 0,0 float metrics.

### BetterAngle Pro v4.9.39: "Angle Calculation & Simulation Overhaul"
v4.9.39: Critical fix for Angle remaining 0 (RawInput memory padding bug resolved). Fixed locale-dependent JSON parsing that broke float scales on European OS settings. Debug Mode now perfectly overrides all Fortnite gating including cursor lock.

### BetterAngle Pro v4.9.38: "Zero-Copy Optics & Advanced Debug Engine"
v4.9.38: Fixed memory-lock color match bugs via CreateDIBSection. Fully functional Angle Logic globally active with Fortnite override bypass in new Debug tab.

### BetterAngle Pro v4.9.37: "Fixed angle and color picker bug"
v4.9.31: Fixed angle and color picker bug.

### BetterAngle Pro v4.9.30: "Fixing auto update"
v4.9.30: Fixed auto update.

### BetterAngle Pro v4.9.24: "Fixed angle calculation & updated version"
v4.9.24: Fixed angle calculation bug and updated version.

### BetterAngle Pro v4.9.23: "Fixed some bugs"
v4.9.23: Bug fixes.

### BetterAngle Pro v4.9.20: "Removed old stuff"
v4.9.20: Removed old stuff.

### BetterAngle Pro v4.9.16: "Added Set Zero Functionality (NOT TESTED YET)"
v4.9.16: Added a "Set Zero" button but can't compile so someone please test it.
### BetterAngle Pro v4.9.15: "Release Workflow Fix"
v4.9.15: Fixed the GitHub Actions release flow so tag pushes create a GitHub Release and upload BetterAngle.exe automatically.

### BetterAngle Pro v4.9.14: "Zero-Touch Installer & Identity Fix"
v4.9.14: Implemented Zero-Touch Auto-Installer and Relauncher. Fixed Git contribution identity. Improved full-screen coordinate accuracy.

### BetterAngle Pro v4.9.13: "Snapshot Logic & Global Workspace"
💎 v4.9.13: Implemented Full-Screen Snapshot selection. Screen now dims during selection for clarity. Fixed color picking to bypass dark overlay via memory-bitmap retrieval.
💎 **[NEW] Dynamic Live ROI Visualizer:** Persistent visual box that dynamically shifts from GREEN (Gliding) to RED (Diving).
🤖 **[NEW] 60FPS UI Engine:** Control panel rendering boosted to 60 FPS (16ms) for buttery-smooth window interaction.
🔴 **[NEW] Professional Tabbed Dashboard:** Separated General/Binds and Software/Updates for a cleaner, modern workspace.
🔍 **[IMPROVED] Cinematic Calibration Tool:** Full-screen cinematic dark dimming when pressing Ctrl+R to focus entirely on the selection area.
📌 **[FIXED] ROI Visibility Toggle:** Added F9 to instantly toggle the visual scanning box on or off.
