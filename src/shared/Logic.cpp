#include "shared/Logic.h"
#include <cmath>
#include "shared/State.h"

bool IsFortniteFocused() {
    HWND hForeground = GetForegroundWindow();
    if (hForeground == NULL) return false;

    wchar_t windowTitle[256];
    GetWindowTextW(hForeground, windowTitle, 256);

    // Fortnite's window title is typically "Fortnite  " or contains "Fortnite"
    std::wstring title(windowTitle);
    if (title.find(L"Fortnite") != std::wstring::npos || title.find(L"Epic Games") != std::wstring::npos || title.find(L"Battle Royale") != std::wstring::npos) {
        return true;
    }

    // Class Name Check
    wchar_t className[256];
    GetClassNameW(hForeground, className, 256);
    std::wstring cls(className);
    if (cls == L"UnrealWindow") {
        return true;
    }

    return false;
}

AngleLogic::AngleLogic(double dpi, double sens)
    : m_dpi(dpi), m_sens(sens), m_accumDx(0), m_baseDx(0), m_baseAngle(0.0), m_scalePerDx(0.0) {}

void AngleLogic::Update(int dx) {
    if (!g_debugMode && !IsFortniteFocused()) return;
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    if (m_scalePerDx == 0.0) return 0.0;
    double angle = Norm360(m_baseAngle + (m_accumDx - m_baseDx) * m_scalePerDx);
    
    // Heuristic Check (v4.9.15)
    g_currentAngle = (float)angle;
    if (g_currentSelection == NONE) {
        if (g_currentAngle > 50.0f || g_currentAngle < -50.0f) {
            g_isDiving = true;
        } else {
            g_isDiving = false;
        }
    }
    
    return angle;
}

void AngleLogic::SetZero() {
    m_baseAngle = 0.0;
    m_baseDx = m_accumDx;
}

void AngleLogic::SetScale(double scale) {
    m_scalePerDx = scale;
}

double AngleLogic::Norm360(double a) const {
    double res = fmod(a, 360.0);
    if (res < 0) res += 360.0;
    return res;
}
