#pragma once
#include <cmath>
namespace diagnostic {
// With CharacterSensor present, GetGoalPosition is the PREVIOUS consumed
// sensor result until CalcNextSteps. Do not compare it to this update's wish
// and then rewrite the fresh request: native steering/collisions own that goal.
// Locomotion's native ProcessMovements chooses alignment and gait together.
// A later VR look-at write invalidates that relationship. Only idle head turns
// use the native alignment lease; walking keeps the native author's decision.
inline bool HeadMayAlignNativeBody(bool controllerMoving,float wishedX,float wishedZ) {
    return !controllerMoving&&std::isfinite(wishedX)&&std::isfinite(wishedZ)&&std::hypot(wishedX,wishedZ)<=.05f;
}
}
