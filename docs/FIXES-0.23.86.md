# 0.23.86 — Grounded sneak posture and pickup legs

Stable body previously rebased every leg through a neutral standing pelvis.
That lifted native bent knees and feet during sneak and ground-item collection.

## Sneak

While the actual native `AniState_Sneak` is active, lower-body bones retain
their native crouch geometry and grounded height. They follow the existing
upright body heading and shared ground-origin correction rather than a
standing-pelvis rotation. The neutral torso and shoulder roots are lowered by
the corresponding native pelvis displacement. Tracked wrists remain exact.

A separate optical stance offset lowers the shared camera/controller origin.
It samples native pelvis-to-bind height during the first second of crouching,
then holds a stable depth instead of transmitting walking pelvis bob to the
headset. Entry and exit use a 120 ms time constant. Native root motion, terrain
height, room-scale movement and raw headset/controller tracking are preserved.
The standing eye-height calibration removes the sampled stance offset if a
save is loaded directly into sneak, avoiding a double height correction.

## Pickup

The exact native Take action, `PS_Normal_Take` state and `_AI_TakeItem` task
select a visual lower-body override. Standing pickup retains a neutral standing
leg pose; pickup from sneak retains the preceding native model-space crouch
pose. A 200 ms tail covers the native animation blend. Cached crouch bones
follow the current world origin, terrain height and body heading.

The native item-transfer task is not skipped, replaced or accelerated. Skills,
inventory, ownership, perception and controller interaction remain native.
Mining, lockpicking, campfires, beds, knockdowns and finishing attacks are not
classified as ground-item pickup by this policy.

## Rendering and lifecycle

The rig is validated before sampling posture. The existing once-per-frame
frozen palette supplies both eyes, and native matrices are restored afterwards.
No gameplay entity transform, velocity, collider or animation state is written.
World release, VR stop, rig changes and invalid contexts discard posture state.
The feature follows the existing tracked arms, stable body and stable horizon
options; no new launcher option or production diagnostic probe is added.

Tests cover native crouch feet, lowered torso, original upright deltas,
standing/crouched pickup, moving ground, exact two-eye restoration, smooth
stance transitions without walking bob, load and slope height calibration,
tracked hands, native palette hook ABI and prior camera return behavior.
Headset validation remains pending.
