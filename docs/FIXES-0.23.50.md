# 0.23.50 - revert wrist/shoulder compensation; small lower forearm offset

User rejected the new wrist-driven elbow/shoulder behavior and requested the
original arm behavior before today's changes, with only the forearm slightly
lowered. Restore ArmTwist.h byte-for-byte from the backup of its original
progressive axial roll implementation (before the compensated-shoulder change).
Remove the wrist-orientation-driven NaturalWrist elbow solve and the rotating/
translating shoulder compensation. Upper-arm root remains the original IK root.

Replace that compensation with one static visual elbow offset: lower it by
3 cm in world vertical after the existing tracked IK solve. Cap the drop at10%
of forearm length on unusually short rigs. Both adjoining bone transforms still
meet at the same elbow; wrist/palm targets are unchanged. No response to wrist
pitch/deviation, shoulder relocation or new dynamic segment-length search.
Existing axial roll distribution, exact controller reach, calibration, rigid
weapon/support contacts, stereo palette and hand effect deltas remain.

Retain accepted locomotion/strafe/rotation, camera and the0.23.49stowed weapon/
shield visibility correction. No settings, launcher, graphics or native physics
changes. DLAA remains off; DX11 trial payload/version follows the new plugin
without enabling the experiment.

Tests retain original progressive twist coverage and verify static lowering
for mirrored arms, multiple scales/yaws and wrist tilts, unchanged shoulder and
tracked wrist, connected upper/lower endpoints after twist and a bounded offset
for very short arms. Headset acceptance is pending.
