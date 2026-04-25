# BetterAngle CPU Usage Analysis

## High CPU Usage Root Causes

Based on code analysis, here are the primary contributors to high CPU usage:

### 1. **Thread Sleep Intervals (Most Critical)**
| Thread | Sleep Interval | CPU Impact | Purpose |
|--------|---------------|------------|---------|
| **FocusMonitorThread** | `Sleep(0)` | **VERY HIGH** | Polls foreground window for Alt-Tab detection |
| **DetectorThread** | `Sleep(1)` | High | ROI scanning for color detection |
| **PollingThread** (Input.cpp) | `Sleep(1)` | High | Polls keyboard state (WASD + mouse) |
| **Main Message Loop** | N/A (Windows) | Low | UI rendering at 100fps (10ms timer) |

### 2. **CPU-Intensive Operations**
| Operation | Frequency | CPU Impact | Details |
|-----------|-----------|------------|---------|
| **ROI Pixel Scanning** | Every 1ms (when Fortnite focused) | **EXTREME** | Scans ROI area (e.g., 100x100 = 10,000 pixels) with SIMD/SSE2 color distance calculations |
| **Screen Capture** (`BitBlt`) | Every scan iteration | High | Captures screen region via GDI `BitBlt` |
| **GetAsyncKeyState Polling** | Every 1ms | Medium | Polls 5 keys (WASD + LButton) continuously |
| **Window Focus Checking** | Every 0ms (no sleep) | **EXTREME** | Calls `IsFortniteForeground()` in tight loop |

### 3. **Specific Problematic Code Patterns**

#### **FocusMonitorThread** (Lines 54-71, BetterAngle.cpp)
```cpp
while (g_running) {
    bool currentFortniteFocused = IsFortniteForeground();
    // ... logic ...
    Sleep(0); // Max CPU performance for lightning fast focus detection
}
```
- **Issue**: `Sleep(0)` yields but doesn't sleep - runs at maximum possible frequency
- **Impact**: Can consume 10-25% of a CPU core continuously
- **Fix**: Increase to `Sleep(1)` or implement event-driven focus detection

#### **DetectorThread** (Lines 74-222, BetterAngle.cpp)
```cpp
while (g_running) {
    if (currentFortniteFocused) {
        RECT mRect = GetMonitorRectByIndex(g_screenIndex);
        RoiConfig cfg = { ... };
        g_matchCount = g_detector.Scan(cfg); // CPU-HEAVY
        // ... more calculations ...
    }
    Sleep(1); // Ultra-fast scan rate for instant transition detection
}
```
- **Issue**: `Sleep(1)` with heavy scanning every iteration
- **ROI Size Impact**: Larger ROI = exponentially more CPU (O(n²) pixel processing)
- **Color Tolerance Calculations**: Integer math per pixel (dr² + dg² + db² ≤ tol²)

#### **PollingThread** (Lines 145-159, Input.cpp)
```cpp
while (g_running) {
    for (int vk : g_gamingKeys) {
        g_physicalKeys[vk].store((GetAsyncKeyState(vk) & 0x8000) != 0, ...);
    }
    Sleep(1);
}
```
- **Issue**: Polling at 1000Hz (1ms) for key states
- **Impact**: Moderate CPU, but combined with other threads adds up

### 4. **Optimization Opportunities**

| Priority | Area | Current | Recommended | Expected CPU Reduction |
|----------|------|---------|-------------|------------------------|
| **CRITICAL** | FocusMonitorThread | `Sleep(0)` | `Sleep(5)` or event-driven | 80-90% reduction |
| **HIGH** | DetectorThread sleep | `Sleep(1)` | `Sleep(5)` when not diving, `Sleep(2)` when active | 60-80% reduction |
| **HIGH** | ROI scanning optimization | Full scan every iteration | Adaptive scanning (skip frames) | 30-50% reduction |
| **MEDIUM** | PollingThread interval | `Sleep(1)` | `Sleep(5)` | 80% reduction |
| **MEDIUM** | GDI+ rendering | 100fps (10ms) | 60fps (16ms) or adaptive | 40% reduction |

### 5. **Potential Bugs Causing Excessive CPU**

1. **No Throttling When Fortnite Not Focused**
   - DetectorThread still runs `Sleep(1)` even when `currentFortniteFocused` is false
   - Should increase sleep interval when game not focused

2. **Tight Loop in Focus Detection**
   - `Sleep(0)` is essentially a yield - still consumes CPU cycles
   - Windows foreground checks are relatively expensive

3. **No Adaptive Scanning**
   - Always scans at maximum speed regardless of need
   - Could implement variable rate based on activity

### 6. **Recommended Immediate Fixes**

1. **FocusMonitorThread**: Change `Sleep(0)` to `Sleep(5)` (200Hz polling)
   ```cpp
   // Line 69 in BetterAngle.cpp
   Sleep(5); // Reduced from 0 to 5ms for reasonable CPU usage
   ```

2. **DetectorThread**: Implement dual-rate sleeping
   ```cpp
   if (!currentFortniteFocused) {
       Sleep(50); // Slow polling when game not focused
   } else if (!nowDiving && !lastDiving) {
       Sleep(10); // Moderate polling when game focused but not diving
   } else {
       Sleep(2); // Fast polling only when actively diving/gliding
   }
   ```

3. **PollingThread**: Increase interval to 5ms
   ```cpp
   // Line 157 in Input.cpp
   Sleep(5); // Reduced from 1ms to 5ms (200Hz polling)
   ```

4. **ROI Size Validation**: Warn users about large ROI areas
   - ROI of 200x200 = 40,000 pixels scanned every 1-5ms
   - Consider maximum reasonable ROI size (e.g., 100x100)

### 7. **CPU Usage Estimation**

| Scenario | Current CPU | With Fixes | Notes |
|----------|-------------|------------|-------|
| Idle (game not focused) | 5-10% core | 0.5-1% | FocusMonitorThread at 5ms, DetectorThread at 50ms |
| Game focused, not diving | 10-25% core | 2-5% | Reduced polling rates |
| Active diving detection | 25-50% core | 10-20% | ROI scanning still heavy but throttled |
| Large ROI (200x200) | 50-100% core | 20-40% | Pixel processing dominates |

### 8. **Additional Considerations**

- **GDI+ Overlay Rendering**: 100fps timer (`SetTimer(g_hHUD, 1, 10, NULL)`) - consider reducing to 30fps (33ms)
- **BlockInput Operations**: Cause temporary CPU spikes during key state synchronization
- **Multiple Monitor Setups**: `GetMonitorRectByIndex` and screen capture may be heavier
- **Anti-Cheat Compatibility**: Some optimizations (like longer sleep) might affect detection latency

### 9. **File Summary for Export**

**Primary CPU Contributors:**
1. `src/main_app/BetterAngle.cpp`:
   - `FocusMonitorThread()`: Lines 54-71 - `Sleep(0)` tight loop
   - `DetectorThread()`: Lines 74-222 - Heavy ROI scanning with `Sleep(1)`

2. `src/shared/Input.cpp`:
   - `StartPollingThread()`: Lines 145-159 - 1ms key polling

3. `src/shared/Detector.cpp`:
   - `FovDetector::Scan()`: Lines 49-126 - Pixel-by-pixel color matching

**Recommended Changes:**
- Increase sleep intervals strategically
- Implement adaptive polling based on game state
- Add ROI size warnings/limits
- Consider frame skipping for overlay rendering

**Expected Outcome:** 70-90% CPU reduction in typical usage scenarios while maintaining functional detection latency.