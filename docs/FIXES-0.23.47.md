# 0.23.47 — off-axis backward input and wrist flexion

User confirmed 0.23.46 rotation and forward/back walking, but lateral walking
still moved backwards, without rotating the view. Latest11760 logs confirm:
left-stick .84,-.13 became filtered axes .99,-.15 and a full Backward+Right
native wish. Likewise -.95,-.30 produced Backward+Left. The old35/25degree
hysteresis gated only lateral input during forward walking; longitudinal input
still used a small .1threshold even during dominant strafe.

NativeStickDirections now gates BOTH secondary axes with the same35degree
entry/25degree exit hysteresis. Lateral movement ignores small forward/back
drift, genuine diagonals still work, and direction sign changes must deliberately
cross the entry threshold. Native action transport, direction construction,
sensor goals, floor, movement/physics speed and accepted instant turn are intact.
Menus/lockpick/radial keep their separate input routes.

ArmTwist already distributed longitudinal roll, but it did not distribute wrist
flexion/deviation. NaturalWrist only lowered the elbow and capped its displacement
at10cm, preventing the forearm from following a dropped hand when the elbow
needed to rise. It also ran only during weapon/contact states.

NaturalWrist now leaves the first15degrees at the wrist and progressively shares
larger bends with the elbow/forearm, targeting a soft35degree wrist limit. It
works in both vertical directions and with empty hands. Exact tracked wrist,
palm, weapon and support contacts stay fixed. Visual upper-arm retargeting and
modest lower-bone length adjustment keep the chain connected. At the retargeting
limits, a continuous feasible-angle search replaces discrete half-step fallback
to avoid popping. Axial roll distribution and frozen per-eye palettes remain.

Validation covers the two captured stick examples through the actual translator
and native-action transport, both sign axes, hysteresis, deliberate diagonals,
neutral release and sprint/menu transitions. Wrist sweeps0..120degrees, both
hands/bend directions and several world yaws check continuous elbow changes,
bounded remaining flexion, exact hands, retargeted endpoints and progressive
twist. Existing palm/weapon binding, two-hand support, crossbow stereo and camera
regressions are retained. Offline checks are not headset acceptance.

No launcher or calibration changes; no DX11/performance experiment enabled.
DLAA remains off. Source and DX11 trial payload/version follow the new plugin.
