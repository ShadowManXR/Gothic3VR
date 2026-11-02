# Gothic 3 VR 0.17.0 — locomotion comfort and follow-up fixes

## Bow

All three VR draw speeds are multiplied by 1.3 relative to 0.16.4. At full physical draw, the verified 1.95 s native charge duration becomes approximately 1.5 / 1 / 0.75 s for the base / Archer / Master Archer tiers. The visible string and arrow use this same effective charge. Native full-power damage, ammunition, skills and the existing linear charge-to-projectile-speed curve are preserved. This tier draw adaptation is a VR policy, not a newly claimed native formula.

## Crossbow

Support captures the existing gun orientation and the left controller's relative orientation on attachment. Subsequent changes of the hand-to-hand line guide the weapon; left wrist twist is applied relative to that capture. The right wrist supplies only the rear position. It no longer forces the initial left roll onto the gun, which caused the reported roughly 45-degree jump and changed the attached palm. Computation remains in tracking space to avoid world-motion feedback. The accepted unsupported sway, common gun/reticle/bolt direction, support-palm contact and native reload progression remain active.

## Transformation disposal

The SDK EndTransform restores the human but leaves disposal to its caller. The installed Script_Game transform-expiry caller at RVA 0x5852C additionally assigns species EMPTY_B (40), routine Dead (9), moves that old transformed entity to (-220000, 100000, 100000), then calls Decay. Manual X+Y return now follows that sequence **only after the active player changed to a different, valid human entity**. It acts only on the player's captured animal form. It does not kill ordinary animals, grant XP, or sweep previously saved entities from the world. Automatic native expiry keeps its native cleanup. The accepted third-person animal view and human eye-height restoration are retained.

## Motion page

Three settings apply live through Save settings:

- **Stable locomotion**: defaults On. Ground movement follows the current controller command and horizontal headset heading, rather than incidental sideways/backward animation velocity. Diagonals stay normalized. A deliberate movement-stick release invokes StopMovement once and zeros horizontal drift; physical room-scale steps never invoke that stop. Faster scoped acceleration/braking and removal of native animation-velocity inertia reduce native lag. Sensor direction correction is temporarily disabled, while floor detection and physical collisions remain native. Native max speeds, run/sprint modifiers and stamina are preserved.
- **Walking speed stabilization**: default 60 ms, range 0–150 ms. Filters the magnitude of native horizontal velocity, not direction or tracked head/hands. Zero disables this speed filter. Zero native speed stays zero. Stops and direction reversals have immediate response. The redirected velocity reaches both the rigid body and native cached current velocity through the verified SetCurrentVelocity entry; they cannot disagree about the chosen direction.
- **Instant turn response**: defaults On. Bypasses native UpdateDeltaRotX filtering for the player's active first-person VR camera and stores the current input in its native accumulator. Neutral input is neutral immediately, with no filtered-input tail. The original filter remains available outside that scope or with this setting off.

Stable locomotion excludes jump, fall, slide, swimming, mounting, animal form, recovery/knockdowns, dialogue, pause, stale tracking, menus, radial input and native scripted combat/navigation movement. NPC movement is unaffected. A neutral controller never takes ownership of keyboard movement it did not start. Native movement properties restore after every intercepted process call, including exceptions. Accepted room-scale collision/transfer logic remains in its existing post-physics order.

Horizontal camera position smoothing is disabled while Stable locomotion is On to avoid adding position lag after a stop. Existing vertical camera stabilization remains, and all XR pose updates stay immediate. These changes address simulation/input inertia; they do not fix frame-rate drops or remove every native leg animation. Actual comfort and residual movement require headset validation.

## Validation and short headset probe

58/58 automated tests passed: including new controller ownership/stop/direction/speed cases, native property restoration under an exception, native x86 velocity and void/float mouse ABI, actual Game.dll instruction boundaries and Script_Game animal cleanup constants. Existing room-scale, damage/reload/progression, tracked arms, stereo GPU, menus and calibration tests remain passing. All 13 launcher pages passed the built-in layout/capture check; Motion was visually inspected. The installer preserves the current user INI byte-for-byte. Missing new keys receive their defaults without rewriting the INI. No game modules or animation assets are patched on disk.

1. Start with both Motion switches On and speed stabilization at 60 ms. Walk forward, sideways and backward; release the stick abruptly and reverse direction. Compare each switch Off/On if a problem remains. Check a slope and a nearby wall.
2. Run with the joystick while taking small physical steps. Check that room-scale motion still combines with joystick movement. Repeat with a 2H weapon held in both hands.
3. Rotate with the right stick and release it. Check that rotation stops promptly, while real head rotation remains immediate.
4. Check full bow string extension and repeated shots; attach the crossbow support hand without changing its wrist pose, then guide it with the left hand.
5. Transform and return with left X+Y held 0.5 s. Check human height and that the newly relinquished animal is absent. An animal already stored in an older save is not removed by a world-wide cleanup.

Runes and scrolls were accepted in 0.16.4 and their grip behavior is unchanged. Game was not launched for this release; new visual/comfort behavior awaits the user's headset test.
