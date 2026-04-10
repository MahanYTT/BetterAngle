#include "shared/Logic.h"
#include <cmath>
#include "shared/State.h"

bool IsFortniteFocused() {
    HWND hForeground = GetForegroundWindow();
    if (!hForeground) return false;

    wchar_t className[256];
    GetClassNameW(hForeground, className, 256);
    std::wstring cls(className);

    // Fortnite uses "UnrealWindow" or "Fortnite"
    if (cls == L"UnrealWindow" || cls == L"Fortnite") {
        return true;
    }

    wchar_t title[256];
    GetWindowTextW(hForeground, title, 256);
    std::wstring wTitle(title);

    return (wTitle.find(L"Fortnite") != std::wstring::npos);
}

AngleLogic::AngleLogic(double dpi, double sens)
    : m_dpi(dpi), m_sens(sens), m_accumDx(0), m_baseDx(0), m_baseAngle(0.0), m_scalePerDx(0.0) {}

void AngleLogic::Update(int dx) {
    if (!g_debugMode) {
        if (!IsFortniteFocused() || g_isCursorVisible) return;
    }
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
