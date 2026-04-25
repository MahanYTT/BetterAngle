# Detailed CPU Analysis: 10% Usage at Idle

## Current Sleep Intervals (Actual Code)

| Thread | Current Sleep | Frequency | CPU Impact (Estimated) |
|--------|---------------|-----------|------------------------|
| **FocusMonitorThread** | `Sleep(16)` | 62.5 Hz | ~0.5% CPU |
| **DetectorThread** | `Sleep(1)` | 1000 Hz | **5-10% CPU** (main culprit) |
| **PollingThread** | `Sleep(1)` | 1000 Hz | **5-10% CPU** |
| **Overlay Timer** | 10ms timer | 100 Hz | ~1-2% CPU |

**Total Estimated Idle CPU: 10-20%** (matches user's 10% observation)

## Why 10% CPU at Idle?

Even when Fortnite is **not focused** and no scanning occurs:
1. **DetectorThread** still wakes up **1000 times per second** (`Sleep(1)`)
   - Each wakeup involves thread context switch, loop condition checks
   - Even minimal work adds up at 1000Hz
2. **PollingThread** wakes up **1000 times per second** to poll 5 keys
   - `GetAsyncKeyState()` calls each time
3. **FocusMonitorThread** at 62.5Hz is reasonable

## Root Cause Analysis

### 1. **DetectorThread Inefficiency**
```cpp
while (g_running) {
    if (!g_allProfiles.empty() && g_currentSelection == NONE) {
        // ... checks Fortnite focus ...
        if (currentFortniteFocused) {
            // Heavy scanning (CPU-intensive)
        }
        // ... other logic ...
    }
    Sleep(1); // <- PROBLEM: Always 1ms even when idle
}
```

**Issue**: No adaptive sleep based on state. Should sleep longer when:
- Fortnite not focused
- No active diving/gliding detection needed

### 2. **PollingThread Over-Polling**
```cpp
while (g_running) {
    for (int vk : g_gamingKeys) {
        g_physicalKeys[vk].store((GetAsyncKeyState(vk) & 0x8000) != 0, ...);
    }
    Sleep(1); // <- PROBLEM: 1000Hz polling for key states
}
```

**Issue**: 1000Hz polling for keyboard state is excessive for gaming. 100-200Hz (5-10ms) is sufficient.

## Recommended Immediate Fixes

### Fix 1: Adaptive Sleep in DetectorThread
```cpp
// In DetectorThread, replace Sleep(1) with:
if (!currentFortniteFocused) {
    Sleep(50); // 20Hz when game not focused
} else if (!nowDiving && !lastDiving) {
    Sleep(10); // 100Hz when game focused but idle
} else {
    Sleep(2); // 500Hz when actively detecting
}
```

**Expected CPU Reduction**: 80-90% for DetectorThread (from 5-10% to 0.5-1%)

### Fix 2: Increase PollingThread Sleep
```cpp
// In Input.cpp line 157, change:
Sleep(5); // 200Hz polling (from 1000Hz)
```

**Expected CPU Reduction**: 80% for PollingThread (from 5-10% to 1-2%)

### Fix 3: Optimize FocusMonitorThread (Optional)
```cpp
// Already at Sleep(16) which is good
// Could consider Sleep(33) for 30Hz if alt-tab latency not critical
```

## Expected Results After Fixes

| Thread | Before | After | Reduction |
|--------|--------|-------|-----------|
| DetectorThread (idle) | 5-10% | 0.5-1% | 90% |
| PollingThread | 5-10% | 1-2% | 80% |
| FocusMonitorThread | 0.5% | 0.5% | 0% |
| Overlay Timer | 1-2% | 1-2% | 0% |
| **Total Idle CPU** | **10-20%** | **3-6%** | **70% reduction** |

## Additional Optimizations

### 1. **ROI Scanning Optimization**
- If ROI is large (e.g., >100x100), consider frame skipping
- Implement incremental scanning (scan different regions each frame)

### 2. **Timer Consolidation**
- Consider combining some polling into fewer threads
- Use event-driven approaches where possible

### 3. **Performance Profiling**
- Add CPU usage logging to identify exact hotspots
- Monitor `g_scannerCpuPct` metric in debug overlay

## Implementation Priority

1. **HIGH**: Adaptive sleep in DetectorThread (biggest win)
2. **HIGH**: Increase PollingThread sleep to 5ms
3. **MEDIUM**: Add ROI size warning in UI
4. **LOW**: Further optimize FocusMonitorThread

## Verification Steps

After implementing fixes:
1. Monitor CPU usage in Task Manager
2. Verify detection latency is still acceptable
3. Test with Fortnite focused/unfocused
4. Check for any regressions in key response

## Code Locations for Changes

1. **DetectorThread**: `src/main_app/BetterAngle.cpp` line 216
2. **PollingThread**: `src/shared/Input.cpp` line 157
3. **FocusMonitorThread**: `src/main_app/BetterAngle.cpp` line 69 (already optimized)

With these changes, idle CPU should drop from 10% to 3-5%, making the application much more efficient while maintaining functionality.