#include "TrackingMath.h"
#include <cstdio>
#include <limits>

int __cdecl main()
{
    unsigned failures = 0;
    auto check = [&](bool condition, char const* name) { std::printf("%s %s\n", condition ? "PASS" : "FAIL", name); if (!condition) ++failures; };
    float m[12] = {1,0,0,1.5f, 0,1,0,1.7f, 0,0,1,-2};
    float q[4]{};
    auto approximately = [](float a, float b) { return std::fabs(a-b) < .0001f; };
    check(diagnostic::TrackingQuaternion(m,q) && approximately(q[0],1) && approximately(q[1],0) && approximately(q[2],0) && approximately(q[3],0),
          "identity rotation with translated HMD");
    float y90[12] = {0,0,1,0, 0,1,0,0, -1,0,0,0};
    check(diagnostic::TrackingQuaternion(y90,q) && approximately(q[0],.70710678f) && approximately(q[2],.70710678f), "90 degree yaw wxyz");
    float x180[12] = {1,0,0,0, 0,-1,0,0, 0,0,-1,0};
    check(diagnostic::TrackingQuaternion(x180,q) && approximately(q[0],0) && approximately(q[1],1), "180 degree pitch branch");
    float y180[12] = {-1,0,0,0, 0,1,0,0, 0,0,-1,0};
    check(diagnostic::TrackingQuaternion(y180,q) && approximately(q[0],0) && approximately(q[2],1), "180 degree yaw branch");
    float z180[12] = {-1,0,0,0, 0,-1,0,0, 0,0,1,0};
    check(diagnostic::TrackingQuaternion(z180,q) && approximately(q[0],0) && approximately(q[3],1), "180 degree roll branch");
    m[0] = 2; check(!diagnostic::TrackingQuaternion(m,q), "scaled pose matrix rejected");
    m[0] = -1; check(!diagnostic::TrackingQuaternion(m,q), "reflected pose matrix rejected");
    m[0] = 1; m[3] = std::numeric_limits<float>::quiet_NaN();
    check(!diagnostic::TrackingQuaternion(m,q), "NaN translation rejected");
    float p[3] = {1,2,3}, rotation[4] = {0,0,0,1};
    check(diagnostic::TrackingFromXyzw(p,rotation,m,q) && approximately(q[0],1) && approximately(m[3],1) && approximately(m[7],2) && approximately(m[11],3), "OpenXR identity xyzw and translation");
    rotation[1]=.70710678f; rotation[3]=.70710678f;
    check(diagnostic::TrackingFromXyzw(p,rotation,m,q) && approximately(m[2],1) && approximately(m[8],-1) && approximately(q[2],.70710678f), "OpenXR yaw xyzw becomes row-major and wxyz");
    rotation[1]=std::numeric_limits<float>::quiet_NaN();
    check(!diagnostic::TrackingFromXyzw(p,rotation,m,q), "invalid OpenXR quaternion rejected");
    return failures ? 1 : 0;
}
