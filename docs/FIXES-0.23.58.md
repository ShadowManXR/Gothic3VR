# 0.23.58 — separate weapon alignment from tracked wrist orientation

User confirmed the first/third-person performance difference is resolved by
0.23.57. New screenshots compare equipped Skyrim hands with Gothic hands:
the Gothic wrist bends sharply relative to the forearm. User clarified this
appears mainly while holding weapons. Saved hand translation/rotation settings
are all zero; melee pitch is -30 degrees and bow pitch is -34 degrees.

Melee previously rotated the tracked palm both to align the native blade with
the controller and to apply weapon pitch/yaw/roll. That is an extra hand-bone
rotation independent of the actual controller, even at zero hand calibration.
Bow also applied its angle correction to the rendered left hand.

OneHandPose now folds automatic blade alignment and weapon angles into the
constant mesh-to-hand attachment in palm-centred local space. Its hand grip
stays the calibrated controller pose. The established native weapon position,
direction and palm pivot are preserved algebraically. Two-hand acquisition and
mesh prediction use the same calibrated attachment as final skinning. Shared
support and weight still move the hand/weapon assembly rigidly; no weapon
sliding or second wrist filter is introduced. This covers both 1H hands and
standard 2H swords/axes. Staff retains its accepted weapon-only profile.

The bow's rendered left hand no longer receives BowPitch/Yaw/Roll. Its existing
bow/arrow/aim geometry, anchor and configured angles continue through the bow
sample. Palms retain their controller-centred pivot, including at long reach.
No elbow/shoulder compensation is reintroduced. The accepted three-centimetre
forearm lowering and progressive axial twist remain unchanged.

Eleven Release CTests pass: actual Hero landmark palm pivots, legacy melee mesh
pose parity across hand/weapon rotations and large world positions, fixed
attachment under weight and oblique two-hand acquisition, dual wield, staff
support, forearm twist, bow, crossbow stereo reuse, body and palette restoration.
These are offline checks; final visual wrist acceptance needs headset review.
No settings are rewritten. Normal runtime stays clean; no previous experiments,
DLAA or DX11 trial is activated. The 0.23.57 label-performance fix is retained.
