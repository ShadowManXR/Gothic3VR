#pragma once
#include "VrSettings.h"
namespace diagnostic {
// Creator-approved behavior, no longer exposed as launcher switches. Keep
// all user calibration/graphics/runtime values independent. Controller input is
// enabled as part of the VR experience, regardless of the selected runtime.
inline void ApplyLauncherFixedFeatures(VrSettings& s) {
    s.autoStart=s.keepFirstPerson=s.hideHead=s.roomScaleWalking=true;
    s.stableHorizon=s.smoothCamera=s.trackedArms=s.stableBody=true;
    s.physicalCrouching=true;
    s.controllers=true;
    s.instantControllerTurn=true;
}
}
