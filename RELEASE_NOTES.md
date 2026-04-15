Generating release notes from commit range: v4.27.242..HEAD ### BetterAngle Pro v4.27.243
- Fix crosshair thickness below 1px with Java-inspired improvements

Generating release notes from commit range: v4.27.241..HEAD ### BetterAngle Pro v4.27.242
- Fix crosshair thickness issue and add reset button

Generating release notes from commit range: v4.27.240..HEAD ### BetterAngle Pro v4.27.241
- feat: Add crosshair reset button to UI

Generating release notes from commit range: v4.27.239..HEAD ### BetterAngle Pro v4.27.240
- fix: Ensure crosshair thickness can go below 1px with proper precision

Generating release notes from commit range: v4.27.238..HEAD ### BetterAngle Pro v4.27.239
- feat: Unconditional 60Hz UI repaint and Drag safeguard

### BetterAngle Pro v4.27.238
- **Continuous UI Updates**: Disabled the performance-saving redraw cull on the GDI HUD. The overlay now blindly paints at 60Hz instead of waiting for internal thresholds to cross. This ensures that the telemetry metrics (like Ping, FPS, Delay, and Foreground Status) remain fully visible and up-to-date even when the mouse is entirely dormant or when Fortnite is completely minimized.
- **UX: Clutter Removed**: The ROI indicator logic no longer visually echoes its assigned hotkey parameter (e.g. [Ctrl+R]) into the main viewport.
- **Drag Safeguard Active Gameplay**: The overlay window can no longer be physically dragged on the screen while Fortnite retains exclusive foreground capture of the mouse device.

