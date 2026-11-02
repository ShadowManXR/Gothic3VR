# 0.23.46 — native movement ownership and scoped instant camera rotation

The 0.23.44 opposite-goal reconciliation is removed. REA/Ghidra and the native
instructions establish that, with CharacterSensor present, SetGoalPose forwards
the fresh request to sensor+44 and returns before writing movement+2c0.
GetGoalPosition (and GetLatestGoalPosition) reads that older consumed field.
CalcNextSteps publishes the sensor result later. Comparing it to this update's
wish cannot justify overwriting the fresh request. The plugin no longer performs
that rewrite. Native sensor smoothing, obstacle handling, floor and physics own
the movement goal. Camera-relative native input, neutral release and idle-only
head alignment with 30-degree free look are retained.

CameraAI GetCurrentWishedRotation still interpolated its target quaternion after
the mouse filter bypass. The verified native block multiplies frame time, a
camera rate and a ramp, then clamps its Slerp weight to [0,1]. Within the existing
instant-controller first-person VR evaluation only, a scoped lease sets zero
ramp duration and rate 2/dt. Native code then uses weight1. Native matrix building,
mouse evaluation, return-buffer ABI, once-per-update turn and same-update replay
remain in place. The original elapsed/ramp/rate bytes are restored in __finally,
including exceptional exit. Invalid dt/state or an unrecognized memory profile
preserves the original interpolation. Dialog/activity/recovery/animal/finisher
and camera-transition contexts do not select the instant-controller scope.

This is not an additional smoothing filter. It does not alter animation rates,
translation speed, collision/terrain or graphics. The Instant controller turn
setting remains the opt-out. Full Game.dll identity and actual native instruction
operands gate the interpolation lease. Startup ready marker carries code1 when
the scoped Slerp bypass is available.

Validation: the new test executes the actual copied native interpolation
arithmetic and goal getter, with local fixture time/application imports and no
game DLL loading. It checks a partial baseline, immediate alpha1 at30..240Hz,
repeat evaluations, exact state restoration, invalid inputs and SEH restoration.
The getter case demonstrates its consumed-field contract, not game physics.
Existing movement, direction release, camera ABI, sensitivity/sign, native profile,
activity and sneak regression tests also apply. Headset acceptance remains pending;
no FPS gain or resolution of all historical movement bugs is claimed.

DX11 experiment payload/version must follow the new plugin hash, while native
draws remain preserved. Normal launches do not enable that experiment. DLAA stays
off and user calibration/settings stay unchanged.
