#pragma once
#include <cmath>

namespace diagnostic
{
// OpenVR row-major 3x4 device-to-standing transform. Quaternion order w,x,y,z.
inline bool TrackingQuaternion(float const (&m)[12], float (&q)[4])
{
    for (float value : m) if (!std::isfinite(value)) return false;
    for (int a = 0; a < 3; ++a)
        for (int b = a; b < 3; ++b)
        {
            float dot = 0;
            for (int row = 0; row < 3; ++row) dot += m[4*row+a]*m[4*row+b];
            if (std::fabs(dot - (a == b ? 1.0f : 0.0f)) > 0.03f) return false;
        }
    float determinant = m[0]*(m[5]*m[10]-m[6]*m[9]) - m[1]*(m[4]*m[10]-m[6]*m[8]) + m[2]*(m[4]*m[9]-m[5]*m[8]);
    if (std::fabs(determinant - 1) > 0.03f) return false;
    float trace = m[0] + m[5] + m[10];
    if (trace > 0)
    {
        float s = std::sqrt(trace+1)*2;
        q[0] = s/4; q[1] = (m[9]-m[6])/s; q[2] = (m[2]-m[8])/s; q[3] = (m[4]-m[1])/s;
    }
    else if (m[0] > m[5] && m[0] > m[10])
    {
        float s = std::sqrt(1+m[0]-m[5]-m[10])*2;
        q[0] = (m[9]-m[6])/s; q[1] = s/4; q[2] = (m[1]+m[4])/s; q[3] = (m[2]+m[8])/s;
    }
    else if (m[5] > m[10])
    {
        float s = std::sqrt(1+m[5]-m[0]-m[10])*2;
        q[0] = (m[2]-m[8])/s; q[1] = (m[1]+m[4])/s; q[2] = s/4; q[3] = (m[6]+m[9])/s;
    }
    else
    {
        float s = std::sqrt(1+m[10]-m[0]-m[5])*2;
        q[0] = (m[4]-m[1])/s; q[1] = (m[2]+m[8])/s; q[2] = (m[6]+m[9])/s; q[3] = s/4;
    }
    float norm = 0;
    for (float value : q) { if (!std::isfinite(value)) return false; norm += value*value; }
    if (norm < 0.001f) return false;
    norm = std::sqrt(norm);
    for (float& value : q) value /= norm;
    return true;
}
// OpenXR stores orientation as x,y,z,w. Produce the same row-major transform
// and normalized w,x,y,z diagnostic representation used by the OpenVR probe.
inline bool TrackingFromXyzw(float const (&position)[3], float const (&rotation)[4],
                             float (&matrix)[12], float (&quaternion)[4])
{
    float length = 0;
    for (float value : rotation) length += value*value;
    length = std::sqrt(length);
    if (!std::isfinite(length) || length < .001f) return false;
    float x=rotation[0]/length, y=rotation[1]/length, z=rotation[2]/length, w=rotation[3]/length;
    float converted[12] = {1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w,position[0],
                          2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w,position[1],
                          2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y,position[2]};
    for (unsigned i=0;i<12;++i) matrix[i] = converted[i];
    return TrackingQuaternion(matrix,quaternion);
}
}
