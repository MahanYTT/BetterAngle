### BetterAngle Pro v4.27.229
- **Reset Crosshair Restoration**: Fully restored the "RESET CROSSHAIR TO DEFAULTS" button to the Crosshair tab. This connects to a newly re-implemented backend `resetCrosshair()` API that standardizes the crosshair to 1.0px Red, centered, with pulse disabled.
- **Version Integrity Fix**: Restored missing version component macros to the core shared state to ensure consistent build identification across HUD and Dashboard.
