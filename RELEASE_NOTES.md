### BetterAngle Pro v4.27.162
- **ROI Selection Fix**: Added proper mouse capture (`SetCapture`/`ReleaseCapture`) during ROI drag operations to prevent losing mouse messages when dragging outside the window.
- **ESC Key Support**: Pressing ESC now cancels ROI/color selection and returns to normal mode, with proper cleanup of mouse capture and screen snapshot.
- **Decimal UI Movement Fix**: Resolved bug where decimal UI couldn't be moved even when Fortnite was not in focus by refining drag logic condition ordering.

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
