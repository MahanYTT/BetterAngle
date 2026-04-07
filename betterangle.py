import ctypes
import json
import logging
import os
import threading
import time
import psutil
import win32api
import win32con
import win32gui
import win32process
import pywintypes
from ctypes import wintypes

# =======================
# ANGLE OVERLAY + FULLSCREEN CROSSHAIR + WATERMARK
# =======================

# ---------- Fortnite exe names ----------
FORTNITE_EXES = {
    "FortniteClient-Win64-Shipping.exe",
    "FortniteClient-Win64-Shipping_EAC.exe",
    "FortniteClient-Win64-Shipping_BE.exe",
}
FORTNITE_EXES_LOWER = {name.lower() for name in FORTNITE_EXES}
FORTNITE_EXE_PREFIX = "fortniteclient-win64-shipping"
FG_CACHE_MS = 75
FOCUS_UPDATE_MS = 100

FG_CACHE_MS = 75
FOCUS_UPDATE_MS = 100

# Set DPI Awareness to handle resolution correctly
try:
    ctypes.windll.shcore.SetProcessDpiAwareness(1)
except Exception:
    try:
        ctypes.windll.user32.SetProcessDPIAware()
    except Exception:
        pass
user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
SCREEN_W = user32.GetSystemMetrics(0)
SCREEN_H = user32.GetSystemMetrics(1)

# Position of the Angle Stats Box (Top Left)
BOX_X, BOX_Y = 30, 30
BOX_W, BOX_H = 400, 240

# Colors
C_INVISIBLE = win32api.RGB(0, 0, 0)
C_BOX_BG    = win32api.RGB(30, 34, 40)
C_WHITE     = win32api.RGB(245, 248, 255)
C_GREY      = win32api.RGB(175, 182, 196)
C_DARK_GREY = win32api.RGB(128, 136, 152)
C_GREEN     = win32api.RGB(0, 255, 127) 
C_YELLOW    = win32api.RGB(255, 215, 0)
C_RED       = win32api.RGB(255, 50, 50)

# ---------- Calibration ----------
CAL_A_DEG = 0.0
CAL_B_DEG = 120.0
DIRECTION_DEFAULT = 1
SMOOTH_ALPHA = 0

# ---------- FOV Detection ----------
PROMPT_NONE = "NONE"
PROMPT_SKYDIVE = "SKYDIVE"
PROMPT_UMBRELLA = "UMBRELLA"
FOV_MODE_NORMAL = "NORMAL"
FOV_MODE_FREEFALL = "FREEFALL"

FOV_DETECT_DEFAULT = {
    "enabled": True,
    # Normalized ROI around the interaction prompt line near the screen center-bottom.
    "roi_norm": {"x": 0.37, "y": 0.74, "w": 0.26, "h": 0.06},
    "coarse_mode": True,
    "coarse_cols": 8,
    "coarse_rows": 4,
    "use_target_color": True,
    "target_rgb": [255, 255, 255],
    "color_tolerance": 24,
    "sample_step": 3,
    "min_sample_interval_ms": 80,
    "max_samples": 48,
    "max_scan_time_ms": 10,
    "white_min_rgb": 205,
    "white_max_channel_delta": 28,
    # Tune these on your setup to separate SKYDIVE vs UMBRELLA prompt lengths.
    "ratio_skydive_min": 0.010,
    "ratio_umbrella_min": 0.028,
    "ratio_hysteresis": 0.003,
    "confirm_frames": 2,
    "cal_hysteresis": 0.004,
    "ratio_calibration": {"NONE": None, "SKYDIVE": None, "UMBRELLA": None},
    "prompt_mode_map": {"NONE": "NORMAL", "SKYDIVE": "FREEFALL", "UMBRELLA": "NORMAL"},
    # If freefall calibration is missing, fallback to normal * multiplier.
    "freefall_scale_multiplier": 1.0,
}

# ---------- Files ----------
SCRIPT_DIR = os.path.dirname(__file__)
PROFILE_PATH = os.path.join(SCRIPT_DIR, "angle_profiles.json")
BINDS_PATH = os.path.join(SCRIPT_DIR, "angle_keybinds.json")
APP_STATE_PATH = os.path.join(SCRIPT_DIR, "angle_app_state.json")
FOV_DETECT_PATH = os.path.join(SCRIPT_DIR, "angle_fov_detector.json")
LOG_PATH = os.path.join(SCRIPT_DIR, "angle_overlay.log")

logging.basicConfig(
    filename=LOG_PATH,
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)
logger = logging.getLogger("angle_overlay")

# ---------- Win32 / RawInput ----------
UINT_PTR = ctypes.c_size_t
WM_INPUT = getattr(win32con, "WM_INPUT", 0x00FF)
RID_INPUT = 0x10000003
RIM_TYPEMOUSE = 0
SRCCOPY = 0x00CC0020
DIB_RGB_COLORS = 0

user32.SetTimer.argtypes = [wintypes.HWND, UINT_PTR, wintypes.UINT, wintypes.LPVOID]
user32.SetTimer.restype = UINT_PTR
user32.KillTimer.argtypes = [wintypes.HWND, UINT_PTR]
user32.KillTimer.restype = wintypes.BOOL

# ---------- Hotkey IDs ----------
HK_CAL_A   = 1
HK_CAL_B   = 2
HK_ZERO    = 3
HK_EXIT    = 4
HK_CFG     = 5
HK_CROSS   = 6
HK_ROI     = 7
HK_DIAG    = 8
HK_COLOR   = 9
HK_PNONE   = 10
HK_PSKY    = 11
HK_PUMB    = 12
HK_WIZARD  = 13

# ---------- Focus Stealer Lock (User Suggested) ----------
STEALER_CLASS_NAME = "AngleFocusStealer"
stealer_hwnd = None
last_game_hwnd = None
camera_locked_until = 0

def stealer_wndproc(hwnd, msg, wparam, lparam):
    return win32gui.DefWindowProc(hwnd, msg, wparam, lparam)

def init_stealer_window():
    global stealer_hwnd
    wc = win32gui.WNDCLASS()
    wc.lpfnWndProc = stealer_wndproc
    wc.lpszClassName = STEALER_CLASS_NAME
    hinst = win32api.GetModuleHandle(None)
    wc.hInstance = hinst
    try:
        atom = win32gui.RegisterClass(wc)
    except Exception:
        pass # Class might already exist
    
    # Create invisible popup window
    # WS_POPUP | WS_VISIBLE (initially hidden by us)
    # WS_EX_TOOLWINDOW prevents taskbar icon
    style = win32con.WS_POPUP
    ex_style = win32con.WS_EX_TOOLWINDOW | win32con.WS_EX_NOACTIVATE
    stealer_hwnd = win32gui.CreateWindowEx(
        ex_style, STEALER_CLASS_NAME, "FocusStealer",
        style, 
        0, 0, 1, 1, # 1x1 pixel
        None, None, hinst, None
    )

def lock_camera():
    global last_game_hwnd, stealer_hwnd
    
    # If already locked, do nothing (or re-assert?)
    if camera_locked_until > win32api.GetTickCount():
        # Re-assert focus if lost?
        if win32gui.GetForegroundWindow() != stealer_hwnd:
             try:
                 win32gui.SetForegroundWindow(stealer_hwnd)
             except: pass
        return

    # 1. Capture current FG window (Fortnite)
    cur_fg = win32gui.GetForegroundWindow()
    if cur_fg and cur_fg != stealer_hwnd:
        last_game_hwnd = cur_fg
    
    # 2. Steal Focus
    if stealer_hwnd:
        try:
            # Show and activate
            win32gui.ShowWindow(stealer_hwnd, win32con.SW_SHOW)
            win32gui.SetForegroundWindow(stealer_hwnd)
            # win32gui.SetCapture(stealer_hwnd) # Optional, but good to eat mouse clicks
        except Exception as e:
            logger.error(f"Failed to steal focus: {e}")

def unlock_camera():
    global last_game_hwnd, stealer_hwnd
    
    # 1. Release
    if stealer_hwnd:
        win32gui.ShowWindow(stealer_hwnd, win32con.SW_HIDE)
    
    # 2. Restore Game
    if last_game_hwnd and win32gui.IsWindow(last_game_hwnd):
        try:
            # Only restore if we are currently the FG (don't steal from user if they alt-tabbed elsewhere)
            if win32gui.GetForegroundWindow() == stealer_hwnd:
                win32gui.SetForegroundWindow(last_game_hwnd)
        except Exception:
            pass

def trigger_camera_lock(duration_ms=1000):
    global camera_locked_until
    camera_locked_until = win32api.GetTickCount() + duration_ms
    lock_camera()

def update_camera_lock():
    global camera_locked_until
    if camera_locked_until > 0:
        if win32api.GetTickCount() > camera_locked_until:
            unlock_camera()
            camera_locked_until = 0
        else:
            # Re-assert lock (fight game trying to take focus back?)
            lock_camera()

HK_PSKY    = 11
HK_PUMB    = 12
HK_WIZARD  = 13

TIMER_ID = 1
TIMER_MS = 25

# ---------- Structures ----------
class RAWINPUTHEADER(ctypes.Structure):
    _fields_ = [
        ("dwType", wintypes.DWORD), ("dwSize", wintypes.DWORD),
        ("hDevice", wintypes.HANDLE), ("wParam", wintypes.WPARAM),
    ]

class RAWINPUTDEVICE(ctypes.Structure):
    _fields_ = [
        ("usUsagePage", wintypes.USHORT), ("usUsage", wintypes.USHORT),
        ("dwFlags", wintypes.DWORD), ("hwndTarget", wintypes.HWND),
    ]

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]

class RGBQUAD(ctypes.Structure):
    _fields_ = [
        ("rgbBlue", ctypes.c_ubyte),
        ("rgbGreen", ctypes.c_ubyte),
        ("rgbRed", ctypes.c_ubyte),
        ("rgbReserved", ctypes.c_ubyte),
    ]

class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", RGBQUAD * 1)]

# ---------- Raw Input Logic ----------
def register_raw_mouse(hwnd):
    RIDEV_INPUTSINK = 0x00000100
    rid = RAWINPUTDEVICE(0x01, 0x02, RIDEV_INPUTSINK, hwnd)
    if not user32.RegisterRawInputDevices(ctypes.byref(rid), 1, ctypes.sizeof(rid)):
        raise ctypes.WinError(ctypes.get_last_error())

def get_rawinput_lLastX(lparam) -> int | None:
    dwSize = wintypes.UINT(0)
    if user32.GetRawInputData(wintypes.HANDLE(lparam), RID_INPUT, None, ctypes.byref(dwSize), ctypes.sizeof(RAWINPUTHEADER)) != 0:
        return None
    size = dwSize.value
    if size < ctypes.sizeof(RAWINPUTHEADER) + 16:
        return None
    buf = ctypes.create_string_buffer(size)
    if user32.GetRawInputData(wintypes.HANDLE(lparam), RID_INPUT, buf, ctypes.byref(dwSize), ctypes.sizeof(RAWINPUTHEADER)) == 0xFFFFFFFF:
        return None
    header = RAWINPUTHEADER.from_buffer_copy(buf)
    if header.dwType != RIM_TYPEMOUSE:
        return None
    offset = ctypes.sizeof(RAWINPUTHEADER) + 12 
    if offset + 4 > size: return None
    return int(ctypes.c_long.from_buffer_copy(buf[offset:offset+4]).value)

# ---------- Utils & State ----------
_fg_cache_tick = 0
_fg_cache_value = False
_running_cache_tick = 0
_running_cache_value = False
_fov_last_sample_tick = 0
# _fov_worker_lock removed
# _fov_worker_active removed
# _fov_worker_result removed
_last_focus_update_tick = 0
_force_full_clear = True

def _is_fortnite_proc_name(proc_name: str) -> bool:
    name = proc_name.lower()
    return name in FORTNITE_EXES_LOWER or name.startswith(FORTNITE_EXE_PREFIX)

