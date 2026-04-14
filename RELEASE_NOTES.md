### BetterAngle Pro v4.27.230
- **Focus-Steal Transition Guard (replaces BlockInput)**: On dive/glide state transitions, BetterAngle now temporarily steals foreground focus to its own transparent HUD window (invisible to the user). Fortnite stops processing camera input for the animation duration (250ms glide→dive, 1000ms dive→glide), then focus is returned instantly. Keyboard input is unaffected throughout.
- **Admin Requirement Removed**: No longer requires Administrator privileges. The UAC manifest flag and elevation check have been removed since `BlockInput` is no longer used.

### BetterAngle Pro v4.27.229
- **Admin Elevation Enforced**: BetterAngle now requires and verifies Administrator privileges at startup. If not elevated, a clear error dialog is shown and the app does not launch. The CMake manifest now embeds `requireAdministrator` so Windows auto-prompts UAC on double-click.
- **Input Freeze on State Transition**: Added `BlockInput` system-level input suspension on diving/gliding FOV animation transitions. Glideâ†’Dive suspends all mouse+keyboard for 250ms; Diveâ†’Glide for 1000ms to prevent FOV animation noise from corrupting the angle accumulator.
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
- **Debug Diagnostics Tab**: Added a new 'DEBUG' tab to the QML Dashboard providing real-time hardware and operating system hooks to monitor Fortnite process detection, window focus, and mouse hidden states. This operates entirely independently of the core angle calculation logic, ensuring allowAngleUpdate continues to strictly require Fortnite to be focused and the mouse hidden without being bypassed by debug monitoring.
### BetterAngle Pro v4.27.181
- **Decimal UI Dragging Fix**: Fixed bug where decimal UI could not be dragged when Fortnite is out of focus. The issue was caused by the "SAFETY GUARD" that enforced window transparency (`WS_EX_TRANSPARENT`) even when Fortnite was not in focus, preventing mouse clicks from being detected. Modified the safety guard to only make the window transparent when Fortnite IS in focus, and make it interactive (not transparent) when Fortnite is NOT in focus, allowing proper dragging of the decimal UI.

### BetterAngle Pro v4.27.176
- **Stage 2 Freezing Fix (Final)**: Applied comprehensive bounds checking for `GetPixel()` calls in Stage 2 color selection to prevent screen freezing when clicking prompt color. The fix ensures coordinates are validated against virtual screen dimensions before pixel sampling, with fallback to default color if out of bounds. This resolves the "completely freezes my whole screen" issue reported by users.

### BetterAngle Pro v4.27.173
- **Build Compilation Fix**: Fixed compilation error where `g_pBackend` was undeclared in `NotifyBackendUpdateStatusChanged()` function. Changed to use `s_backendInstance` which is the correct variable name used throughout the codebase. This resolves the "not pushing out effectively" build issue.

### BetterAngle Pro v4.27.172
- **Stage 2 Freezing Fix**: Fixed critical bug where Stage 2 (color selection) would completely freeze after clicking. The issue was caused by `GetPixel()` being called with out-of-bounds coordinates, which could hang or crash. Added bounds checking before calling `GetPixel()` in both BetterAngle.cpp and FirstTimeSetup.cpp.
- **ESC Key Enhancement**: Improved ESC key handling to ensure proper window focus management when cancelling ROI/color selection. Added `SetForegroundWindow(GetDesktopWindow())` to release focus after ESC is pressed.
- **Bounds Validation**: Color selection now validates that sample coordinates are within virtual screen bounds before attempting to read pixel data, preventing hangs and crashes.

### BetterAngle Pro v4.27.169
- **Update Concurrency Guard**: Implemented robust lock guards in the UI and backend to prevent "double processing" or redundant update threads.
- **RAII Thread Safety**: Added an RAII guard to the update cycle to ensure that progress flags are always cleared and the UI is notified, even if network requests fail.

### BetterAngle Pro v4.27.166
- **ROI Selection Hotfix**: Fixed critical bug where "Select prompt stage 1" (ROI selection) was completely broken after ESC key implementation. The issue was caused by a "SAFETY GUARD" timer that forcibly re-enabled window transparency every 16.7ms, preventing mouse interaction during ROI drag.
- **Window Transparency Logic**: Modified the safety guard to skip transparency enforcement when in ROI/Color selection mode, allowing proper mouse capture and drag functionality.
- **ROI Validation**: Added proper validation in WM_LBUTTONUP handler to check if ROI rectangle is valid (non-zero size) before transitioning to color selection stage.

### BetterAngle Pro v4.27.164
- **Default Value Fix**: Changed the default value of "Dive to glide threshold match limit %" slider from 5% to 9% as requested. Both glide and freefall thresholds now default to 9% for new installations.

### BetterAngle Pro v4.27.163
- **Maintenance Release**: Version bump for deployment pipeline synchronization and clean build environment.

### BetterAngle Pro v4.27.162
- **ROI Selection Fix**: Added proper mouse capture (`SetCapture`/`ReleaseCapture`) during ROI drag operations to prevent losing mouse messages when dragging outside the window.
- **ESC Key Support**: Pressing ESC now cancels ROI/color selection and returns to normal mode, with proper cleanup of mouse capture and screen snapshot.
- **Decimal UI Movement Fix**: Resolved bug where decimal UI couldn't be moved even when Fortnite was not in focus by refining drag logic condition ordering.

### BetterAngle Pro v4.27.165
- **Progressive Pulse Animation**: Re-engineered the crosshair pulse to use 1.2-second smooth fades. This creates a 'glide' transition into and out of transparency.
- **Micro-Hold Logic**: The crosshair now stays fully transparent for exactly **0.3 seconds**, providing a brief, rhythmic visual pause before fading back in.

### BetterAngle Pro v4.27.161
- **Final Branding Restored**: Re-integrated the characteristic **">" symbol** into the center of the master cyan orb assets.
- **Deep Shave & Luma Scrub**: Applied a refined 6% margin shave and darkness-threshold filter to the new branding, ensuring absolute transparency with zero black artifacts.
- **Universal Icon Refresh (v161)**: Implemented a cache-busting transition to `BetterAngle_v161.ico` to force Windows to update all desktop and start menu shortcuts immediately.

### BetterAngle Pro v4.27.157
- **UI Nomenclature Update**: Renamed the calibration slider in the General tab to **"Dive to glide threshold match limit %"** per user feedback.
- **Improved Layout**: Cleaned up redundant descriptive text in the General tab for a higher-precision dashboard layout.

### BetterAngle Pro v4.27.154
- **Branding Transparency Hardened**: Programmatically scrubbed the AI-generated checkerboard patterns from the master logo assets, ensuring 100% pure transparency for the cyan orb.
- **Icon Cache-Busting**: Implemented a transition to `BetterAngle_v153.ico` (v154 build). This forces the Windows shell to ignore its old cached icons and display the new high-fidelity assets across the desktop, taskbar, and Control Panel immediately.
- **Universal Asset Injection**: Updated the RC resource script and the Inno Setup configuration to ensure the uninstaller and all program shortcuts use the latest refined branding.
