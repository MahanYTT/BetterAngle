#ifndef LOGIC_H
#define LOGIC_H

#include <windows.h>
#include <string>

double FetchFortniteSensitivity();

class AngleLogic {
public:
    AngleLogic(double sensX);
    void Update(int dx);
    double GetAngle() const;
    long long GetAccumDx() const { return m_accumDx; }
    void SetZero();
    void LoadProfile(double sensX);
    void SetDivingState(bool diving);

private:
    double m_sensX;
    bool m_isDiving;
    
    long long m_accumDx;
    long long m_baseDx;
    double m_baseAngle;
    
    double Norm360(double a) const;
};

#endif // LOGIC_H
