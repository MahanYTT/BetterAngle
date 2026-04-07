import cv2
import pytesseract
import numpy as np

# Set tesseract path if needed:
pytesseract.pytesseract.tesseract_cmd = r"C:\Program Files\Tesseract-OCR\tesseract.exe"

BACKENDS = [
    ("DSHOW", cv2.CAP_DSHOW),
    ("MSMF",  cv2.CAP_MSMF),
]

def ocr_digits(img_bgr):
    gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)
    gray = cv2.resize(gray, None, fx=2.5, fy=2.5, interpolation=cv2.INTER_CUBIC)
    gray = cv2.GaussianBlur(gray, (3, 3), 0)
    _, th = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    config = r"--oem 3 --psm 7 -c tessedit_char_whitelist=0123456789"
    text = pytesseract.image_to_string(th, config=config).strip()
    digits = "".join(ch for ch in text if ch.isdigit())

    if not digits:
        th_inv = cv2.bitwise_not(th)
        text2 = pytesseract.image_to_string(th_inv, config=config).strip()
        digits = "".join(ch for ch in text2 if ch.isdigit())

    return digits, th

def open_cam(index):
    for name, backend in BACKENDS:
        cap = cv2.VideoCapture(index, backend)
        if cap.isOpened():
            # try to reduce latency
            try:
                cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            except Exception:
                pass
            return cap, name
        cap.release()
    return None, None

def main():
    print("Starting camera debug. If a window never appears, OpenCV can't open any camera.", flush=True)

    idx = 0
    cap = None
    backend_name = None
    roi = None  # (x,y,w,h)

    while True:
        if cap is None:
            cap, backend_name = open_cam(idx)
            if cap is None:
                print(f"Index {idx}: failed (DSHOW+MSMF). Press N to try next, Q to quit.", flush=True)
                # Show a tiny dummy window so you KNOW it's running
                dummy = np.zeros((180, 640, 3), dtype=np.uint8)
                cv2.putText(dummy, f"No camera at index {idx}. Press N / Q.",
                            (10, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255,255,255), 2)
                cv2.imshow("OBS Camera Debug", dummy)
                k = cv2.waitKey(0) & 0xFF
                if k in (ord('q'), ord('Q')):
                    break
                if k in (ord('n'), ord('N')):
                    idx += 1
                    continue
                continue
            else:
                print(f"Index {idx}: opened with {backend_name}", flush=True)

        ret, frame = cap.read()
        if not ret or frame is None:
            # Sometimes camera needs a moment
            cv2.waitKey(10)
            continue

        show = frame.copy()
        h, w = show.shape[:2]

        if roi is not None:
            x, y, rw, rh = roi
            cv2.rectangle(show, (x, y), (x+rw, y+rh), (0,255,0), 2)
            crop = frame[y:y+rh, x:x+rw]
            digits, _ = ocr_digits(crop)
            cv2.putText(show, f"OCR: {digits}", (10, 40),
                        cv2.FONT_HERSHEY_SIMPLEX, 1.1, (0,255,0), 3)

        cv2.putText(show, f"Index {idx} ({backend_name})  |  N=next  S=select ROI  Q=quit",
                    (10, h-20), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255,255,255), 2)

        cv2.imshow("OBS Camera Debug", show)
        k = cv2.waitKey(1) & 0xFF

        if k in (ord('q'), ord('Q')):
            break

        if k in (ord('n'), ord('N')):
            cap.release()
            cap = None
            roi = None
            idx += 1
            continue

        if k in (ord('s'), ord('S')):
            r = cv2.selectROI("OBS Camera Debug", frame, fromCenter=False, showCrosshair=True)
            x, y, rw, rh = map(int, r)
            if rw > 0 and rh > 0:
                roi = (x, y, rw, rh)
                nx, ny, nw, nh = x/w, y/h, rw/w, rh/h
                print("\n=== Use these in overlay script ===", flush=True)
                print(f"CAM_INDEX = {idx}", flush=True)
                print(f"ROI_NX = {nx:.6f}", flush=True)
                print(f"ROI_NY = {ny:.6f}", flush=True)
                print(f"ROI_NW = {nw:.6f}", flush=True)
                print(f"ROI_NH = {nh:.6f}", flush=True)
                print("==================================\n", flush=True)

    try:
        if cap:
            cap.release()
    except Exception:
        pass
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
