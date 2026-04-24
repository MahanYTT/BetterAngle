# Nitro Synchronization Logic (v5.1.7) - The "Door Opening" Fix

## The Problem: "Input Blindness"
When BetterAngle performs an FOV transition (Gliding/Diving) or an Alt-Tab event, it uses `BlockInput(TRUE)` to prevent mouse jitter. However, this creates a "blind spot" in Windows:
1. **The Block:** Windows stops processing all keyboard/mouse interrupts.
2. **The Release:** If a user releases a key (e.g., 'W') during this 400ms-1000ms window, the `KeyUp` signal is discarded by the OS.
3. **The Ghost:** When `BlockInput(FALSE)` is called, the Windows "Software State Table" is stale. It still thinks 'W' is held down because it missed the release event.

## The Solution: The "Bulletproof Blueprint"
To resolve this, v5.1.7 implements a triple-layer synchronization system that bypasses the Windows message queue.

### 1. The Physical Truth Polling Thread
A dedicated high-priority thread runs a 1ms loop using `GetAsyncKeyState`. 
- **Why:** `GetAsyncKeyState` is one of the few APIs that queries the device driver/hardware state directly, bypassing the blocked message queue.
- **Result:** The software knows the 100% truth about your fingers even when the "door is locked."

### 2. The Essential-6 Cluster
We monitor only the keys critical for movement to avoid interference:
- **W, A, S, D** (Movement)
- **Space** (Jump/Glide)
- **Shift** (Sprinting)

### 3. The Double-Check Handshake
At the exact microsecond the input is unlocked, the software performs a "Nitro Sync":
- **Snapshot A:** Hardware state captured 1ms BEFORE the lock.
- **Snapshot B:** Hardware state captured at the EXACT MOMENT of the unlock (Double-validated by the 1ms thread + a fresh hardware poll).
- **The Delta:** If Snapshot A says "Down" and Snapshot B says "Up", the software manually injects a `KeyUp` command into the game.

## Result
Movement is now physically bound to the hardware state. Ghosting and "Auto-Run" bugs are mathematically eliminated because the polling frequency (1ms) is 50x faster than the fastest human finger release (~50ms).
