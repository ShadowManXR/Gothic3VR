#pragma once
namespace diagnostic
{
struct StereoCameraState
{
    bool gameRunning=false,modeAvailable=false,firstPerson=false,tempFirstPerson=false,dialog=false,animal=false,activity=false;
};
enum class StereoCameraMode { OutsideGame, Unavailable, FirstPerson, Temporary, Dialog, NativeOther };
inline bool ComfortFirstPerson(StereoCameraState const& state,bool keepFirstPerson,bool calibrated,bool switching) {
    if(!state.gameRunning||!state.modeAvailable||state.dialog||state.animal||state.activity)return false;
    // A temporary native mode or a late SetMode must not expose a single
    // third-person transition frame once the player eye height is calibrated.
    // Initial load still waits for the camera AI to supply a real eye height.
    return (keepFirstPerson&&calibrated)||((state.firstPerson||state.tempFirstPerson)&&(!switching||calibrated));
}
inline StereoCameraMode CameraMode(StereoCameraState const& state)
{
    if(!state.gameRunning)return StereoCameraMode::OutsideGame;
    if(!state.modeAvailable)return StereoCameraMode::Unavailable;
    if(state.dialog)return StereoCameraMode::Dialog;
    if(state.activity)return StereoCameraMode::NativeOther;
    if(state.tempFirstPerson)return StereoCameraMode::Temporary;
    return state.firstPerson?StereoCameraMode::FirstPerson:StereoCameraMode::NativeOther;
}
inline char const* CameraModeName(StereoCameraMode mode)
{
    switch(mode)
    {
    case StereoCameraMode::OutsideGame:return "outside_game";
    case StereoCameraMode::Unavailable:return "mode_unavailable";
    case StereoCameraMode::FirstPerson:return "first_person";
    case StereoCameraMode::Temporary:return "temporary_first_person";
    case StereoCameraMode::Dialog:return "dialog";
    default:return "native_other";
    }
}
inline bool StereoCameraAllowed(StereoCameraState const& state,bool connected)
{
    // First-person gates initial activation only. Once connected, the native
    // renderer's current perspective camera remains the source during hits,
    // dialogue, interactions and other mode transitions. Pose/camera validity
    // and shutdown checks are enforced separately by the render hook.
    return connected||CameraMode(state)==StereoCameraMode::FirstPerson;
}
}
