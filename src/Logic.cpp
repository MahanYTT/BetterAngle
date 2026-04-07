#include "Logic.h"
#include <cmath>

AngleLogic::AngleLogic(double dpi, double sens)
    : m_dpi(dpi), m_sens(sens), m_accumDx(0), m_baseDx(0), m_baseAngle(0.0), m_scalePerDx(0.0) {}

void AngleLogic::Update(int dx) {
    m_accumDx += dx;
}

double AngleLogic::GetAngle() const {
    if (m_scalePerDx == 0.0) return 0.0;
    return Norm360(m_baseAngle + (m_accumDx - m_baseDx) * m_scalePerDx);
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
