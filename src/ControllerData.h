#pragma once
#include <cstdint>
namespace diagnostic {
struct ControllerPose {bool valid=false;float position[3]{},quaternion[4]{0,0,0,1};bool tracked=false;};
struct ControllerHand {
    bool active=false;ControllerPose grip,aim,aimInView;
    float stick[2]{},trigger=0,squeeze=0;
    bool primary=false,secondary=false,stickClick=false,menu=false;
};
struct ControllerFrame {bool focused=false;uint64_t timeMs=0;ControllerHand hands[2];uint32_t availableButtons=0,bindingHeld=0,bindingPressed=0,bindingChord=0;bool bindingsMapped=false;};
inline ControllerFrame ControllersDuringRenderWait(ControllerFrame const& sample,bool focused,uint64_t now) {
    // Preserve actual focus separately from sample age. Consumers still reject
    // inputs older than 100 ms; expiration must not fabricate a focus loss.
    return focused&&sample.focused&&now>=sample.timeMs?sample:ControllerFrame{};
}
}
