#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic::arms {
// Small static visual adjustment only. Keep original tracking/axial twist and
// the fixed shoulder; do not derive elbow/shoulder positions from wrist tilt.
// Both adjoining bone deltas meet at the lowered elbow; tracked palm is exact.
inline bool LowerTrackedForearm(float units,Solution& solution){
    if(!std::isfinite(units)||units<=0||!Finite(solution.elbow)||!Finite(solution.wrist))return false;
    float length=Length(solution.wrist-solution.elbow);
    if(!std::isfinite(length)||length<.001f)return false;
    float drop=std::min(.03f*units,.1f*length);
    if(!std::isfinite(drop)||drop<=0)return false;
    solution.elbow.y-=drop;return true;
}
}
