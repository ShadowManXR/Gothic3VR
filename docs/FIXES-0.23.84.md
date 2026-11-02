# 0.23.84 — Conjured sword palm contact

The retained 0.23.83 session recorded the conjured sword's initial native
mesh-to-wrist offset at 68.4 cm. The previous 75 cm spawn guard accepted that
transient offset and rigidly retained it. Visibility worked, but the sword
remained far from the hand.

Only `It_1H_MagicFlameSword_01` now receives an explicit handle-to-fist contact
after its existing blade-angle alignment. The native flame sword mesh has its
guard near Z=0, its narrow handle between about -26 and -4 cm, and a pommel
near -31 cm. Its handle contact is Z=-16 cm, scaled by world units per metre.
The calibrated closed-fist contact uses the existing accepted palm geometry.
This replaces only the attachment translation. Hand tracking, weapon rotation,
weapon angle settings, weight, native spell creation and duration remain intact.

The same attachment supplies the pre-cull mesh, frozen stereo poses and melee
collision segment. The reversible summoned-item visibility scope from 0.23.83
and accepted fire performance policy from 0.23.82 remain unchanged.

Regression coverage includes the recorded 68.4 cm initial offset, three world
scales, 240 controller rotations and translations per hand, nonidentity hand
calibration, both eyes, unchanged mesh orientation and byte-identical ordinary
weapon poses. Headset alignment still requires confirmation.
