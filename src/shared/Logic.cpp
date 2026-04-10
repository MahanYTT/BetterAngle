#include "shared/Logic.h"
#include <cmath>
#include "shared/State.h"

bool IsFortniteFocused() {
    if (g_forceDetection) return true; // Safety fallback

    HWND hForeground = GetForegroundWindow();
    if (!hForeground) return false;

    wchar_t className[256];
    GetClassNameW(hForeground, className, 256);
    std::wstring cls(className);

    return (cls == L"UnrealWindow");
}

AngleLogic::AngleLogic(double sensX)
    : m_sensX(sensX), m_isDiving(false), m_accumDx(0), m_baseDx(0), m_baseAngle(0.0) {}

void AngleLogic::Update(int dx) {
    if (!g_debugMode) {
        if (!g_fortniteFocusedCache || g_isCursorVisible) return;
    }
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    double normalScale = 0.05555 * m_sensX;
    double scale = m_isDiving ? (normalScale * 1.0916) : normalScale;
    
    if (scale == 0.0) scale = 0.0031415; // Bullet-proof fallback
    double angle = Norm360(m_baseAngle + (m_accumDx - m_baseDx) * scale);
    
    g_currentAngle = (float)angle;
    
    return angle;
}

void AngleLogic::SetZero() {
    m_baseAngle = 0.0;
    m_baseDx = m_accumDx;
}

void AngleLogic::LoadProfile(double sensX) {
    if (m_sensX == sensX) return;
    
    // Prevent the angle jump when swapping Sensitivity mid-air
    m_baseAngle = GetAngle();
    m_baseDx = m_accumDx;
    
    m_sensX = sensX;
}

void AngleLogic::SetDivingState(bool diving) {
    if (m_isDiving == diving) return;
    
    // Keep angle stationary during state transition
    m_baseAngle = GetAngle();
    m_baseDx = m_accumDx;
    
    m_isDiving = diving;
}

double AngleLogic::Norm360(double a) const {
    double res = fmod(a, 360.0);
    if (res < 0) res += 360.0;
    return res;
}