def is_fortnite_running() -> bool:
    global _running_cache_tick, _running_cache_value
    tick = win32api.GetTickCount()
    if (tick - _running_cache_tick) < 500:
        return _running_cache_value
    try:
        for p in psutil.process_iter(["name"]):
            name = p.info.get("name")
            if name and _is_fortnite_proc_name(name):
                _running_cache_tick = tick
                _running_cache_value = True
                return True
    except (psutil.Error, OSError):
        _running_cache_tick = tick
        _running_cache_value = False
        return False
    _running_cache_tick = tick
    _running_cache_value = False
    return False

def is_our_window(hwnd) -> bool:
    if not hwnd:
        return False
    if hwnd in (globals().get("overlay_hwnd_global"), globals().get("settings_hwnd"), globals().get("selector_hwnd")):
        return True
    try:
        _, pid = win32process.GetWindowThreadProcessId(hwnd)
        return pid == os.getpid()
    except (pywintypes.error, OSError):
        return False

_fg_cache_hwnd = 0
_fg_cache_hwnd_res = False

def is_fortnite_foreground(force_refresh=False) -> bool:
    global _fg_cache_tick, _fg_cache_value, diag_focus_hwnd, diag_focus_pid, diag_focus_proc
    global _fg_cache_hwnd, _fg_cache_hwnd_res

    tick = win32api.GetTickCount()
    if not force_refresh and (tick - _fg_cache_tick) < FG_CACHE_MS:
        return _fg_cache_value

    hwnd = win32gui.GetForegroundWindow()
    if not hwnd:
        diag_focus_hwnd = 0
        diag_focus_pid = 0
        diag_focus_proc = "none"
        _fg_cache_tick = tick
        _fg_cache_value = False
        return False

    # Optimization: If HWND is same as last check, return cached result immediately
    # BUT we still need to respect force_refresh or cache timeout?
    # No, if HWND hasn't changed, the process behind it likely hasn't changed either.
    # However, we should still update tick.
    if hwnd == _fg_cache_hwnd and not force_refresh:
         _fg_cache_tick = tick
         return _fg_cache_hwnd_res

    # New window focused or forced refresh
    _fg_cache_hwnd = hwnd
    if is_our_window(hwnd):
        diag_focus_hwnd = int(hwnd)
        diag_focus_pid = os.getpid()
        diag_focus_proc = "angle-ui"
        _fg_cache_tick = tick
        _fg_cache_value = False
        _fg_cache_hwnd_res = False
        return False
    try:
        _, pid = win32process.GetWindowThreadProcessId(hwnd)
        proc_name = psutil.Process(pid).name()
        diag_focus_hwnd = int(hwnd)
        diag_focus_pid = int(pid)
        diag_focus_proc = proc_name
        is_fn = _is_fortnite_proc_name(proc_name)
        _fg_cache_value = is_fn
        _fg_cache_hwnd_res = is_fn
    except (psutil.Error, pywintypes.error, OSError):
        diag_focus_hwnd = int(hwnd) if hwnd else 0
        diag_focus_pid = 0
        diag_focus_proc = "error"
        _fg_cache_value = False
        _fg_cache_hwnd_res = False
    _fg_cache_tick = tick
    return _fg_cache_value

def norm360(a): return a % 360.0
def angle_diff(a, b): return ((b - a + 540) % 360) - 180

def set_diag_error(message):
    global diag_last_error, diag_error_count
    diag_last_error = str(message)[:220]
    diag_error_count += 1

def maybe_log_diagnostics():
    global diag_last_log_tick
    if not diag_enabled:
        return
    now = win32api.GetTickCount()
    if (now - diag_last_log_tick) < 1500:
        return
    diag_last_log_tick = now
    logger.info(
        "DIAG tracking=%s paused=%s reason=%s fg=%s pid=%s timer_ms=%s input=%s fov_step=%s fov_samples=%s fov_scan_ms=%.2f skip=%s err_count=%s",
        tracking_active,
        bool(paused_line),
        diag_pause_reason,
        diag_focus_proc,
        diag_focus_pid,
        diag_timer_delta_ms,
        diag_input_count,
        diag_fov_step,
        diag_fov_samples,
        diag_fov_scan_ms,
        diag_fov_skip,
        diag_error_count,
    )

def load_json(path, default):
    if not os.path.exists(path):
        return default
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError, TypeError, ValueError) as exc:
        logger.warning("Failed to load JSON from %s: %s", path, exc)
        return default

def save_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)

def profile_key(dpi, sens): return f"dpi={dpi}_sens={sens:.6f}"

# Keybinds Configuration
ACTION_ORDER = [
    ("Toggle Crosshair", HK_CROSS),
    ("Open Settings",    HK_CFG), 
    ("Pick Prompt Color", HK_COLOR),
    ("Pick Prompt ROI",  HK_ROI),
    ("Cal Prompt NONE", HK_PNONE),
    ("Cal Prompt SKYDIVE", HK_PSKY),
    ("Cal Prompt UMBRELLA", HK_PUMB),
    ("Toggle Diagnostics", HK_DIAG),
    ("Set Zero",         HK_ZERO),
    ("Cal A (0 deg)",    HK_CAL_A), 
    ("Cal B (120 deg)",  HK_CAL_B), 
    ("Cal B (120 deg)",  HK_CAL_B), 
    ("Setup Wizard",     HK_WIZARD),
    ("Exit",             HK_EXIT)
]

DEFAULT_BINDS = {
    str(HK_CROSS): {"mod": 0, "vk": 0x78},  # Default: F9
    str(HK_CAL_A): {"mod": int(win32con.MOD_CONTROL), "vk": 0x77},
    str(HK_CAL_B): {"mod": int(win32con.MOD_CONTROL), "vk": 0x78},
    str(HK_ZERO):  {"mod": int(win32con.MOD_CONTROL), "vk": 0x79},
    str(HK_EXIT):  {"mod": int(win32con.MOD_CONTROL), "vk": 0x7B},
    str(HK_CFG):   {"mod": int(win32con.MOD_CONTROL), "vk": ord('U')},
    str(HK_ROI):   {"mod": int(win32con.MOD_CONTROL), "vk": ord('P')},
    str(HK_DIAG):  {"mod": 0, "vk": 0x77},  # F8
    str(HK_COLOR): {"mod": int(win32con.MOD_CONTROL), "vk": ord('O')},
    str(HK_PNONE): {"mod": int(win32con.MOD_CONTROL), "vk": ord('1')},
    str(HK_PSKY):  {"mod": int(win32con.MOD_CONTROL), "vk": ord('2')},
    str(HK_PSKY):  {"mod": int(win32con.MOD_CONTROL), "vk": ord('2')},
    str(HK_PUMB):  {"mod": int(win32con.MOD_CONTROL), "vk": ord('3')},
    str(HK_WIZARD): {"mod": int(win32con.MOD_CONTROL), "vk": ord('4')},
}

binds = load_json(BINDS_PATH, DEFAULT_BINDS)
for k,v in DEFAULT_BINDS.items(): binds.setdefault(k,v)

def mod_to_str(mod):
    p = []
    if mod & win32con.MOD_CONTROL: p.append("Ctrl")
    if mod & win32con.MOD_ALT: p.append("Alt")
    if mod & win32con.MOD_SHIFT: p.append("Shift")
    return "+".join(p)

def vk_to_str(vk):
    if 0x30 <= vk <= 0x39 or 0x41 <= vk <= 0x5A:
        return chr(vk)
    if 0x70 <= vk <= 0x87:
        return f"F{vk - 0x6F}"
    if 0x60 <= vk <= 0x69:
        return f"Num{vk - 0x60}"
    m = {
        27: "Esc",
        32: "Space",
        13: "Enter",
        8: "BkSp",
        9: "Tab",
        20: "Caps",
        33: "PGUP",
        34: "PGDN",
        35: "End",
        36: "Home",
        37: "Left",
        38: "Up",
        39: "Right",
        40: "Down",
        45: "INS",
        46: "DEL",
        91: "LWin",
        92: "RWin",
        93: "Menu",
        106: "Num*",
        107: "Num+",
        109: "Num-",
        110: "Num.",
        111: "Num/",
    }
    return m.get(vk, f"K_{vk}")

def bind_to_str(mod, vk):
    m = mod_to_str(mod)
    k = vk_to_str(vk)
    return f"{m}+{k}" if m else k

def load_fov_detector_config():
    cfg = load_json(FOV_DETECT_PATH, {})
    if not isinstance(cfg, dict):
        cfg = {}
    out = dict(FOV_DETECT_DEFAULT)
    out.update({k: v for k, v in cfg.items() if k in out and k != "roi_norm"})

    roi_default = FOV_DETECT_DEFAULT["roi_norm"]
    roi_in = cfg.get("roi_norm", {})
    if not isinstance(roi_in, dict):
        roi_in = {}
    roi = {
        "x": float(roi_in.get("x", roi_default["x"])),
        "y": float(roi_in.get("y", roi_default["y"])),
        "w": float(roi_in.get("w", roi_default["w"])),
        "h": float(roi_in.get("h", roi_default["h"])),
    }

    roi["x"] = min(max(0.0, roi["x"]), 1.0)
    roi["y"] = min(max(0.0, roi["y"]), 1.0)
    roi["w"] = min(max(0.01, roi["w"]), 1.0 - roi["x"])
    roi["h"] = min(max(0.01, roi["h"]), 1.0 - roi["y"])
    out["roi_norm"] = roi
    out["coarse_mode"] = bool(out.get("coarse_mode", True))
    out["coarse_cols"] = min(max(4, int(out.get("coarse_cols", 8))), 40)
    out["coarse_rows"] = min(max(2, int(out.get("coarse_rows", 4))), 20)
    out["use_target_color"] = bool(out.get("use_target_color", True))
    trgb = out.get("target_rgb", [255, 255, 255])
    if not isinstance(trgb, (list, tuple)) or len(trgb) != 3:
        trgb = [255, 255, 255]
    out["target_rgb"] = [
        min(max(int(trgb[0]), 0), 255),
        min(max(int(trgb[1]), 0), 255),
        min(max(int(trgb[2]), 0), 255),
    ]
    out["color_tolerance"] = min(max(int(out.get("color_tolerance", 24)), 0), 80)
    out["sample_step"] = max(1, int(out.get("sample_step", 3)))
    out["min_sample_interval_ms"] = min(max(16, int(out.get("min_sample_interval_ms", 80))), 500)
    out["max_samples"] = min(max(16, int(out.get("max_samples", 48))), 128)
    out["max_scan_time_ms"] = min(max(4, int(out.get("max_scan_time_ms", 10))), 40)
    out["white_min_rgb"] = min(max(int(out.get("white_min_rgb", 205)), 0), 255)
    out["white_max_channel_delta"] = min(max(int(out.get("white_max_channel_delta", 28)), 0), 255)
    out["ratio_skydive_min"] = max(0.0, float(out.get("ratio_skydive_min", 0.010)))
    out["ratio_umbrella_min"] = max(out["ratio_skydive_min"], float(out.get("ratio_umbrella_min", 0.028)))
    out["ratio_hysteresis"] = max(0.0, float(out.get("ratio_hysteresis", 0.003)))
    out["confirm_frames"] = min(max(1, int(out.get("confirm_frames", 3))), 8)
    out["cal_hysteresis"] = max(0.0, float(out.get("cal_hysteresis", 0.004)))
    raw_cal = out.get("ratio_calibration", {})
    if not isinstance(raw_cal, dict):
        raw_cal = {}
    cal = {}
    for key in (PROMPT_NONE, PROMPT_SKYDIVE, PROMPT_UMBRELLA):
        v = raw_cal.get(key)
        if v is None:
            cal[key] = None
        else:
            try:
                cal[key] = float(v)
            except (TypeError, ValueError):
                cal[key] = None
    out["ratio_calibration"] = cal
    raw_map = out.get("prompt_mode_map", {})
    if not isinstance(raw_map, dict):
        raw_map = {}
    mode_map = {}
    for key in (PROMPT_NONE, PROMPT_SKYDIVE, PROMPT_UMBRELLA):
        mode = str(raw_map.get(key, FOV_DETECT_DEFAULT["prompt_mode_map"][key])).upper()
        mode_map[key] = FOV_MODE_FREEFALL if mode == FOV_MODE_FREEFALL else FOV_MODE_NORMAL
    out["prompt_mode_map"] = mode_map
    out["freefall_scale_multiplier"] = max(0.1, float(out.get("freefall_scale_multiplier", 1.0)))
    out["enabled"] = bool(out.get("enabled", True))
    return out

