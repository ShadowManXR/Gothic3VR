# 0.23.76 - Freeze the crossbow support hand after resolving its contact

The crossbow grip stayed logically latched, but the avatar's left hand still
followed the separated controller. The hero palette was frozen before the
current frame's ranged mesh and support-palm contact were prepared. The frozen
render path then reused the earlier free-hand pose in both eyes, bypassing the
later valid attachment target.

Prepare the ranged mesh, nock and crossbow support geometry before the single
hero-palette solve/freeze. That snapshot now contains the foregrip-attached
left palm. Both eyes and repeated mesh/cache passes reuse it, retaining the
once-per-frame rig preparation. No additional per-eye IK solve is introduced.

Holding the left grip keeps the rendered palm at the captured foregrip contact
as the controller moves or rotates away. Release still releases the hand;
equipping a different weapon requires a fresh nearby grab. Existing left-hand
aim guidance and right-hand weapon positioning remain in charge of the weapon.
The right-hand grip, native reload/skill progression, bolt placement, bow
behaviour, hand calibration and accepted landscape FFR are unchanged.

The contact/frozen-palette regression exercises 200 moving/rotating weapon
frames with the raw support controller deliberately far away, multiple passes
in both eyes, nonzero palm calibration and restoration of native palettes.
Native render ordering is reviewed in `StereoCameraRender`; headset acceptance
still requires checking the actual avatar/weapon contact in the game.

Only the game plugin changes. The 0.23.75 launcher remains installed.
