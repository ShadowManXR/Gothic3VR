# Rigid staff grip — 0.21.1

The accepted calibrated staff/halberd palm profile is locked. With support, both visible palms use the common weapon frame. One-hand orientation and live grip controls remain. See FIXES-0.21.1.md.

## Previous release notes

# Staff / Halberd — 0.13.6

The user confirmed both Staff/Halberd palm contacts in the 0.13.6 headset probe. Right-palm orientation and upward direction were accepted earlier. This confirms visual grip contact; the feedback does not separately validate both-end damage, every staff/halberd template, or a particular contact setting value.

The headset probe of the standard 2H baseline showed the hero's right palm turned upside down. Staff and Halberd have different native attachment axes from swords/axes. Their VR profile now transforms the weapon independently instead of correcting those axes by rotating the right hand.

Native Staff (12) and Halberd (51), including Blade Staff, share this separate profile without item-name overrides. Native use types, skills, ownership, damage and spell bonuses are preserved. The native attachment supplies a cached local shaft contact once per equipped weapon, using the same pivot-centered shaft as support capture rather than the midpoint of an asymmetric blade-head bounding box; it does not animate the VR wrist. Each rendered weapon pose places that contact at the calibrated right palm plus configurable finger-space and mirrored palm-depth contact offsets. The right hand retains normal controller tracking. The longer mesh reach is resolved relative to that actual contact, independently of min/max endpoint order.

The launcher page Staff / Halberd exposes Weapon pitch, Weapon yaw, Weapon roll, Support grip radius (default 15 cm), Grip toward fingers (default +4 cm, -10 to +10 cm), and Grip palm depth (default +2 cm, -10 to +10 cm). Positive finger values shift contact away from the wrist. Positive depth moves contact toward the palm-facing side: grip -X in the right hand, +X in the left. Both offsets apply to the support-hand contact as well as the primary contact. The attached left wrist is reconstructed from that corrected hand-local contact instead of the raw palm landmark. The upward long-end direction confirmed by the user is fixed. Long end toward little finger was removed at the user's request; legacy StaffReverseGrip values are ignored. Save settings applies live and cancels an active stroke/grab; it preserves accepted 1H, 2H sword/axe, hand and bow settings. Existing staff angles and grip radius remain compatible; StaffGripForwardCm and StaffGripDepthCm store contact offsets. Existing finger-offset settings are preserved.

Bring the free left palm to the shaft and hold left grip. A captured weapon-local contact keeps the support palm attached. Controller positions steer the shared weapon frame; wrist twists are combined rather than competing. Each frame reanchors the coordinated mesh at the corrected right-hand contact, including native origin/heading changes with repeated XR timestamps. Release left grip to free the left hand. The existing tracking, menu, occupied-hand and session guards remain active.

Both shaft endpoints participate in swept/capsule contacts. Staff physical swing intensity uses measured shaft rotation/translation in tracking space; axial roll and native world motion cannot generate fake attacks. Native damage calculation and the per-weapon cap remain active, with one damage event per target per stroke. Bounds approximate the shaft; individual meshes still need visual checks. Mage spell casting remains native.

46 automated tests pass. New regressions cover Blade Staff bounds (-166.6 to 95.7 cm), swapped endpoint order, a shifted attachment, fixed accepted direction, contact displacement and weapon-only calibration, unchanged controller/right-hand transforms, both palm anchors, locomotion continuity, both-end contact, world/roll rejection and damage deduplication. The user accepted the palm orientation and upward direction in 0.13.4. The 0.13.5 contact was reported on the back of the right hand and at the left wrist. Corrected 0.13.6 palm contacts were confirmed by the user in the headset.

Logs: vr_tracked_weapon reports native type and pose_type=12; vr_staff_weapon_only reports palm_error_cm and right_hand_untwisted=1, grip_forward_cm, depth_cm and fixed_direction=1. Error is measured against the actual rendered hand palette plus its contact offset. vr_two_hand_palm_anchor measures the rendered support-palm contact; vr_melee_swing and vr_melee_hit describe physical strokes.