def get_prompt_roi_px(cfg):
    roi = cfg["roi_norm"]
    x = int(roi["x"] * SCREEN_W)
    y = int(roi["y"] * SCREEN_H)
    w = max(1, int(roi["w"] * SCREEN_W))
    h = max(1, int(roi["h"] * SCREEN_H))
    if x + w > SCREEN_W:
        w = SCREEN_W - x
    if y + h > SCREEN_H:
        h = SCREEN_H - y
    return x, y, w, h

def effective_sample_step(w, h, base_step, max_samples):
    step = max(1, int(base_step))
    est_x = (w + step - 1) // step
    est_y = (h + step - 1) // step
    est_total = est_x * est_y
    if est_total <= max_samples:
        return step
    scale = (est_total / float(max_samples)) ** 0.5
    return max(step, int(step * scale) + 1)

def matches_prompt_pixel(r, g, b, use_target_color, target_rgb, color_tolerance, white_min_rgb, max_channel_delta):
    if use_target_color:
        return (
            abs(r - target_rgb[0]) <= color_tolerance
            and abs(g - target_rgb[1]) <= color_tolerance
            and abs(b - target_rgb[2]) <= color_tolerance
        )
    if r < white_min_rgb or g < white_min_rgb or b < white_min_rgb:
        return False
    return max(abs(r - g), abs(r - b), abs(g - b)) <= max_channel_delta

def sample_white_ratio(
    x,
    y,
    w,
    h,
    step,
    use_target_color,
    target_rgb,
    color_tolerance,
    white_min_rgb,
    max_channel_delta,
    max_scan_time_ms,
    max_samples,
):
    hdc = win32gui.GetDC(0)
    white = 0
    total = 0
    timed_out = False
    capped = False
    start_tick = win32api.GetTickCount()
    try:
        for py in range(y, y + h, step):
            if (win32api.GetTickCount() - start_tick) >= max_scan_time_ms:
                timed_out = True
                break
            for px in range(x, x + w, step):
                c = win32gui.GetPixel(hdc, px, py)
                if c == -1:
                    continue
                # COLORREF returned by GetPixel is 0x00bbggrr
                r = c & 0xFF
                g = (c >> 8) & 0xFF
                b = (c >> 16) & 0xFF
                total += 1
                if matches_prompt_pixel(
                    r, g, b, use_target_color, target_rgb, color_tolerance, white_min_rgb, max_channel_delta
                ):
                    white += 1
                if total >= max_samples:
                    capped = True
                    break
                # Avoid very long scans on slow systems.
                if (total & 0x3F) == 0 and (win32api.GetTickCount() - start_tick) >= max_scan_time_ms:
                    timed_out = True
                    break
                if (total & 0x7F) == 0:
                    # Yield so UI/input thread can run.
                    time.sleep(0)
            if timed_out or capped:
                break
    finally:
        win32gui.ReleaseDC(0, hdc)
    return ((white / total) if total else 0.0), total, timed_out, capped

def capture_roi_bgra(x, y, w, h):
    if w <= 0 or h <= 0:
        return None
    hdc_screen = user32.GetDC(0)
    if not hdc_screen:
        return None
    
    # We use a persistent compatible DC/bitmap for speed if possible, 
    # but here we just do a one-off capture.
    hdc_mem = gdi32.CreateCompatibleDC(hdc_screen)
    hbmp = gdi32.CreateCompatibleBitmap(hdc_screen, w, h)
    old_obj = gdi32.SelectObject(hdc_mem, hbmp)
    
    try:
        gdi32.BitBlt(hdc_mem, 0, 0, w, h, hdc_screen, x, y, SRCCOPY)
        
        bmi = BITMAPINFO()
        bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
        bmi.bmiHeader.biWidth = w
        bmi.bmiHeader.biHeight = -h  # top-down
        bmi.bmiHeader.biPlanes = 1
        bmi.bmiHeader.biBitCount = 32
        bmi.bmiHeader.biCompression = DIB_RGB_COLORS
        
        buf = ctypes.create_string_buffer(w * h * 4)
        lines = gdi32.GetDIBits(
            hdc_mem,
            hbmp,
            0,
            h,
            ctypes.byref(buf),
            ctypes.byref(bmi),
            DIB_RGB_COLORS,
        )
        if lines != h:
            return None
        return buf
    finally:
        if old_obj:
            gdi32.SelectObject(hdc_mem, old_obj)
        gdi32.DeleteObject(hbmp)
        gdi32.DeleteDC(hdc_mem)
        user32.ReleaseDC(0, hdc_screen)

def get_screen_pixel_rgb(x, y):
    hdc = user32.GetDC(0)
    c = gdi32.GetPixel(hdc, x, y)
    user32.ReleaseDC(0, hdc)
    if c == -1: return None
    return (c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF)


def sample_white_ratio_coarse(
    x,
    y,
    w,
    h,
    cols,
    rows,
    use_target_color,
    target_rgb,
    color_tolerance,
    white_min_rgb,
    max_channel_delta,
    max_scan_time_ms,
):
    buf = capture_roi_bgra(x, y, w, h)
    if buf is None:
        return 0.0, 0, True
    raw = buf.raw
    stride = w * 4
    white = 0
    total = 0
    timed_out = False
    start_tick = win32api.GetTickCount()
    for iy in range(rows):
        if (win32api.GetTickCount() - start_tick) >= max_scan_time_ms:
            timed_out = True
            break
        py = int(((iy + 0.5) * h) / rows)
        if py < 0:
            py = 0
        elif py >= h:
            py = h - 1
        row_off = py * stride
        for ix in range(cols):
            px = int(((ix + 0.5) * w) / cols)
            if px < 0:
                px = 0
            elif px >= w:
                px = w - 1
            off = row_off + (px * 4)
            b = raw[off]
            g = raw[off + 1]
            r = raw[off + 2]
            total += 1
            if matches_prompt_pixel(
                r, g, b, use_target_color, target_rgb, color_tolerance, white_min_rgb, max_channel_delta
            ):
                white += 1
    return ((white / total) if total else 0.0), total, timed_out

# --- FOV Worker (Persistent) ---
import queue
_fov_request_queue = queue.Queue(maxsize=1)
_fov_result_queue = queue.Queue(maxsize=1)
_fov_worker_thread = None

