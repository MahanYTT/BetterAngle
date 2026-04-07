# BetterAngle

A high-performance, low-latency angle tracker and FOV detection overlay for Windows.

## 🚀 Transition to C++
We are currently porting the project from Python to **Pure C++** to achieve:
- **Low-End Hardware Support**: Minimal RAM (<5MB) and CPU usage (~0.1%).
- **Connected Updates**: Automated builds and releases via GitHub Actions.
- **Zero Latency**: Direct Win32 Raw Input and GDI+ rendering.

## 🛠 Features
- **Raw Input Tracking**: Captures mouse delta directly from the hardware.
- **ROI FOV Detector**: Automatically detects game states (Dive/Glide) using high-speed pixel scanning.
- **Transparent Overlay**: Click-through, topmost UI that doesn't interfere with gameplay.
- **Auto-Updater**: Automatically stays up-to-date with the latest releases from GitHub.

## 📦 Download
Check the **[Releases](https://github.com/MahanYTT/BetterAngle/releases)** page for the latest `BetterAngle.exe`.

## ⚙️ Development
1. Clone the repository: `git clone https://github.com/MahanYTT/BetterAngle.git`
2. Open with Visual Studio or compile using `msbuild`.

---
*Created by MahanYTT*
