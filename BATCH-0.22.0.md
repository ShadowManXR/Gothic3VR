# Gothic 3 VR 0.22.0 — swing speed, lockpick controls and feedback

This batch addresses the six feedback points after 0.21.1. It retains native locomotion, learned skills, damage calculation, stamina costs and the finalized bow/crossbow policy. The installer does not launch the game. Current saved calibration and preferences are preserved byte for byte.

## Melee velocity and distance

1H Melee Weapon, 2H Melee and Staff / Halberd each gain Minimum swing speed, initially 100 cm/s (1 m/s), adjustable live from 50 to 400 cm/s in 10 cm/s steps. Each profile retains its independent Damage swing minimum and Stamina swing minimum, in 10 cm steps. Both physical hands in dual wield use the 1H settings.

A hit now needs both distance and speed. The qualifying distance must be traversed continuously at or above the configured speed. Slow travel cannot build credit for a small fast flick. Slow contact, a stationary weapon and small repeated tremor produce neither physical damage nor attack stamina consumption. Speed comes from calibrated weapon endpoint displacement and the real XR sample interval in tracking space; joystick movement, native animation and actor/world translations cannot add speed. Translation and rotation of the weapon both contribute. Lost tracking, stale or discontinuous samples, equipment/session changes and calibration reset the appropriate gate. Reused XR samples retain the existing gate without adding travel.

This gate runs before native stamina debit, contact queries and damage delivery. Costs remain once per stroke, and existing normal/power/unsupported-2H/native skill rules remain. The new gate does not override native finishing or other non-swing actions. Existing distance values are retained; use Save settings to apply speed changes live. Diagnostic vr_melee_velocity records measured and required speed plus readiness.

## Lockpick and walking controls

In menus, left joystick left/right sends extended keyboard arrows. A lockpick native action (SDK gEAction_Lockpick = 100), or a lockpick routine task/state, additionally forwards the user's configured StrafeLeft/StrafeRight keys. This handles both menu navigation and puzzles that read the game's directional action bindings. Let the left joystick return to neutral after entering the puzzle, then move left/right. Neutral re-arming prevents the movement used to approach a chest from becoming an accidental lockpick command. Focus/tracking loss releases emitted keys. Generic menu navigation does not issue player movement keys. Right pointer and trigger selection remain available.

No lockpick difficulty, learned skill requirement, lockpick inventory consumption or chest protection is changed. Actual operation of the user's installed lockpick puzzle still requires a headset test; this build verifies translation and real Windows key binding/extended scan-code construction, not live puzzle acceptance.

Native joystick movement now has a roughly 35-degree forward/back cone. A slight diagonal stays straight; deliberate diagonal or sideways input still strafes. Once strafe starts, its return threshold is about 25 degrees, preventing direction chatter. Run, left-grip skilled Sneak, R3 + forward native Sprint, native terrain/animation and speed remain as before. This is input filtering, not a replacement locomotion motor.

## Status and finishing camera

Active native Sneak uses a compact gold eye icon near the bottom left instead of a large SNEAK label. Native sneak haptic feedback remains. FINISH is smaller, red and bottom centre, and still depends on a currently eligible grounded target. Indicators use a head-relative VIEW layer in both eyes and hide in menus/radial.

For a finishing kill, aim at the downed target with a drawn melee weapon and hold right grip + right trigger for 0.8 seconds, as before. The requested native finishing state immediately yields to third-person through the existing activity camera path, including the short interval before the native action is assigned. The full body/head is visible and first-person tracked-arm overrides are suspended for the native animation. On completion, Always first person returns through the existing calibrated human eye anchor and camera reset. Normal melee and recovery keep their previous camera policy. The native finishing animation remains responsible for kill permission, protection, witnesses and quest credit.

## Verification

66/66 automated tests pass, including a new combined regression for slow long moves, slow travel followed by a tiny fast flick, qualified fast distance, angular endpoint speed, stationary contact, tremor, duplicate samples and tracking gaps. Native controller key neutral/release handling, 35/25-degree steering, actual transparent eye/red FINISH pixels and finishing third-person classification/return are covered. Settings tests cover existing-INI defaults, independent profile saving, clamping and ten-unit steps. All 15 launcher pages pass layout checks; 1H, 2H, Staff and indicator captures are visually inspected.

The five finalized ranged headers, locked melee attachment/tracked arms, native combat rules and two-hand handling headers are byte-identical to 0.21.1. Native game module hashes are checked at packaging. No native game assets are included. Real headset comfort, lockpick puzzle input and finishing animation require user confirmation.