### BetterAngle Pro v4.27.237
- **Anti-Ghosting Sync Fix**: Interjected a 50ms queue-flush deferral before deploying the synthetic key synchronizations directly into the Windows pipeline. Previously, EAC (or just Windows' internal latency) was inadvertently dropping the unstick injections because they arrived while the OS was still busy deconstructing the hardware gate.
- **Prompt Color Focus Sync**: Added foreground detection evaluations directly into the 60FPS UI loop; the ROI state visualizer now reflects application bounds correctly during Alt-Tab regardless of whether raw mouse updates are being intercepted.

### BetterAngle Pro v4.27.236
- **Broadcast & Recording Fix**: The BetterAngle crosshair and debug overlay will now properly appear in gameplay recordings (OBS, ShadowPlay, Discord Streaming, etc). We completely stripped the obscure internal Windows flag (`WDA_EXCLUDEFROMCAPTURE`) that was artificially cloaking the transparent HUD.

### BetterAngle Pro v4.27.235
- **Anti-Ghosting Motion Lock [Hardware Edition]**: Completely ripped out the focus-stealing system which caused character movement drops in some games due to internal key-state panics. We've reverted to the highly reliable system-level `BlockInput` interceptor, but engineered a hardcore **Physical-State Synchronizer** alongside it to permanently fix the annoying keyboard ghosting issue.
- **How the Anti-Ghosting Works**: Right before blocking your input for the 0.25s / 1.00s FOV animations, the app snapshots every single key you are physically holding (like 'W'). The moment the hardware block ends, the app re-evaluates physical reality vs logical state. If you released 'W' *during* the block, the app instantly injects a synthetic `KEYUP` directly to the system to cleanly cut off the ghosting. 
- **Administrator Privileges Required**: Because `BlockInput` modifies the Windows kernel input stack to temporarily paralyze raw mouse input, the `.exe` will now natively prompt for UAC Administrator permissions on launch once again.

### BetterAngle Pro v4.27.234
- **Profile Precision Fix**: Fixed a bug where `target_color` and `crossColor` values were being serialized to JSON using `(float)` casts. This caused 32-bit `COLORREF` integers to lose precision in their lower bits and write out in scientific notation (e.g. `1.67772e+07`). When loaded back in, the color drift caused the angle detector to completely mismatch colors, making it seem like the calibration was wiped / ignored after a restart. Colors are now guaranteed to save losslessly as exactly `(unsigned long)` numbers.
- **Profile Initialization Safety**: Added default values inside the `Profile` struct to prevent random memory garbage values (like `fov = 1.59075e+25` or negative resolutions) from being written to newly created configurations.

### BetterAngle Pro v4.27.233
- **VERSION_STR Fix**: Restored missing `#define V_MAJ/V_MIN/V_PAT` macros in `State.h` that were accidentally dropped in a prior edit. This caused `VERSION_STR` to expand literally (showing "V_MAJ.V_MIN.V_PAT"), breaking the version display in the updater UI and causing an infinite update loop since the version comparison always failed.
- **Updater Infinite Loop Fixed**: With `VERSION_STR` now resolving correctly, version comparison against GitHub releases works as expected â€” no spurious "update available" flags.
- **ROI Prompt Suppressed on First Launch**: Default profile no longer seeds hardcoded ROI coordinates. The detection zone prompt is invisible until the user runs the ROI selector hotkey for the first time.
- **ROI Prompt Shows Keybind**: The DIVING/GLIDING label on the ROI box now prefixes the configured hotkey (e.g. `[Ctrl+R] GLIDING`) so users know exactly how to toggle it.
- **ROI Prompt Turns Purple During Focus-Lock**: When the focus-steal input suspension is active during a dive/glide transition animation, the ROI box turns purple and shows `LOCKING` with remaining milliseconds.
- **Expanded Debug Overlay (GDI+)**: On-screen debug panel now shows 10 rows: FPS, scanner delay, match ratio, threshold %, dive state, input lock timer, Fortnite running/focused/mouse hidden, and active profile name.
- **Expanded Debug Tab (Dashboard)**: QML debug panel now shows scanner delay, match ratio, dive state, input lock status, separated from game-gate indicators by a divider.
- **Tolerance Defaults to 1**: New profiles and the default profile now start with tolerance = 1 instead of 2.

Generating release notes from commit range: v4.27.230..HEAD ### BetterAngle Pro v4.27.232
- fix: Ensure crosshair thickness can go below 1px with proper clamping
- chore: auto-increment version for release
- feat: Replace BlockInput with focus-steal transition guard, remove admin requirement

### BetterAngle Pro v4.27.231
- **Crosshair Thickness Clamp Fix**: Added additional validation in `setCrossThickness` to clamp values between 0.1px and 10.0px, ensuring the crosshair thickness can properly go below 1px as intended by the UI slider. This provides an extra safeguard beyond the profile loading logic.
- **Focus-Steal Transition Guard (replaces BlockInput)**: On dive/glide state transitions, BetterAngle now temporarily steals foreground focus to its own transparent HUD window (invisible to the user). Fortnite stops processing camera input for the animation duration (250ms glideÃ¢â€ â€™dive, 1000ms diveÃ¢â€ â€™glide), then focus is returned instantly. Keyboard input is unaffected throughout.
- **Admin Requirement Removed**: No longer requires Administrator privileges. The UAC manifest flag and elevation check have been removed since `BlockInput` is no longer used.
- feat: Restore Reset Crosshair button and backend API (v4.27.229)
- chore: auto-increment version for release

### BetterAngle Pro v4.27.230
- **Focus-Steal Transition Guard (replaces BlockInput)**: On dive/glide state transitions, BetterAngle now temporarily steals foreground focus to its own transparent HUD window (invisible to the user). Fortnite stops processing camera input for the animation duration (250ms glideÃ¢â€ â€™dive, 1000ms diveÃ¢â€ â€™glide), then focus is returned instantly. Keyboard input is unaffected throughout.
- **Admin Requirement Removed**: No longer requires Administrator privileges. The UAC manifest flag and elevation check have been removed since `BlockInput` is no longer used.

### BetterAngle Pro v4.27.229
- **Admin Elevation Enforced**: BetterAngle now requires and verifies Administrator privileges at startup. If not elevated, a clear error dialog is shown and the app does not launch. The CMake manifest now embeds `requireAdministrator` so Windows auto-prompts UAC on double-click.
- **Input Freeze on State Transition**: Added `BlockInput` system-level input suspension on diving/gliding FOV animation transitions. GlideÃ¢â€ â€™Dive suspends all mouse+keyboard for 250ms; DiveÃ¢â€ â€™Glide for 1000ms to prevent FOV animation noise from corrupting the angle accumulator.
- **Screen Capture Exclusion**: The HUD overlay window is now excluded from Win32 `BitBlt`-based screen capture via `SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE)`, preventing the UI from blocking the detector's pixel reads.
- **Color Selection Dot Removed**: Removed the red dot from the Stage 2 magnifier cursor tip that was obscuring the pixel being aimed at.

Generating release notes from commit range: v4.27.226..HEAD ### BetterAngle Pro v4.27.228
- fix: Crosshair sub-pixel thickness and reset button behavior

Generating release notes from commit range: v4.27.224..HEAD

### BetterAngle Pro v4.27.227
- **Crosshair Reset Button Fix**: Updated the reset crosshair button to properly turn the crosshair OFF and reset all crosshair settings to their default values (thickness: 1.0px, color: red, offsets: 0, pulse: off), matching fresh installation behavior.
- **Crosshair Sub-Pixel Thickness Fix**: Fixed crosshair thickness loading logic to properly support sub-pixel values down to 0.1px. Previously, extremely small values (<0.01) were reset to 1.0px; now they're clamped to the UI minimum of 0.1px, ensuring the crosshair can be made visibly thinner.

### BetterAngle Pro v4.27.226
- **Breathing Pulse Animation**: Implemented a sophisticated 3.0-second crosshair pulse cycle. Features a 1.2s smooth fade-out, a distinct **0.3s transparency pause**, and a 1.5s slow return to full opacity for a premium "alive" look.

### BetterAngle Pro v4.27.225
- **Breathing Pulse Animation**: Implemented a sophisticated 3.0-second crosshair pulse cycle. Features a 1.2s smooth fade-out, a distinct **0.3s transparency pause**, and a 1.5s slow return to full opacity for a premium "alive" look.

### BetterAngle Pro v4.27.224
- feat: Restore diveGlideMatch bindings and inject real-time GDI+ debug performance overlay
- **Crosshair Sub-Pixel Thickness Fix**: Fixed crosshair thickness loading logic to properly support sub-pixel values down to 0.1px. Previously, extremely small values (<0.01) were reset to 1.0px; now they're clamped to the UI minimum of 0.1px, ensuring the crosshair can be made visibly thinner.

### BetterAngle Pro v4.27.223
- **Trigger Calibration Restore**: Fixed the 'NaN%' render issue on the Dive-to-Glide Match Limit slider in the frontend. The Qt binding has been fully restored and directly hooks into the master C++ detector logic arrays, bypassing hardcoded magic bounds.
- **Advanced Diagnostic Sub-Overlay**: Introduced a new toggle in the DEBUG tab allowing users to project an analytics glass sub-panel beneath the master Win32 Angle HUD on-screen. This renders real-time engine tracking metrics including dynamic FPS, algorithmic core thread detection lag bounds, and precision hardware state synchronization (Fortnite context matching).


### BetterAngle Pro v4.27.221
- **Angle Wrap Formatting**: Fixed angle calculation UI logic so that the value dynamically formats and correctly resets precisely at the wrap point (359.9 to 0.0), ensuring the angle is faithfully bounded and doesn't display as 360.0.
- **Debug Diagnostics Tab**: Added a new 'DEBUG' tab to the QML Dashboard providing real-time hardware and operating system hooks to monitor Fortnite process detection, window focus, and mouse hidden states. This operates entirely independent of the core angle calculation logic.



