#ifndef LOGIC_H
#define LOGIC_H

#include <windows.h>

class AngleLogic {
public:
    AngleLogic(double dpi, double sens);
    void Update(int dx);
    double GetAngle() const;
    void SetZero();
    void SetScale(double scale);

private:
    double m_dpi;
    double m_sens;
    long long m_accumDx;
    long long m_baseDx;
    double m_baseAngle;
    double m_scalePerDx; // Calibrated scale
    
    double Norm360(double a) const;
};

#endif // LOGIC_H