def fov_worker_loop():
    while True:
        try:
            req = _fov_request_queue.get(timeout=1.0) # Check every 1s if idle
        except queue.Empty:
            continue
            
        if req is None: # Sentinel
            break
            
        x, y, w, h, step, cfg = req
        start_tick = win32api.GetTickCount()
        result = None
        
        try:
            white_min_rgb = cfg["white_min_rgb"]
            max_channel_delta = cfg["white_max_channel_delta"]
            max_scan_time_ms = cfg["max_scan_time_ms"]
            max_samples = cfg["max_samples"]
            coarse_mode = bool(cfg.get("coarse_mode", True))
            coarse_cols = int(cfg.get("coarse_cols", 8))
            coarse_rows = int(cfg.get("coarse_rows", 4))
            use_target_color = bool(cfg.get("use_target_color", True))
            target_rgb = cfg.get("target_rgb", [255, 255, 255])
            color_tolerance = int(cfg.get("color_tolerance", 24))

            if coarse_mode:
                cols = max(4, coarse_cols)
                rows = max(2, coarse_rows)
            else:
                s = max(1, int(step))
                cols = max(4, (w + s - 1) // s)
                rows = max(2, (h + s - 1) // s)
                estimate = cols * rows
                if estimate > max_samples:
                    scale = (estimate / float(max_samples)) ** 0.5
                    cols = max(4, int(cols / scale))
                    rows = max(2, int(rows / scale))
            cols = min(cols, max(4, w))
            rows = min(rows, max(2, h))

            ratio, samples, timed_out = sample_white_ratio_coarse(
                x,
                y,
                w,
                h,
                cols,
                rows,
                use_target_color,
                target_rgb,
                color_tolerance,
                white_min_rgb,
                max_channel_delta,
                max_scan_time_ms,
            )
            capped = (cols * rows) >= max_samples
            result = {
                "ratio": ratio,
                "samples": samples,
                "step": f"{cols}x{rows}",
                "scan_ms": float(win32api.GetTickCount() - start_tick),
                "timed_out": timed_out,
                "capped": capped,
            }
        except Exception as exc:
            result = {"error": str(exc)}
            
        # Push result (non-blocking, overwrite/discard if full unlikely as we process one at a time)
        try:
             _fov_result_queue.put_nowait(result)
        except queue.Full:
             pass 
        _fov_request_queue.task_done()

def ensure_fov_worker_running():
    global _fov_worker_thread
    if _fov_worker_thread is None or not _fov_worker_thread.is_alive():
        _fov_worker_thread = threading.Thread(target=fov_worker_loop, daemon=True, name="fov-worker")
        _fov_worker_thread.start()

def pop_fov_worker_result():
    try:
        return _fov_result_queue.get_nowait()
    except queue.Empty:
        return None

def start_fov_worker_scan(x, y, w, h, step, cfg):
    ensure_fov_worker_running()
    # If a request is already pending, we don't need to stack another one. 
    # Just skip this frame's request.
    if _fov_request_queue.full():
        return False
        
    try:
        _fov_request_queue.put_nowait((x, y, w, h, step, cfg))
        return True
    except queue.Full:
        return False

def classify_prompt_by_ratio(ratio, prev_prompt, cfg):
    cal = cfg.get("ratio_calibration", {})
    cal_vals = {}
    for key in (PROMPT_NONE, PROMPT_SKYDIVE, PROMPT_UMBRELLA):
        v = cal.get(key)
        if isinstance(v, (int, float)):
            cal_vals[key] = float(v)
    if len(cal_vals) >= 2:
        ordered = sorted(cal_vals.items(), key=lambda kv: abs(ratio - kv[1]))
        best_prompt, best_dist = ordered[0]
        if prev_prompt in cal_vals and prev_prompt != best_prompt:
            prev_dist = abs(ratio - cal_vals[prev_prompt])
            if prev_dist <= (best_dist + cfg.get("cal_hysteresis", 0.004)):
                return prev_prompt
        return best_prompt

    sky = cfg["ratio_skydive_min"]
    umb = cfg["ratio_umbrella_min"]
    hys = cfg["ratio_hysteresis"]

    if prev_prompt == PROMPT_UMBRELLA:
        if ratio >= (umb - hys):
            return PROMPT_UMBRELLA
    if prev_prompt == PROMPT_SKYDIVE:
        if ratio >= (sky - hys) and ratio < umb:
            return PROMPT_SKYDIVE

    if ratio >= umb:
        return PROMPT_UMBRELLA
    if ratio >= sky:
        return PROMPT_SKYDIVE
    return PROMPT_NONE

def mode_for_prompt(prompt):
    mode = fov_detect_cfg.get("prompt_mode_map", {}).get(prompt, FOV_MODE_NORMAL)
    return FOV_MODE_FREEFALL if mode == FOV_MODE_FREEFALL else FOV_MODE_NORMAL

def lparam_point(lparam):
    x = ctypes.c_short(lparam & 0xFFFF).value
    y = ctypes.c_short((lparam >> 16) & 0xFFFF).value
    return x, y

def normalized_roi_from_points(x1, y1, x2, y2):
    left = max(0, min(x1, x2))
    top = max(0, min(y1, y2))
    right = min(SCREEN_W, max(x1, x2))
    bottom = min(SCREEN_H, max(y1, y2))
    w = right - left
    h = bottom - top
    if w < 8 or h < 8:
        return None
    return {
        "x": left / float(SCREEN_W),
        "y": top / float(SCREEN_H),
        "w": w / float(SCREEN_W),
        "h": h / float(SCREEN_H),
    }

def save_prompt_roi_from_points(x1, y1, x2, y2):
    global fov_detect_cfg, status_line
    roi_norm = normalized_roi_from_points(x1, y1, x2, y2)
    if roi_norm is None:
        status_line = "Prompt ROI too small. Drag a larger area."
        return False
    fov_detect_cfg["roi_norm"] = roi_norm
    save_json(FOV_DETECT_PATH, fov_detect_cfg)
    status_line = "Prompt ROI updated."
    logger.info("Prompt ROI updated to %s", roi_norm)
    return True

def get_screen_pixel_rgb(x, y):
    hdc = win32gui.GetDC(0)
    try:
        c = win32gui.GetPixel(hdc, x, y)
        if c == -1:
            return None
        return (c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF)
    finally:
        win32gui.ReleaseDC(0, hdc)

def save_prompt_target_color(rgb):
    global fov_detect_cfg, status_line
    if rgb is None:
        status_line = "Color pick failed."
        return False
    fov_detect_cfg["target_rgb"] = [int(rgb[0]), int(rgb[1]), int(rgb[2])]
    fov_detect_cfg["use_target_color"] = True
    fov_detect_cfg["color_tolerance"] = 10  # Relaxed from 0 to 10 for better detection
    save_json(FOV_DETECT_PATH, fov_detect_cfg)
    status_line = f"Prompt color set to RGB{tuple(fov_detect_cfg['target_rgb'])}"
    logger.info("Prompt color updated to %s (tolerance=10)", fov_detect_cfg["target_rgb"])
    return True

def calibrate_prompt_ratio(label):
    global fov_detect_cfg, status_line
    if label not in (PROMPT_NONE, PROMPT_SKYDIVE, PROMPT_UMBRELLA):
        return
    if diag_fov_samples <= 0:
        status_line = "No detector sample yet. Wait 1s and try again."
        return
    fov_detect_cfg["ratio_calibration"][label] = float(prompt_white_ratio)
    save_json(FOV_DETECT_PATH, fov_detect_cfg)
    status_line = f"Calibrated {label}: {prompt_white_ratio:.4f}"
    logger.info("Prompt calibration %s=%s", label, prompt_white_ratio)

def hide_console_window():
    try:
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        user32_local = ctypes.WinDLL("user32", use_last_error=True)
        console_hwnd = kernel32.GetConsoleWindow()
        if console_hwnd:
            user32_local.ShowWindow(console_hwnd, win32con.SW_HIDE)
    except Exception as exc:
        logger.debug("Console hide skipped: %s", exc)

def load_initial_profile_values():
    state = load_json(APP_STATE_PATH, {})
    dpi = state.get("dpi", 800)
    sens = state.get("sens", 6.5)
    try:
        dpi = int(dpi)
    except (TypeError, ValueError):
        dpi = 800
    try:
        sens = float(sens)
    except (TypeError, ValueError):
        sens = 6.5
    return max(1, dpi), max(0.001, sens)

def prompt_profile_ui(default_dpi, default_sens):
    try:
        import tkinter as tk
        from tkinter import messagebox
    except Exception as exc:
        logger.warning("Tkinter unavailable. Using defaults. %s", exc)
        return default_dpi, default_sens

    selected = {"value": None}
    try:
        root = tk.Tk()
    except Exception as exc:
        logger.warning("Failed to open setup window. Using defaults. %s", exc)
        return default_dpi, default_sens
    root.title("Angle Overlay Setup")
    root.resizable(False, False)
    root.attributes("-topmost", True)
    root.configure(bg="#242a33")

    title = tk.Label(root, text="Angle Profile", fg="#f5f8ff", bg="#242a33", font=("Segoe UI", 12, "bold"))
    title.grid(row=0, column=0, columnspan=2, padx=14, pady=(12, 4), sticky="w")

    tk.Label(root, text="DPI", fg="#c8d0df", bg="#242a33", font=("Segoe UI", 10)).grid(row=1, column=0, padx=(14, 6), pady=4, sticky="w")
    dpi_var = tk.StringVar(value=str(default_dpi))
    dpi_entry = tk.Entry(root, textvariable=dpi_var, width=18, font=("Segoe UI", 10))
    dpi_entry.grid(row=1, column=1, padx=(6, 14), pady=4, sticky="w")

    tk.Label(root, text="Sensitivity", fg="#c8d0df", bg="#242a33", font=("Segoe UI", 10)).grid(row=2, column=0, padx=(14, 6), pady=4, sticky="w")
    sens_var = tk.StringVar(value=f"{default_sens:g}")
    sens_entry = tk.Entry(root, textvariable=sens_var, width=18, font=("Segoe UI", 10))
    sens_entry.grid(row=2, column=1, padx=(6, 14), pady=4, sticky="w")

    info = tk.Label(root, text="Start opens overlay. Cancel exits.", fg="#9aa4b7", bg="#242a33", font=("Segoe UI", 9))
    info.grid(row=3, column=0, columnspan=2, padx=14, pady=(4, 8), sticky="w")

    def on_start():
        try:
            dpi = int(dpi_var.get().strip())
            sens = float(sens_var.get().strip())
        except ValueError:
            messagebox.showerror("Invalid Input", "DPI must be an integer and sensitivity must be numeric.")
            return
        if dpi <= 0 or sens <= 0:
            messagebox.showerror("Invalid Input", "DPI and sensitivity must be greater than 0.")
            return
        selected["value"] = (dpi, sens)
        root.destroy()

    def on_cancel():
        root.destroy()

    buttons = tk.Frame(root, bg="#242a33")
    buttons.grid(row=4, column=0, columnspan=2, padx=14, pady=(0, 12), sticky="e")
    tk.Button(buttons, text="Start", width=10, command=on_start).pack(side="left", padx=(0, 8))
    tk.Button(buttons, text="Cancel", width=10, command=on_cancel).pack(side="left")

    root.bind("<Return>", lambda *_: on_start())
    root.protocol("WM_DELETE_WINDOW", on_cancel)
    dpi_entry.focus_set()
    root.mainloop()

    if selected["value"] is None:
        raise SystemExit(0)
    return selected["value"]

# State Variables
profiles = load_json(PROFILE_PATH, {})
fov_detect_cfg = load_fov_detector_config()
if not os.path.exists(FOV_DETECT_PATH):
    save_json(FOV_DETECT_PATH, fov_detect_cfg)
hide_console_window()
DPI, SENS = prompt_profile_ui(*load_initial_profile_values())
save_json(APP_STATE_PATH, {"dpi": DPI, "sens": SENS})

KEY = profile_key(DPI, SENS)
accum_dx = 0.0
calA_dx, calB_dx = None, None
base_dx, base_angle = 0.0, 0.0
scale_deg_per_dx_normal = None
scale_deg_per_dx_freefall = None
angle_smoothed = None
DIRECTION = DIRECTION_DEFAULT
show_crosshair = False
prompt_state = PROMPT_NONE
prompt_candidate = PROMPT_NONE
prompt_candidate_frames = 0
prompt_white_ratio = 0.0
fov_mode = FOV_MODE_NORMAL
fov_mode_target = FOV_MODE_NORMAL
fov_is_transitioning = False
fov_transition_start_tick = 0
fov_transition_source_scale = 0.0
fov_transition_target_scale = 0.0
fov_transition_duration = 1000
tracking_active = False
diag_enabled = True
diag_last_error = ""
diag_error_count = 0
diag_focus_hwnd = 0
diag_focus_pid = 0
diag_focus_proc = "unknown"
diag_pause_reason = "init"
diag_input_count = 0
diag_last_input_tick = 0
diag_timer_count = 0
diag_last_timer_tick = 0
diag_timer_delta_ms = 0
diag_last_log_tick = 0
diag_fov_scan_ms = 0.0
diag_fov_step = "0"
diag_fov_samples = 0
diag_fov_skip = ""

profile_entry = profiles.get(KEY, {})
if isinstance(profile_entry, dict):
    try:
        if "scale_deg_per_dx_normal" in profile_entry:
            scale_deg_per_dx_normal = float(profile_entry["scale_deg_per_dx_normal"])
        else:
            scale_deg_per_dx_normal = float(profile_entry["scale_deg_per_dx"])
        if "scale_deg_per_dx_freefall" in profile_entry:
            scale_deg_per_dx_freefall = float(profile_entry["scale_deg_per_dx_freefall"])
        DIRECTION = int(profile_entry.get("direction", 1))
        if scale_deg_per_dx_freefall is not None:
            status_line = "Ready (normal + freefall)."
        elif fov_detect_cfg["freefall_scale_multiplier"] != 1.0:
            status_line = "Ready (freefall uses multiplier)."
        else:
            status_line = "Ready (calibrate freefall for max accuracy)."
    except (KeyError, TypeError, ValueError):
        scale_deg_per_dx_normal = None
        scale_deg_per_dx_freefall = None
        status_line = "Profile invalid. Please Calibrate."
else:
    status_line = "New Profile. Please Calibrate."

paused_line = ""

def set_zero():
    global base_dx, base_angle, angle_smoothed
    base_dx, base_angle, angle_smoothed = accum_dx, 0.0, None

def get_active_scale():
    if fov_mode == FOV_MODE_FREEFALL:
        if scale_deg_per_dx_freefall is not None:
            return scale_deg_per_dx_freefall
        if scale_deg_per_dx_normal is not None:
            return scale_deg_per_dx_normal * fov_detect_cfg["freefall_scale_multiplier"]
        return None
    return scale_deg_per_dx_normal

def update_fov_mode():
    global prompt_state, prompt_candidate, prompt_candidate_frames, prompt_white_ratio, fov_mode, _fov_last_sample_tick
    global diag_fov_scan_ms, diag_fov_step, diag_fov_samples, diag_fov_skip
    global fov_mode_target, fov_is_transitioning, fov_transition_start_tick
    global fov_transition_source_scale, fov_transition_target_scale, fov_transition_duration
    global base_angle, base_dx, angle_smoothed

    if not fov_detect_cfg["enabled"]:
        prompt_state = PROMPT_NONE
        prompt_white_ratio = 0.0
        fov_mode = FOV_MODE_NORMAL
        diag_fov_skip = "disabled"
        return
    if not tracking_active:
        prompt_state = PROMPT_NONE
        prompt_candidate = PROMPT_NONE
        prompt_candidate_frames = 0
        prompt_white_ratio = 0.0
        fov_mode = FOV_MODE_NORMAL
        diag_fov_skip = "not-tracking"
        return
    # Only scan pixels if Fortnite is foreground OR not running.
    # If Fortnite is running but backgrounded, we pause to not annoy the user.
    fn_running = is_fortnite_running()
    fn_focus = is_fortnite_foreground()
    
    if fn_running and not fn_focus:
        prompt_state = PROMPT_NONE
        prompt_candidate = PROMPT_NONE
        prompt_candidate_frames = 0
        prompt_white_ratio = 0.0
        fov_mode = FOV_MODE_NORMAL
        diag_fov_skip = "paused-bg"
        return
    if settings_hwnd and win32gui.IsWindow(settings_hwnd) and win32gui.IsWindowVisible(settings_hwnd):
        diag_fov_skip = "settings-open"
        return
    if selector_hwnd and win32gui.IsWindow(selector_hwnd):
        diag_fov_skip = "roi-select"
        return

    # Consume latest worker result if available.
    result = pop_fov_worker_result()
    if result is not None:
        if "error" in result:
            set_diag_error(f"fov worker error: {result['error']}")
            diag_fov_skip = "worker-error"
        else:
            diag_fov_scan_ms = float(result.get("scan_ms", 0.0))
            diag_fov_step = str(result.get("step", 0))
            diag_fov_samples = int(result.get("samples", 0))
            prompt_white_ratio = float(result.get("ratio", 0.0))
            if result.get("timed_out"):
                diag_fov_skip = "scan-timeout"
            elif result.get("capped"):
                diag_fov_skip = "sample-cap"
            else:
                diag_fov_skip = ""
            if result.get("scan_ms", 0.0) > 1000:
                set_diag_error(f"scan too slow: {result.get('scan_ms', 0.0):.1f}ms")

            raw_prompt = classify_prompt_by_ratio(prompt_white_ratio, prompt_state, fov_detect_cfg)
            if raw_prompt == prompt_candidate:
                prompt_candidate_frames += 1
            else:
                prompt_candidate = raw_prompt
                prompt_candidate_frames = 1
            if prompt_candidate_frames >= fov_detect_cfg["confirm_frames"]:
                prompt_state = prompt_candidate
            
            new_mode = mode_for_prompt(prompt_state)            # INSTANT SWITCH & LOCK STRATEGY
            if new_mode != fov_mode:
                 # 1. Rebase Angle using OLD mode (prevent jump)
                 curr = compute_angle()

                 # 2. Update Mode
                 prev_mode = fov_mode
                 fov_mode = new_mode

                 # 3. Apply Rebase
                 if curr is not None:
                     global base_angle, base_dx, angle_smoothed
                     base_angle = curr
                     base_dx = accum_dx
                     angle_smoothed = curr
                 
                 # 4. Lock Camera for 1s if switching to/from Dive/Glide
                 # The user specifically requested a "1s delay when changing to no mouse movement only"
                 trigger_camera_lock(1000)
                 # 3. Lock Camera for 1s if switching to/from Dive/Glide
                 # The user specifically requested a "1s delay when changing to no mouse movement only"
                 global camera_locked_until
                 camera_locked_until = win32api.GetTickCount() + 1000
                 lock_camera()

    now = win32api.GetTickCount()
    sample_interval_ms = int(fov_detect_cfg["min_sample_interval_ms"])
    if fov_detect_cfg.get("coarse_mode", True):
        sample_interval_ms = min(sample_interval_ms, 80)
    else:
        sample_interval_ms = min(sample_interval_ms, 120)
    if (now - _fov_last_sample_tick) < sample_interval_ms:
        # If we got a result this frame, we are happy. If not, and we are throttled, say so.
        if result is None:
             diag_fov_skip = "throttled"
        return

    # Try to start a new scan
    # If the queue is already full, we are busy.
    x, y, w, h = get_prompt_roi_px(fov_detect_cfg)
    step = effective_sample_step(
        w,
        h,
        fov_detect_cfg["sample_step"],
        fov_detect_cfg["max_samples"],
    )
    
    # This will return False if queue is full (worker busy)
    started = start_fov_worker_scan(x, y, w, h, step, fov_detect_cfg)
    if started:
        _fov_last_sample_tick = now
        if result is None:
             diag_fov_skip = "" # Started new scan
    else:
        # Worker was busy
        if result is None:
             diag_fov_skip = "worker-busy"

def update_focus_state():
    global paused_line, tracking_active, diag_pause_reason
    
    fn_running = is_fortnite_running()
    fn_focus = is_fortnite_foreground(force_refresh=True)

    if not fn_running or fn_focus:
        tracking_active = True
        paused_line = "" if fn_focus else "(TEST MODE)"
        diag_pause_reason = "fortnite-foreground" if fn_focus else "fortnite-not-running"
        return
        
    tracking_active = False
    paused_line = "PAUSED"
    
    # Ensure camera is unlocked if we lose focus mid-transition
    unlock_camera()
    fg_hwnd = win32gui.GetForegroundWindow()
    diag_pause_reason = "backgrounded"

def compute_angle():
    global angle_smoothed
    active_scale = get_active_scale()
    if active_scale is None:
        return None
    est = norm360(base_angle + (accum_dx - base_dx) * active_scale * DIRECTION)
    
    if SMOOTH_ALPHA > 0 and angle_smoothed is not None:
        angle_smoothed = norm360(angle_smoothed + SMOOTH_ALPHA * angle_diff(angle_smoothed, est))
        return angle_smoothed
    angle_smoothed = est
    return est

def overlay_panel_rect():
    extra_h = 115 if diag_enabled else 0
    # Include wizard area if active (wizard is drawn 240px below main box)
    if wizard_active:
        extra_h += 250
    return (BOX_X, BOX_Y, BOX_X + BOX_W, BOX_Y + BOX_H + extra_h)

# ---------------- FONT FUNCTION ----------------
def create_font(height=20, weight=400, face="Segoe UI"):
    lf = win32gui.LOGFONT()
    lf.lfHeight = height
    lf.lfWeight = weight
    lf.lfFaceName = face
    lf.lfQuality = win32con.CLEARTYPE_QUALITY
    return win32gui.CreateFontIndirect(lf)

def draw_text_font(hdc, x, y, text, font, color=C_WHITE):
    win32gui.SetTextColor(hdc, color)
    old_font = win32gui.SelectObject(hdc, font)
    try:
        win32gui.ExtTextOut(hdc, x, y, 0, None, text)
    finally:
        win32gui.SelectObject(hdc, old_font)

FONT_BIG = None
FONT_MED = None
FONT_SML = None
FONT_TINY = None
FONT_TITLE = None
FONT_ITEM = None

def init_fonts():
    global FONT_BIG, FONT_MED, FONT_SML, FONT_TINY, FONT_TITLE, FONT_ITEM
    FONT_BIG = create_font(48, 700, "Segoe UI")
    FONT_MED = create_font(20, 600, "Segoe UI")
    FONT_SML = create_font(16, 400, "Consolas")
    FONT_TINY = create_font(13, 500, "Segoe UI")
    FONT_TITLE = create_font(24, 600, "Segoe UI")
    FONT_ITEM = create_font(19, 400, "Segoe UI")

def cleanup_fonts():
    global FONT_BIG, FONT_MED, FONT_SML, FONT_TINY, FONT_TITLE, FONT_ITEM
    for font in (FONT_BIG, FONT_MED, FONT_SML, FONT_TINY, FONT_TITLE, FONT_ITEM):
        if font:
            win32gui.DeleteObject(font)
    FONT_BIG = FONT_MED = FONT_SML = FONT_TINY = FONT_TITLE = FONT_ITEM = None

# ---------------- FULLSCREEN OVERLAY PAINT ----------------
def paint_overlay(hwnd, hdc):
    global _force_full_clear
    try:
        rect = win32gui.GetClientRect(hwnd)
        panel_rect = overlay_panel_rect()
        # 1. Only force a full transparent clear when required (startup/crosshair mode changes).
        if _force_full_clear:
            brush_clear = win32gui.CreateSolidBrush(C_INVISIBLE)
            win32gui.FillRect(hdc, rect, brush_clear)
            win32gui.DeleteObject(brush_clear)
            _force_full_clear = False

        win32gui.SetBkMode(hdc, win32con.TRANSPARENT)

        # 2. Draw FULL SCREEN CROSSHAIR
        if show_crosshair:
            cx, cy = SCREEN_W // 2, SCREEN_H // 2
            brush_cross = win32gui.CreateSolidBrush(C_RED)
            win32gui.FillRect(hdc, (0, cy, SCREEN_W, cy + 1), brush_cross)
            win32gui.FillRect(hdc, (cx, 0, cx + 1, SCREEN_H), brush_cross)
            win32gui.DeleteObject(brush_cross)

        # 3. Draw Background Box for Angle Data
        box_h = panel_rect[3] - panel_rect[1]
        brush_box = win32gui.CreateSolidBrush(C_BOX_BG)
        win32gui.FillRect(hdc, panel_rect, brush_box)
        win32gui.DeleteObject(brush_box)

        # 3b. Draw ROI Frame (3mm => ~12px)
        # Green = Normal, Red = Freefall/Dive
        roi_px = get_prompt_roi_px(fov_detect_cfg)
        if roi_px:
             rx, ry, rw, rh = roi_px
             # Ensure we don't draw outside screen
             frame_col = C_RED if fov_mode == FOV_MODE_FREEFALL else C_GREEN
             draw_outline_rect(hdc, rx, ry, rx+rw, ry+rh, frame_col, thickness=2)

        # 4. Draw Angle Text
        ang = compute_angle()
        t_x = BOX_X + 20
        t_y = BOX_Y + 10

        if ang is not None:
            draw_text_font(hdc, t_x, t_y, "CURRENT ANGLE", FONT_SML, C_GREY)
            draw_text_font(hdc, t_x, t_y + 25, f"{ang:.1f}°", FONT_BIG, C_GREEN)
        else:
            draw_text_font(hdc, t_x, t_y, "NOT CALIBRATED", FONT_SML, C_RED)
            draw_text_font(hdc, t_x, t_y + 25, "---.-°", FONT_BIG, C_GREY)

        y_stats = t_y + 90
        draw_text_font(hdc, t_x, y_stats, f"Profile: {DPI} DPI / {SENS}", FONT_SML, C_WHITE)
        status_c = C_YELLOW if paused_line or get_active_scale() is None else C_WHITE
        if wizard_active:
             status_c = C_YELLOW
             draw_text_font(hdc, t_x, y_stats + 18, f"{status_line}", FONT_SML, status_c)
        else:
             draw_text_font(hdc, t_x, y_stats + 18, f"Status:  {status_line if not paused_line else paused_line}", FONT_SML, status_c)
        mode_color = C_GREEN if fov_mode == FOV_MODE_FREEFALL else C_WHITE
        draw_text_font(
            hdc,
            t_x,
            y_stats + 36,
            f"Mode:    {fov_mode} ({prompt_state}, {prompt_white_ratio * 100:.1f}%)",
            FONT_SML,
            mode_color,
        )
        t_x2 = t_x + 240
        trgb_main = fov_detect_cfg.get("target_rgb", [255, 255, 255])
        preview_rect_main = (t_x2, y_stats + 36, t_x2 + 15, y_stats + 51)
        preview_brush_main = win32gui.CreateSolidBrush(win32api.RGB(trgb_main[0], trgb_main[1], trgb_main[2]))
        win32gui.FillRect(hdc, preview_rect_main, preview_brush_main)
        win32gui.DeleteObject(preview_brush_main)

        # Binds help
        b_cross = bind_to_str(binds[str(HK_CROSS)]["mod"], binds[str(HK_CROSS)]["vk"])
        b_zero = bind_to_str(binds[str(HK_ZERO)]["mod"], binds[str(HK_ZERO)]["vk"])
        b_cfg = bind_to_str(binds[str(HK_CFG)]["mod"], binds[str(HK_CFG)]["vk"])
        b_color = bind_to_str(binds[str(HK_COLOR)]["mod"], binds[str(HK_COLOR)]["vk"])
        b_roi = bind_to_str(binds[str(HK_ROI)]["mod"], binds[str(HK_ROI)]["vk"])
        b_wizard = bind_to_str(binds[str(HK_WIZARD)]["mod"], binds[str(HK_WIZARD)]["vk"])
        b_diag = bind_to_str(binds[str(HK_DIAG)]["mod"], binds[str(HK_DIAG)]["vk"])
        b_pnone = bind_to_str(binds[str(HK_PNONE)]["mod"], binds[str(HK_PNONE)]["vk"])
        b_psky = bind_to_str(binds[str(HK_PSKY)]["mod"], binds[str(HK_PSKY)]["vk"])
        b_pumb = bind_to_str(binds[str(HK_PUMB)]["mod"], binds[str(HK_PUMB)]["vk"])
        help_x = BOX_X + 250
        help_val_x = help_x + 96
        draw_text_font(hdc, help_x, t_y, "XHAIR:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y, b_cross, FONT_MED, C_WHITE)
        draw_text_font(hdc, help_x, t_y + 30, "ZERO:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 30, b_zero, FONT_MED, C_WHITE)
        draw_text_font(hdc, help_x, t_y + 60, "SETTINGS:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 60, b_cfg, FONT_MED, C_YELLOW)
        draw_text_font(hdc, help_x, t_y + 90, "COLOR:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 90, b_color, FONT_MED, C_WHITE)
        draw_text_font(hdc, help_x, t_y + 120, "ROI:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 120, b_roi, FONT_MED, C_WHITE)
        draw_text_font(hdc, help_x, t_y + 150, "CAL:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 150, f"{b_pnone}/{b_psky}/{b_pumb}", FONT_SML, C_WHITE)
        draw_text_font(hdc, help_x, t_y + 168, "WIZARD:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 168, b_wizard, FONT_SML, C_YELLOW)
        draw_text_font(hdc, help_x, t_y + 186, "DIAG:", FONT_SML, C_GREY)
        draw_text_font(hdc, help_val_x, t_y + 186, b_diag, FONT_SML, C_WHITE)

        if diag_enabled:
            now_tick = win32api.GetTickCount()
            ms_since_input = (now_tick - diag_last_input_tick) if diag_last_input_tick else -1
            trgb = fov_detect_cfg.get("target_rgb", [255, 255, 255])
            cal = fov_detect_cfg.get("ratio_calibration", {})
            draw_text_font(
                hdc,
                t_x,
                y_stats + 56,
                f"DBG track={tracking_active} reason={diag_pause_reason}",
                FONT_SML,
                C_YELLOW,
            )
            draw_text_font(
                hdc,
                t_x,
                y_stats + 74,
                f"FG {diag_focus_proc} pid={diag_focus_pid} hwnd=0x{diag_focus_hwnd:X}",
                FONT_SML,
                C_GREY,
            )
            draw_text_font(
                hdc,
                t_x,
                y_stats + 92,
                f"TIMER {diag_timer_delta_ms}ms  INPUT dt={ms_since_input}ms cnt={diag_input_count}",
                FONT_SML,
                C_GREY,
            )
            draw_text_font(
                hdc,
                t_x,
                y_stats + 110,
                f"FOV scan={diag_fov_scan_ms:.1f}ms step={diag_fov_step} n={diag_fov_samples} skip={diag_fov_skip or '-'}",
                FONT_SML,
                C_GREY,
            )
            draw_text_font(
                hdc,
                t_x,
                y_stats + 128,
                f"CLR rgb=({trgb[0]},{trgb[1]},{trgb[2]}) tol={fov_detect_cfg.get('color_tolerance', 0)}",
                FONT_SML,
                C_GREY,
            )
            draw_text_font(
                hdc,
                t_x,
                y_stats + 146,
                f"CAL N={cal.get(PROMPT_NONE)} S={cal.get(PROMPT_SKYDIVE)} U={cal.get(PROMPT_UMBRELLA)}",
                FONT_SML,
                C_GREY,
            )
            if diag_last_error:
                draw_text_font(hdc, t_x, y_stats + 164, f"ERR {diag_last_error}", FONT_SML, C_RED)

        # 6. WIZARD UI EXTENSION
        if wizard_active:
             # Calculate position below the main box
             wiz_y = BOX_Y + box_h + 10
             wiz_h = 240 # Height for instructions
             wiz_rect = (BOX_X, wiz_y, BOX_X + BOX_W, wiz_y + wiz_h)
             
             # Background
             brush_wiz = win32gui.CreateSolidBrush(C_BOX_BG) 
             win32gui.FillRect(hdc, wiz_rect, brush_wiz)
             win32gui.DeleteObject(brush_wiz)
             
             # Border (Yellow)
             pen_wiz = win32gui.CreatePen(win32con.PS_SOLID, 2, C_YELLOW)
             old_pen = win32gui.SelectObject(hdc, pen_wiz)
             win32gui.MoveToEx(hdc, wiz_rect[0], wiz_rect[1])
             win32gui.LineTo(hdc, wiz_rect[2], wiz_rect[1])
             win32gui.LineTo(hdc, wiz_rect[2], wiz_rect[3])
             win32gui.LineTo(hdc, wiz_rect[0], wiz_rect[3])
             win32gui.LineTo(hdc, wiz_rect[0], wiz_rect[1])
             win32gui.SelectObject(hdc, old_pen)
             win32gui.DeleteObject(pen_wiz)

             wx = wiz_rect[0] + 20
             wy = wiz_rect[1] + 15
             
             draw_text_font(hdc, wx, wy, f"WIZARD: STEP {wizard_step}/3", FONT_TITLE, C_YELLOW)
             wy += 35
             
             # Live Signal
             ratio_pct = prompt_white_ratio * 100.0
             ratio_c = C_GREEN if ratio_pct > 0.0 else C_RED
             draw_text_font(hdc, wx, wy, f"Signal: {ratio_pct:.1f}%", FONT_BIG, ratio_c)
             
             # Color Preview Box
             trgb = fov_detect_cfg.get("target_rgb", [255, 255, 255])
             preview_x = wx + 180
             preview_rect = (preview_x, wy + 5, preview_x + 30, wy + 35)
             preview_brush = win32gui.CreateSolidBrush(win32api.RGB(trgb[0], trgb[1], trgb[2]))
             win32gui.FillRect(hdc, preview_rect, preview_brush)
             win32gui.DeleteObject(preview_brush)
             
             # Preview Border
             preview_pen = win32gui.CreatePen(win32con.PS_SOLID, 1, C_WHITE)
             old_preview_pen = win32gui.SelectObject(hdc, preview_pen)
             win32gui.MoveToEx(hdc, preview_rect[0], preview_rect[1])
             win32gui.LineTo(hdc, preview_rect[2], preview_rect[1])
             win32gui.LineTo(hdc, preview_rect[2], preview_rect[3])
             win32gui.LineTo(hdc, preview_rect[0], preview_rect[3])
             win32gui.LineTo(hdc, preview_rect[0], preview_rect[1])
             win32gui.SelectObject(hdc, old_preview_pen)
             win32gui.DeleteObject(preview_pen)
             
             if ratio_pct == 0.0:
                  draw_text_font(hdc, wx + 140, wy + 40, "(Check Settings!)", FONT_SML, C_RED)
             wy += 45

             if wizard_step == 1:
                draw_text_font(hdc, wx, wy, "- Stand on ground (Still)", FONT_MED, C_WHITE)
                wy += 25
                draw_text_font(hdc, wx, wy, "- Ensure NO prompts visible", FONT_SML, C_GREY)
                wy += 35
                draw_text_font(hdc, wx, wy, "Action: Capture NONE", FONT_SML, C_YELLOW)
                wy += 20
                draw_text_font(hdc, wx, wy, f"Press [{b_wizard}] Now", FONT_MED, C_WHITE)
             elif wizard_step == 2:
                draw_text_font(hdc, wx, wy, "- Jump / Depl. Skydive", FONT_MED, C_WHITE)
                wy += 25
                draw_text_font(hdc, wx, wy, "- Wait for 'Skydive' prompt", FONT_SML, C_GREY)
                wy += 35
                draw_text_font(hdc, wx, wy, "Action: Capture SKYDIVE", FONT_SML, C_YELLOW)
                wy += 20
                draw_text_font(hdc, wx, wy, f"Press [{b_wizard}] Now", FONT_MED, C_WHITE)
             elif wizard_step == 3:
                draw_text_font(hdc, wx, wy, "- Deploy Glider", FONT_MED, C_WHITE)
                wy += 25
                draw_text_font(hdc, wx, wy, "- Wait for 'Glide' prompt", FONT_SML, C_GREY)
                wy += 35
                draw_text_font(hdc, wx, wy, "Action: Capture UMBRELLA", FONT_SML, C_YELLOW)
                wy += 20
                draw_text_font(hdc, wx, wy, f"Press [{b_wizard}] Now", FONT_MED, C_WHITE)

        # 5. WATERMARK
        draw_text_font(hdc, BOX_X + BOX_W - 130, BOX_Y + box_h - 20, "Made by MahanYTT", FONT_TINY, C_DARK_GREY)
    except Exception:
        set_diag_error("paint_overlay exception")
        logger.exception("paint_overlay failed")

# ---------------- SETTINGS PAINT ----------------
SET_W, SET_H = 520, 500 

def paint_settings(hwnd, hdc):
    try:
        rect = win32gui.GetClientRect(hwnd)
        brush = win32gui.CreateSolidBrush(win32api.RGB(34, 39, 47))
        win32gui.FillRect(hdc, rect, brush)
        win32gui.DeleteObject(brush)
        win32gui.SetBkMode(hdc, win32con.TRANSPARENT)

        draw_text_font(hdc, 15, 10, "Overlay Settings", FONT_TITLE, C_WHITE)
        draw_text_font(hdc, 15, 40, "Click row, then press keys. Esc to close.", FONT_ITEM, C_GREY)
        b_cfg = bind_to_str(binds[str(HK_CFG)]["mod"], binds[str(HK_CFG)]["vk"])
        draw_text_font(hdc, 15, 68, f"Open this menu: {b_cfg}", FONT_SML, C_YELLOW)

        y = 96
        row_h = 30
        
        sel_id = globals().get('selected_hk_id', None)

        for name, hk_id in ACTION_ORDER:
            b = binds[str(hk_id)]
            txt_bind = bind_to_str(int(b['mod']), int(b['vk']))
            
            if sel_id == hk_id:
                sel_rect = (10, y, SET_W-10, y+row_h)
                brush_sel = win32gui.CreateSolidBrush(win32api.RGB(72, 80, 94))
                win32gui.FillRect(hdc, sel_rect, brush_sel)
                win32gui.DeleteObject(brush_sel)
                draw_text_font(hdc, 20, y+4, f"{name}", FONT_ITEM, C_YELLOW)
                draw_text_font(hdc, 300, y+4, f"[ {txt_bind} ]", FONT_ITEM, C_YELLOW)
            else:
                draw_text_font(hdc, 20, y+4, f"{name}", FONT_ITEM, C_WHITE)
                draw_text_font(hdc, 300, y+4, f"{txt_bind}", FONT_ITEM, C_GREEN)
            
            y += row_h
    except Exception:
        logger.exception("paint_settings failed")

# ---------------- MAIN / PROCS ----------------
overlay_hwnd_global = None
settings_hwnd = None
selector_hwnd = None
selected_hk_id = None
SETTINGS_CLASS = "ModernSettingsWnd"
SELECTOR_CLASS = "PromptRoiSelectorWnd"
selector_dragging = False
selector_start = (0, 0)
selector_end = (0, 0)
selector_mode = "roi"

def register_hotkeys_global(hwnd):
    global status_line
    for i in range(15):
        try:
            user32.UnregisterHotKey(hwnd, i)
        except pywintypes.error:
            pass
    failed = []
    for _, hk in ACTION_ORDER:
        b = binds[str(hk)]
        if not user32.RegisterHotKey(hwnd, hk, int(b["mod"]), int(b["vk"])):
            failed.append(bind_to_str(int(b["mod"]), int(b["vk"])))
    if failed:
        status_line = f"Hotkey conflict: {', '.join(failed)}"
        logger.warning("Hotkey registration failed for: %s", failed)

def draw_outline_rect(hdc, left, top, right, bottom, color, thickness=2):
    if right <= left or bottom <= top:
        return
    brush = win32gui.CreateSolidBrush(color)
    try:
        win32gui.FillRect(hdc, (left, top, right, top + thickness), brush)
        win32gui.FillRect(hdc, (left, bottom - thickness, right, bottom), brush)
        win32gui.FillRect(hdc, (left, top, left + thickness, bottom), brush)
        win32gui.FillRect(hdc, (right - thickness, top, right, bottom), brush)
    finally:
        win32gui.DeleteObject(brush)

def paint_selector(hwnd, hdc):
    try:
        rect = win32gui.GetClientRect(hwnd)
        if diag_enabled:
            now_tick = win32api.GetTickCount()
            if (now_tick - diag_last_log_tick) > 1000:
                diag_last_log_tick = now_tick
                logger.info("DIAG: FPS=%.1f Input/s=%.1f", 1000.0 / diag_timer_delta_ms if diag_timer_delta_ms > 0 else 0, diag_input_count)
                diag_input_count = 0
            
            dy = 0
            draw_text_font(hdc, BOX_X, BOX_Y - 200 + dy, f"Proc: {diag_focus_proc} ({diag_focus_pid}) HWND:{diag_focus_hwnd}", FONT_TINY, C_WHITE); dy += 12
            draw_text_font(hdc, BOX_X, BOX_Y - 200 + dy, f"Scan: {diag_fov_scan_ms:.2f}ms Step:{diag_fov_step} Samples:{diag_fov_samples}", FONT_TINY, C_WHITE); dy += 12
            draw_text_font(hdc, BOX_X, BOX_Y - 200 + dy, f"Skip: {diag_fov_skip}", FONT_TINY, C_WHITE if diag_fov_skip == "" else C_RED); dy += 12
            draw_text_font(hdc, BOX_X, BOX_Y - 200 + dy, f"Ratio: {prompt_white_ratio:.4f} State:{prompt_state}", FONT_TINY, C_WHITE); dy += 12
            draw_text_font(hdc, BOX_X, BOX_Y - 200 + dy, f"Cand: {prompt_candidate} Frames:{prompt_candidate_frames}", FONT_TINY, C_GREY); dy += 12
            draw_text_font(hdc, BOX_X, BOX_Y - 200 + dy, f"Cache: FG={_fg_cache_value} Tick={_fg_cache_tick}", FONT_TINY, C_GREY); dy += 12

        # No Wizard Overlay in Selector

        if selector_mode == "color":
            # In Color Picker: Transparent background (Key=Black) + Opaque instruction box
            key_brush = win32gui.CreateSolidBrush(win32api.RGB(0, 0, 0)) # Clean black
            # To ensure it's transparent, we must fill with key
            win32gui.FillRect(hdc, rect, key_brush) 
            win32gui.DeleteObject(key_brush)

            # Draw a box for instructions so they are readable
            instr_rect = (10, 10, 400, 130)
            bg_brush = win32gui.CreateSolidBrush(C_BOX_BG)
            win32gui.FillRect(hdc, instr_rect, bg_brush)
            win32gui.DeleteObject(bg_brush)

            # Border for fun
            instr_pen = win32gui.CreatePen(win32con.PS_SOLID, 2, C_YELLOW)
            old_pen = win32gui.SelectObject(hdc, instr_pen)
            win32gui.MoveToEx(hdc, instr_rect[0], instr_rect[1])
            win32gui.LineTo(hdc, instr_rect[2], instr_rect[1])
            win32gui.LineTo(hdc, instr_rect[2], instr_rect[3])
            win32gui.LineTo(hdc, instr_rect[0], instr_rect[3])
            win32gui.LineTo(hdc, instr_rect[0], instr_rect[1])
            win32gui.SelectObject(hdc, old_pen)
            win32gui.DeleteObject(instr_pen)

            draw_text_font(hdc, 20, 20, "Pick Prompt Color", FONT_TITLE, C_WHITE)
            draw_text_font(hdc, 20, 50, "Click directly on white text.", FONT_ITEM, C_GREY)
            draw_text_font(hdc, 20, 75, "Esc = cancel", FONT_SML, C_YELLOW)
            rgb = fov_detect_cfg.get("target_rgb", [255, 255, 255])
            draw_text_font(hdc, 20, 100, f"Current: RGB({rgb[0]}, {rgb[1]}, {rgb[2]})", FONT_SML, C_WHITE)

        else:
            # ROI Selector: Dark overlay to dim screen
            dark = win32gui.CreateSolidBrush(win32api.RGB(20, 24, 30))
            win32gui.FillRect(hdc, rect, dark)
            win32gui.DeleteObject(dark)
            win32gui.SetBkMode(hdc, win32con.TRANSPARENT)
            
            draw_text_font(hdc, 20, 18, "Select Prompt Area", FONT_TITLE, C_WHITE)
            draw_text_font(hdc, 20, 46, "Drag a box around the prompt.", FONT_ITEM, C_GREY)
            draw_text_font(hdc, 20, 72, "Enter = save selection, Esc = cancel", FONT_SML, C_YELLOW)

            x1, y1 = selector_start
            x2, y2 = selector_end
            left, right = sorted((x1, x2))
            top, bottom = sorted((y1, y2))
            if right - left >= 2 and bottom - top >= 2:
                draw_outline_rect(hdc, left, top, right, bottom, C_GREEN, thickness=2)
                draw_text_font(
                    hdc,
                    left + 6,
                    max(0, top - 24),
                    f"{right-left}x{bottom-top}",
                    FONT_SML,
                    C_WHITE,
                )
    except Exception:
        logger.exception("paint_selector failed")

def close_selector():
    global selector_hwnd, selector_dragging
    if selector_hwnd and win32gui.IsWindow(selector_hwnd):
        win32gui.DestroyWindow(selector_hwnd)
    selector_hwnd = None
    selector_dragging = False

def begin_prompt_selector(mode):
    global selector_hwnd, selector_dragging, selector_start, selector_end, status_line, selector_mode
    if selector_hwnd and win32gui.IsWindow(selector_hwnd):
        win32gui.SetForegroundWindow(selector_hwnd)
        return
    selector_mode = mode
    selector_dragging = False
    selector_start = (0, 0)
    selector_end = (0, 0)
    status_line = "Selecting prompt color..." if mode == "color" else "Selecting prompt ROI..."
    if settings_hwnd:
        win32gui.ShowWindow(settings_hwnd, win32con.SW_HIDE)
    ex_style = win32con.WS_EX_TOPMOST | win32con.WS_EX_LAYERED | win32con.WS_EX_TOOLWINDOW
    selector_hwnd = win32gui.CreateWindowEx(
        ex_style,
        SELECTOR_CLASS,
        "Select Prompt ROI",
        win32con.WS_POPUP,
        0,
        0,
        SCREEN_W,
        SCREEN_H,
        0,
        0,
        0,
        None,
    )
    if selector_mode == "color":
         # Use Alpha=1 (almost invisible) instead of ColorKey, because ColorKey makes it click-through!
         # 1/255 is effectively invisible but still captures mouse clicks.
         win32gui.SetLayeredWindowAttributes(selector_hwnd, 0, 1, win32con.LWA_ALPHA)
    else:
         # ROI mode: Use alpha blending for dimming
         win32gui.SetLayeredWindowAttributes(selector_hwnd, 0, 165, win32con.LWA_ALPHA)
         
    win32gui.ShowWindow(selector_hwnd, win32con.SW_SHOW)
    win32gui.SetForegroundWindow(selector_hwnd)

def begin_prompt_roi_selector():
    begin_prompt_selector("roi")

def begin_prompt_color_selector():
    begin_prompt_selector("color")

def selector_wndproc(hwnd, msg, wparam, lparam):
    global selector_dragging, selector_start, selector_end, selector_hwnd, status_line
    if msg == win32con.WM_LBUTTONDOWN:
        if selector_mode == "color":
            x, y = lparam_point(lparam)
            # Hide the overlay window momentarily so we capture the true screen color
            # without our own UI interfering
            win32gui.ShowWindow(selector_hwnd, win32con.SW_HIDE)
            # Add a tiny sleep to ensure GDI updates visibility? usually synchronous.
            time.sleep(0.05) 
            rgb = get_screen_pixel_rgb(x, y)
            win32gui.ShowWindow(selector_hwnd, win32con.SW_SHOW)
            
            save_prompt_target_color(rgb)
            close_selector()
            if overlay_hwnd_global:
                win32gui.InvalidateRect(overlay_hwnd_global, None, True)
            # Chain to ROI selection
            begin_prompt_roi_selector()
            status_line = "Color saved. Now select Prompt ROI Area."
            return 0
        selector_start = lparam_point(lparam)
        selector_end = selector_start
        selector_dragging = True
        win32gui.SetCapture(hwnd)
        win32gui.InvalidateRect(hwnd, None, True)
        return 0
    if msg == win32con.WM_MOUSEMOVE and selector_dragging and selector_mode == "roi":
        selector_end = lparam_point(lparam)
        win32gui.InvalidateRect(hwnd, None, True)
        return 0
    if msg == win32con.WM_LBUTTONUP:
        if selector_dragging and selector_mode == "roi":
            selector_end = lparam_point(lparam)
            selector_dragging = False
            win32gui.ReleaseCapture()
            if save_prompt_roi_from_points(selector_start[0], selector_start[1], selector_end[0], selector_end[1]):
                update_fov_mode()
            close_selector()
            if overlay_hwnd_global:
                win32gui.InvalidateRect(overlay_hwnd_global, None, True)
        return 0
    if msg == win32con.WM_KEYDOWN:
        if int(wparam) == win32con.VK_ESCAPE:
            status_line = "Prompt color selection canceled." if selector_mode == "color" else "Prompt ROI selection canceled."
            close_selector()
            if overlay_hwnd_global:
                win32gui.InvalidateRect(overlay_hwnd_global, None, True)
            return 0
        if int(wparam) == win32con.VK_RETURN:
            if selector_mode == "roi":
                if save_prompt_roi_from_points(selector_start[0], selector_start[1], selector_end[0], selector_end[1]):
                    update_fov_mode()
            close_selector()
            if overlay_hwnd_global:
                win32gui.InvalidateRect(overlay_hwnd_global, None, True)
            return 0
    if msg == win32con.WM_PAINT:
        hdc, ps = win32gui.BeginPaint(hwnd)
        try:
            paint_selector(hwnd, hdc)
        finally:
            win32gui.EndPaint(hwnd, ps)
        return 0
    if msg == win32con.WM_CLOSE:
        status_line = "Prompt color selection canceled." if selector_mode == "color" else "Prompt ROI selection canceled."
        close_selector()
        return 0
    if msg == win32con.WM_DESTROY:
        selector_hwnd = None
        selector_dragging = False
        return 0
    return win32gui.DefWindowProc(hwnd, msg, wparam, lparam)

def settings_wndproc(hwnd, msg, wparam, lparam):
    global settings_hwnd, selected_hk_id
    if msg == win32con.WM_LBUTTONDOWN:
        _, y = lparam_point(lparam)
        idx = (y - 96) // 30
        if 0 <= idx < len(ACTION_ORDER):
            selected_hk_id = ACTION_ORDER[idx][1]
            win32gui.InvalidateRect(hwnd, None, True)
        return 0
    if msg in (win32con.WM_KEYDOWN, win32con.WM_SYSKEYDOWN):
        if selected_hk_id is not None:
            vk = int(wparam)
            if vk == win32con.VK_ESCAPE: 
                selected_hk_id = None
                win32gui.InvalidateRect(hwnd, None, True)
                return 0
            if vk not in (16,17,18,91,92): 
                mod = 0
                if win32api.GetKeyState(win32con.VK_CONTROL) < 0: mod |= win32con.MOD_CONTROL
                if win32api.GetKeyState(win32con.VK_MENU) < 0:    mod |= win32con.MOD_ALT
                if win32api.GetKeyState(win32con.VK_SHIFT) < 0:   mod |= win32con.MOD_SHIFT
                binds[str(selected_hk_id)] = {"mod":mod, "vk":vk}
                save_json(BINDS_PATH, binds)
                if overlay_hwnd_global: register_hotkeys_global(overlay_hwnd_global)
                selected_hk_id = None
                win32gui.InvalidateRect(hwnd, None, True)
                if overlay_hwnd_global:
                    win32gui.InvalidateRect(overlay_hwnd_global, None, True)
        elif int(wparam) == win32con.VK_ESCAPE:
            win32gui.ShowWindow(hwnd, win32con.SW_HIDE)
        return 0
    if msg == win32con.WM_PAINT:
        hdc, ps = win32gui.BeginPaint(hwnd)
        try:
            paint_settings(hwnd, hdc)
        finally:
            win32gui.EndPaint(hwnd, ps)
        return 0
    if msg == win32con.WM_CLOSE:
        win32gui.ShowWindow(hwnd, win32con.SW_HIDE); return 0
    return win32gui.DefWindowProc(hwnd, msg, wparam, lparam)

# Wizard State
wizard_active = False
wizard_step = 0
wizard_vals = {}

def advance_calibration_wizard():
    global wizard_active, wizard_step, status_line, wizard_vals, fov_detect_cfg
    global prompt_white_ratio
    
    if not wizard_active:
        wizard_active = True
        wizard_step = 1
        wizard_vals = {}
        status_line = "WIZARD STARTED. Follow On-Screen Instructions."
        logger.info("Wizard started")
        return

    # Capture current ratio
    current_ratio = prompt_white_ratio
    
    if wizard_step == 1:
        wizard_vals["NONE"] = current_ratio
        wizard_step = 2
        status_line = f"WIZARD: Value Captured ({current_ratio:.4f}). Next: SKYDIVE."
        logger.info("Wizard step 1 captured NONE=%s", current_ratio)
    elif wizard_step == 2:
        wizard_vals["SKYDIVE"] = current_ratio
        wizard_step = 3
        status_line = f"WIZARD: Value Captured ({current_ratio:.4f}). Next: UMBRELLA."
        logger.info("Wizard step 2 captured SKYDIVE=%s", current_ratio)
    elif wizard_step == 3:
        wizard_vals["UMBRELLA"] = current_ratio
        logger.info("Wizard step 3 captured UMBRELLA=%s", current_ratio)
        
        # Calculate Logic
        none_val = wizard_vals.get("NONE", 0.0)
        sky_val = wizard_vals.get("SKYDIVE", 0.0)
        umb_val = wizard_vals.get("UMBRELLA", 0.0)
        
        # Validation
        # If signal is 0 for skydive/umbrella, we can't calibrate.
        if sky_val == 0.0 or umb_val == 0.0:
            status_line = "WIZARD ERROR: No signal detected (0.0%). Check ROI/Color."
            wizard_active = False # Close wizard so they can fix it
            return

        if sky_val <= none_val: 
            status_line = f"WIZARD ERROR: Skydive ({sky_val:.3f}) <= None ({none_val:.3f})"
            wizard_active = False
            return
        if umb_val <= sky_val:
            status_line = f"WIZARD ERROR: Umbrella ({umb_val:.3f}) <= Skydive ({sky_val:.3f})"
            wizard_active = False
            return

        new_sky_min = (none_val + sky_val) / 2.0
        new_umb_min = (sky_val + umb_val) / 2.0
        
        # Determine strict minimum difference
        if (sky_val - none_val) < 0.002: 
             new_sky_min = sky_val - 0.001
        if (umb_val - sky_val) < 0.002:
             new_umb_min = umb_val - 0.001

        fov_detect_cfg["ratio_skydive_min"] = new_sky_min
        fov_detect_cfg["ratio_umbrella_min"] = new_umb_min
        
        # Clear specific calibration overrides
        if "ratio_calibration" not in fov_detect_cfg: fov_detect_cfg["ratio_calibration"] = {}
        fov_detect_cfg["ratio_calibration"]["NONE"] = None
        fov_detect_cfg["ratio_calibration"]["SKYDIVE"] = None
        fov_detect_cfg["ratio_calibration"]["UMBRELLA"] = None
        
        save_json(FOV_DETECT_PATH, fov_detect_cfg)
        status_line = f"WIZARD COMPLETE. Saved."
        logger.info("Wizard completed. Saved new thresholds: %s, %s", new_sky_min, new_umb_min)
        wizard_active = False

def overlay_wndproc(hwnd, msg, wparam, lparam):
    global accum_dx, calA_dx, calB_dx, status_line, scale_deg_per_dx_normal, scale_deg_per_dx_freefall, paused_line, selected_hk_id, show_crosshair
    global diag_enabled, diag_input_count, diag_last_input_tick, diag_timer_count, diag_last_timer_tick, diag_timer_delta_ms
    global _last_focus_update_tick, _force_full_clear, wizard_active

    if msg == WM_INPUT:
        dx = get_rawinput_lLastX(lparam)
        if dx is not None:
            diag_input_count += 1
            diag_last_input_tick = win32api.GetTickCount()
            
            # DRIFT FIX: Only accumulate delta if tracking is active AND settings menu is closed.
            settings_open = (settings_hwnd and win32gui.IsWindowVisible(settings_hwnd))
            if tracking_active and not settings_open and not fov_is_transitioning:
                accum_dx += float(dx)
        return 0
    if msg == win32con.WM_HOTKEY:
        hk_id = wparam # Assign wparam to hk_id for consistent use
        if hk_id == HK_EXIT: win32gui.PostQuitMessage(0)
        elif hk_id == HK_CROSS:
            show_crosshair = not show_crosshair
            _force_full_clear = True
            win32gui.InvalidateRect(hwnd, None, True)
        elif hk_id == HK_WIZARD:
            advance_calibration_wizard()
            win32gui.InvalidateRect(hwnd, None, True)
        elif hk_id == HK_CFG: 
            if settings_hwnd: 
                win32gui.ShowWindow(settings_hwnd, win32con.SW_SHOW)
                win32gui.SetForegroundWindow(settings_hwnd)
        elif hk_id == HK_COLOR:
            begin_prompt_color_selector()
        elif hk_id == HK_ROI:
            begin_prompt_roi_selector()
        elif wparam == HK_DIAG:
            diag_enabled = not diag_enabled
            status_line = "Diagnostics ON" if diag_enabled else "Diagnostics OFF"
            _force_full_clear = True
            win32gui.InvalidateRect(hwnd, None, True)
        elif wparam == HK_PNONE:
            calibrate_prompt_ratio(PROMPT_NONE)
        elif wparam == HK_PSKY:
            calibrate_prompt_ratio(PROMPT_SKYDIVE)
        elif wparam == HK_PUMB:
            calibrate_prompt_ratio(PROMPT_UMBRELLA)
        elif wparam == HK_ZERO: set_zero()
        elif wparam == HK_CAL_A: calA_dx = accum_dx; status_line = "Captured A (0)"
        elif wparam == HK_CAL_B: 
            calB_dx = accum_dx
            if calA_dx is not None:
                d = calB_dx - calA_dx
                if abs(d) > 0:
                    new_scale = (CAL_B_DEG - CAL_A_DEG) / d
                    if fov_mode == FOV_MODE_FREEFALL:
                        scale_deg_per_dx_freefall = new_scale
                        status_line = "Calibrated FREEFALL!"
                    else:
                        scale_deg_per_dx_normal = new_scale
                        status_line = "Calibrated NORMAL!"
                    profile_out = {"dpi": DPI, "sens": SENS, "direction": 1}
                    if scale_deg_per_dx_normal is not None:
                        profile_out["scale_deg_per_dx_normal"] = scale_deg_per_dx_normal
                    if scale_deg_per_dx_freefall is not None:
                        profile_out["scale_deg_per_dx_freefall"] = scale_deg_per_dx_freefall
                    # Keep legacy key for backward compatibility with older builds.
                    if "scale_deg_per_dx_normal" in profile_out:
                        profile_out["scale_deg_per_dx"] = profile_out["scale_deg_per_dx_normal"]
                    profiles[KEY] = profile_out
                    save_json(PROFILE_PATH, profiles)
                    set_zero()
        return 0
    if msg == win32con.WM_TIMER:
        now_tick = win32api.GetTickCount()
        if diag_last_timer_tick:
            diag_timer_delta_ms = int(now_tick - diag_last_timer_tick)
        diag_last_timer_tick = now_tick
        diag_timer_count += 1
        try:
            if (now_tick - _last_focus_update_tick) >= FOCUS_UPDATE_MS:
                update_focus_state()
                _last_focus_update_tick = now_tick
            update_fov_mode()
            update_camera_lock()
            maybe_log_diagnostics()
        except Exception as exc:
            set_diag_error(f"timer update exception: {exc}")
            logger.exception("Timer update failed")
        # Invalidate ENTIRE window to ensure ROI frame (which is outside panel) is redrawn
        win32gui.InvalidateRect(hwnd, None, False)
        return 0
    if msg == win32con.WM_PAINT:
        hdc, ps = win32gui.BeginPaint(hwnd)
        try:
            paint_overlay(hwnd, hdc)
        finally:
            win32gui.EndPaint(hwnd, ps)
        return 0
    if msg == win32con.WM_DESTROY: win32gui.PostQuitMessage(0); return 0
    return win32gui.DefWindowProc(hwnd, msg, wparam, lparam)

def main():
    global overlay_hwnd_global, settings_hwnd

    wc = win32gui.WNDCLASS()
    wc.lpfnWndProc = overlay_wndproc
    wc.lpszClassName = "FullscreenOverlay"
    wc.hCursor = win32gui.LoadCursor(0, win32con.IDC_ARROW)
    win32gui.RegisterClass(wc)

    wc2 = win32gui.WNDCLASS()
    wc2.lpfnWndProc = settings_wndproc
    wc2.lpszClassName = SETTINGS_CLASS
    wc2.hCursor = win32gui.LoadCursor(0, win32con.IDC_ARROW)
    win32gui.RegisterClass(wc2)

    wc3 = win32gui.WNDCLASS()
    wc3.lpfnWndProc = selector_wndproc
    wc3.lpszClassName = SELECTOR_CLASS
    wc3.lpszClassName = SELECTOR_CLASS
    wc3.hCursor = win32gui.LoadCursor(0, win32con.IDC_CROSS)
    win32gui.RegisterClass(wc3)
    
    # Removed Blocker Window Class

    init_fonts()
    hwnd = None
    try:
        # WS_EX_NOACTIVATE ensures clicks don't steal focus from Fortnite
        # WS_EX_TRANSPARENT makes it click-through (initial state)
        exStyle = win32con.WS_EX_TOPMOST | win32con.WS_EX_LAYERED | win32con.WS_EX_TOOLWINDOW | \
                  win32con.WS_EX_TRANSPARENT | win32con.WS_EX_NOACTIVATE
        hwnd = win32gui.CreateWindowEx(exStyle, "FullscreenOverlay", "Overlay", win32con.WS_POPUP, 0, 0, SCREEN_W, SCREEN_H, 0,0,0,None)
        overlay_hwnd_global = hwnd
        win32gui.SetLayeredWindowAttributes(hwnd, C_INVISIBLE, 0, win32con.LWA_COLORKEY)
        win32gui.ShowWindow(hwnd, win32con.SW_SHOW)

        settings_hwnd = win32gui.CreateWindowEx(
            win32con.WS_EX_TOPMOST|win32con.WS_EX_TOOLWINDOW,
            SETTINGS_CLASS, "Overlay Settings", win32con.WS_OVERLAPPED|win32con.WS_SYSMENU|win32con.WS_CAPTION,
            100, 100, SET_W, SET_H, 0,0,0,None
        )

        # Create Blocker Window (Invisible start)
        # Removed Blocker Window Creation

        register_raw_mouse(hwnd)
        register_hotkeys_global(hwnd)
        user32.SetTimer(hwnd, TIMER_ID, TIMER_MS, None)
        win32gui.PumpMessages()
    finally:
        unlock_camera() # Ensure cursor is free on exit
        if selector_hwnd and win32gui.IsWindow(selector_hwnd):
            win32gui.DestroyWindow(selector_hwnd)
        if hwnd:
            user32.KillTimer(hwnd, TIMER_ID)
        cleanup_fonts()

if __name__ == "__main__":
    main()
