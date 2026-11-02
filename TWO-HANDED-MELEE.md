# Support fist contact —0.22.10

Standard2H swords/axes anchor the left grip at the knuckle crease with mirrored palm depth. Staff uses its accepted separate contact settings. 75/75 tests pass; headset acceptance pending. See FIXES-0.22.10.md.

## Previous release notes

# No support-acquisition snap — 0.22.6

Left grip captures current primary orientation; both hands then transport that frame together. Calibration offsets do not realign the weapon shaft to raw controller centres. 71/71 tests pass; live acceptance pending. See FIXES-0.22.6.md.

## Previous release notes

# Fixed palm contact — 0.21.1

2H attachment is captured once per drawn weapon. Unsupported weight acts on hand and weapon together; two-hand support keeps one shared pose. Grip cannot rotate independently inside the visible palm. See FIXES-0.21.1.md.

## Previous release notes

# Two-hand melee — 0.16.1

With **Heavy one-hand handling** enabled (default), unsupported 2H swords/axes rotate with tracking-space inertia, produce 20% of the calculated VR stroke damage and accept at most one unsupported contact per 0.9 seconds. The tracked palm remains immediate; only the weapon rotation is weighted. Actual left-grip attachment restores direct coordinated steering and normal stroke damage. Releasing or capturing support cannot create a free strike. Staff/Halberd and 1H weapons are excluded. The checkbox in 2H Melee can disable this comfort/gameplay option. 0.16.1 halves the rotational response time (0.28 to 0.14 s) and doubles its angular rate limit (1.6 to 3.2 rad/s), while keeping the unsupported damage/spam restriction. Native post-armour damage bonuses are no longer capped to weapon base damage. New headset validation is pending; accepted attached grip, shared rotation and walking behavior are retained.

Equip a weapon you can use under the native game skill/strength rules. The right hand holds it. Move the left hand near its handle and hold the left grip button (squeeze, not trigger). Capture uses the actual rendered handle/shaft and calibrated palm, including native attachment offsets. While held, the support palm and orientation stay attached to a captured weapon-local contact; swords/axes use one shared weapon frame: controller positions steer its direction, and wrist twists are averaged around the weapon axis. The frame is transported continuously through vertical directions. Controller pitch/yaw cannot independently steer the blade or move the held hand through calibration offsets. Staffs and halberds have a separate weapon-only frame that preserves right-palm tracking; see STAFF.md. Releasing grip returns to right-hand steering. The left hand must be free. A shield, torch or other off-hand item prevents support-hand capture.

2H Melee provides separate pitch/yaw/roll, Support grip radius (default 12 cm, 5–25 cm), and a Two-hand grip checkbox. Save settings applies live and cancels an existing gesture safely. Existing 1H, hand, bow and Bow grip settings are preserved. Native slot, ownership, skills, inventory and saves are unchanged.

The native SDK use types 1H=2, 2H=3, Staff=12, Halberd=51 and Axe=52 are shared by pose generation, click suppression and physical damage selection. These correspondences are asserted at compilation. All other item categories remain excluded. 1H contact bounds permit short weapons down to 8 cm; long melee bounds permit up to 3.5 metres. Swords/axes use pivot-to-striking-end contact; Staff and Halberd use both shaft endpoints and a separate shaft grip.

Capture requires both controllers, current focus, fresh tracking, an initial grip release followed by a press, and proximity to the shaft. The support range is limited near the handle for standard 2H swords/axes; Staff/Halberd use their shaft. The captured side of the dominant hand is retained so a rear/pommel grip does not reverse the weapon. Menus, the radial, recentering, weapon/session changes, stale/jumping tracking and occupied left hand release the capture. Returning while grip remains pressed cannot automatically grab. Near-coincident hands or separation beyond 1.2 metres release it. Both tracked wrists retain full reach using the bow's existing tracked IK path.

For swords/axes, render pose and physical swing orientation both use the coordinated grip solver, with raw tracking driving the damage orientation independently of native camera/player movement. Staffs and halberds additionally measure physical shaft motion and reject stationary axial roll; see STAFF.md. This lets the left hand move a supported weapon while the right stays still. Grab/release mode changes settle before damage, so changing the grip cannot create a free hit. Native damage, armour calculation, per-weapon maximum, swept/capsule contacts and per-stroke target deduplication remain on the accepted melee path. No extra two-hand damage bonus is invented.

46 automated tests include category exclusions, short melee, independent staff/halberd frame and both-end contact, long weapon bounds, front/rear grip, release hysteresis, duplicate stereo samples, occupied-hand/menu/focus/tracking/session resets, physical off-hand swings, world-movement rejection and damage deduplication. The user confirmed two-hand steering in 0.12.0, but reported a lateral acquisition offset and a freely moving left hand. The user accepted the 0.12.1 handle capture and palm attachment, but reported conflicting controller rotations. The user accepted 0.12.2 coordinated orientation and position, but reported repeated hand/weapon jumps during locomotion. 0.12.3 fixes a reproduced world-pose cache defect; the user confirmed stable two-hand locomotion, while rotation and palm attachment remain correct, for the tested swords/axes. Tests also cover a 4 cm attachment offset, asymmetric axe bounds, real-handle acquisition, calibrated palm contact through weapon translation/rotation, and release. Coordinated-grip tests cover fixed-position wrist pitch rejection, equal/opposing roll, captured offsets, common vertical/oblique rotations, duplicate stereo calls and stale-frame reset. A regression additionally advances the native world origin/heading at three renders per XR timestamp: all renders must follow the current origin, while genuinely identical stereo inputs remain idempotent. This test fails on 0.12.2 and passes with the 0.12.3 cache correction. Individual weapon mesh alignment, hilt capture position, staff animation overrides and usable skills need the equipped-weapon probe; no claim of validation for all templates is made.

Handle extraction uses the longest mesh axis and its local pivot; sword capture is restricted below the striking blade, other heavy weapons exclude the head end, and staffs/halberds use the shaft with their own grip radius. Bounding-box extraction is a conservative approximation; individual unusual meshes still need visual validation. No additional native assets are shipped.

Logs: vr_two_hand_palm_anchor (actual render palette palm-to-weapon error; reused_tracking_world_updates counts origin/heading updates with a reused XR timestamp), vr_tracked_weapon (native use type), vr_two_hand_grip, vr_melee_blade, vr_melee_swing and vr_melee_hit. See MAGIC.md and CROSSBOW.md for ranged integration. Shields and explicit special-ammo selection remain separate work.





