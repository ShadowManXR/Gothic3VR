#pragma once
#include <cmath>
namespace diagnostic {
// Native OnPlayerWalk selects Walk if Shift is first pressed while standing or
// braking. Start in Run and press the same native action only once moving.
inline bool NativeSprintPress(bool requested,bool moving,bool sneak,bool standing,bool braking){
    return requested&&moving&&!sneak&&!standing&&!braking;
}
}
