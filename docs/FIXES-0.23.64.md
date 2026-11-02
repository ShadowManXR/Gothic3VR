# 0.23.64 — sprint facing and recovery camera return

Forward controller Sprint uses a short native AlignmentLookAt lease spanning
native input consumption through native physics. It is limited to a fresh
controller-owned Forward direction and an already authorized native Fast wish
(5). Backward/side-only input, recovery, activities, animal forms, mounts and
native combat-rotation/target ownership remain native. No movement mode,
velocity, position, sensor goal or actor transform is synthesized. Native
alignment writes retain priority and the lease restores only its own value.

When a third-person activity/knockdown finishes, release the three native FP
latches, select the normal preset, rebind the current upright actor and then
restore FP/recenter. Both native virtual slots are verified against the installed
Game.dll profile. Transition-only logs include actor/camera forward and look-back
state to diagnose any remaining orientation difference without a frame profiler.

Investigation corrections: native ProcessMovements' vtable+16C branch is
IsLookingBack, not IsInFirstPerson. Its cached matrix+6C is left untouched.
SetTarget's session-state11 guard means Editor, not first-person camera mode.
No cache patch or speculative state-guard bypass from those hypotheses ships.

Relevant movement, facing-lease, sprint, recovery, native-binary and logical
control regressions pass. Headset confirmation of these two fixes is pending.
The working Meta wait-overlap host, graphics, runtime choice, user calibration,
launcher and native renderer files are unchanged. No timing request is rearmed.
