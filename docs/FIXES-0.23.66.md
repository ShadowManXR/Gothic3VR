# 0.23.66 — observe the unresolved diagonal Sprint reversal

The user reproduced backward motion with forward-left/right Sprint in both
0.23.64 and 0.23.65. The attempted Sprint alignment override is removed. This
version does **not** claim to fix the reversal.

A one-shot `logs/native-movement.request` file is consumed at startup. Only
that run observes native controller movement for 120 seconds from gameplay,
at most 480 selected updates (4 Hz). There is no gameplay file polling,
renderer capture, extra hook, general profiler, or movement-state write.
Subsequent launches are unarmed unless another request is explicitly created.

Each selected hero update records stages before/after native ProcessMovements
and before/after native character physics, when those phases occur in the
same update. Records include stick X/Z, digital direction mask, native timers,
Sprint selection/press, native wished direction/mode, reference frame, actual
world position/velocity, actor Z, native alignment, camera matrix and look-back
state. NPCs, other threads, session teardown and unreadable objects are ignored.

CP Game.dll is verified by the existing movement-hook SHA256 guard. The native
SetGoalPose at RVA 0xe6a50 copies its incoming request to +0x2dc before sending
it to CharacterSensor. The observer reads that request separately from the
active target at +0x360. GetLatestGoalPosition at RVA 0xe7090 exposes +0x2c0,
which can still be a previous consumed sensor result. It must not be used as
evidence that this update's request needs rewriting. No target is rewritten.

`tools/analyze_native_movement.py` correlates the phase pairs and highlights
opposite directions without asserting causality from actor orientation alone.
The existing native camera reset on activity/recovery return remains unchanged.
Meta64 compositor, its successful frame-wait overlap fix, launcher, all user
calibration, progression, rendering settings and distances are untouched.
SMAA is still pending, with upstream sources only; it is not enabled here.

Validation: plugin Release build and eight relevant movement/trace/native
binary/recovery regression tests passed. Headset reproduction is still needed.

Headset capture 2026-10-25: diagnostic-20261025-211159-24284.jsonl contains347
complete four-phase sample sets, including53 diagonal Sprint samples. Digital
native timers agree with input masks; native wished direction and fresh goal
agree with the scoped camera direction. The hero position/velocity is zero or
unchanged across the physics callback itself: world transfer happens later,
so this is not evidence of correct physical movement. Across15 selected
intervals with matching diagonal/Sprint endpoints and under15deg heading
change, actual displacement never points backward (minimum dot0.7783).
This supports the user's much improved trial, but does not establish that an
intermittent bug is permanently fixed. No additional movement changes installed.
