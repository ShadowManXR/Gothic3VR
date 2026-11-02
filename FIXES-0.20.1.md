# Gothic 3 VR 0.20.1 — native locomotion

The user rejected Direct VR locomotion as worse than the original game. This release removes the direct translation generator and velocity redirection hooks from the runtime, together with the Direct VR locomotion checkbox and Movement start response slider. Standard/native locomotion is now the only joystick locomotion path.

## Controller actions on native locomotion

| Action | Control |
| --- | --- |
| Run | Move the left joystick |
| Sprint | Hold R3 while pushing the left joystick forward; native rules/stamina apply |
| Sneak | Hold left grip and move with the left hand free; learned native Sneak required; release grip to exit |
| Main menu / Back | Y |
| Radial | L3 opens; right joystick selects; A confirms; B cancels; Y changes page |
| Camera reset | Hold L3 + R3 for 0.5 seconds |

The reported regression came from gating Sneak and Sprint on direct locomotion and its valid direction/translation hook. Those conditions are removed. Both now use a separate controller context with fresh input, normal player state and native first-person gameplay. The Sneak skill and left-hand support checks remain; a supported 2H weapon or crossbow takes priority over Sneak. The native SNEAK badge and state-change vibration requests remain. Missing native skill retains NO SNEAK SKILL feedback.

Controller Run/Sneak select only gCCharacterControl_PS::SetWishedMovementMode before the original ProcessMovements consumes that mode. No wished direction is overwritten. Native movement keeps its original goals, translations, velocity, acceleration, braking, animation, speed, terrain and collision decisions. Sprint still sends the user's configured native SessionKey.Walk binding (Shift by default) and yields mode selection to the native handler; Fast is never forced. Normal controller movement selects Run instead of Walk, including after releasing Sneak or Sprint. The controller-owned Walk toggle remains blocked. Untouched keyboard locomotion is not captured.

The existing native camera-heading input bridge remains for head-directed movement. Accepted collision-checked physical room-scale transfers still run after native physics; their scoped notification guard remains. This is independent of joystick locomotion. Native jumps, water, knockdowns, animal form, activities and menus keep their own handling. Activity third-person and return-to-first-person remain from 0.20.0.

## Settings and compatibility

Motion now exposes only the independent Instant turn response preference and a description of native controls. Fourteen launcher pages remain, with 58 numeric settings. Radial size and six independent per-category damage/stamina sliders retain their original INI key identities and values despite removing the obsolete acceleration slider. The melee thresholds remain 10–200 cm in 10 cm steps; small adjustments below the configured damage gate do no damage. See BATCH-0.20.0.md for the independent threshold policy and activity behavior; its direct locomotion notes are superseded by this release.

The old StableLocomotion, DirectVelocity and LocomotionResponseMs keys are ignored by the runtime even if an old INI still contains enabled values. Saving in the new launcher removes those obsolete keys atomically while preserving unrelated sections and current hand/weapon/camera preferences. Installation itself preserves the existing user INI byte-for-byte. Bow and crossbow handling are unchanged.

## Validation

64/64 automated tests pass, including new native mode ownership, Run/Sneak priority, no forced Fast, stale/focus/menu/actor reset and keyboard non-ownership cases. Real Windows INI tests verify obsolete-key cleanup and unchanged radial/swing key identities. Existing rendering, native binary, room-scale, activity camera, haptic, melee and ranged regression tests remain green. Some isolated historical locomotion helper tests remain in the test suite; they do not imply that the removed motor is active.

Fourteen launcher pages pass the layout/capture check. Motion, 1H Melee Weapon and Radial were visually inspected. Native game module hashes and byte-identical finalized bow/crossbow headers are verified. The installer backs up binaries, updates both game and development launcher entry points and preserves the user INI. Gothic 3 is not launched unattended.

Headset test remains pending: normal native Run, release/restart/reverse, R3 Sprint and native stamina, left-grip Sneak and badge/vibration, releasing back to Run, left-hand weapon support, Y menu, physical steps and activity third-person. No claim of headset acceptance is made by the automated checks.
