# Gothic 3 VR 0.18.0 — direct ground movement and crossbow render continuity

The user accepted the bow as complete in 0.17.1, and retained the current crossbow guidance, grip and unsupported sway. This release addresses extra steps after joystick release and intermittent support-hand/held-bolt poses. New behavior still requires a headset test.

## Ground movement

Earlier changes adjusted native acceleration/braking or filtered the velocity after native movement planning. Those approaches did not remove the animation-driven stopping tail. Direct VR locomotion now replaces the active human player's normal controlled ground translation generator, before native moving-floor contribution and physics, with a controller-authored horizontal velocity. There is no camera position filter while this setting is enabled.

The full-hash guarded Game.dll hook is at ProcessControledTranslation RVA 0xE2410; its complete six-byte prologue is checked. The replacement uses the native directional maximum, walk/sneak/sprint speed modifier, current slowdown modifier, terrain scale and scaled frame time. The native speed scalar at offset 0x158 and AddToCurrentVelocity receive the corresponding values. Neutral input contributes zero every frame. Controller ownership persists after release so native goal braking cannot take over again a quarter second later. A fresh direction is accepted immediately, without calling StopMovement or waiting for a stopping action.

The surrounding native movement callback still performs collision correction, sensor/terrain updates, rotation, moving-floor contribution, physics and frame-state saving. NPCs, navigation-controlled movement, scripted combat, mounts, jumping/falling, water, knockdowns and animal forms use their original paths. Menu/pause/focus/tracking/context gates restore native handling. Room-scale transfers retain their existing collision check and fixed-step/history synchronization. This replacement does not rewrite the actor's position or collision history to force a stop.

Motion → **Direct VR locomotion — separate movement from animation** enables the replacement. **Movement start response** is a short acceleration ramp only: 0 ms starts immediately, 60 ms is the default, and the range is 0–150 ms. The existing LocomotionResponseMs INI key/value is preserved. Release has no mod-owned braking tail. **Instant turn response** retains the accepted mouse-rotation bypass. **Legacy camera smoothing** operates only with direct locomotion disabled. Stable horizon, player-anchored eye height and immediate physical headset/controller tracking remain active.

Stopping logs now include `vr_stop_trace` for 600 ms after release, recording joystick intent, actor root, eye base, body horizontal velocity, motor speed and room-transfer count. This can distinguish residual physical actor motion from a camera offset if the next headset test still shows a jump. `vr_ground_motor` samples record native speed/modifier values when diagnostic sampling is enabled; hook refusal/fault logs identify fallback to native movement.

## Crossbow render continuity

The native animation palette can report a cache hit after the mod has restored its borrowed transforms. Previously that skipped reapplying both the tracked support hand and held bolt. Crossbow palettes now reapply their current VR pose on these cache hits; the native return value and restoration behavior remain unchanged. Bow palettes retain their existing behavior.

Support contact is resolved once before the stereo frame and read unchanged by both eyes and subsequent mesh passes. A late render callback no longer releases the grip by checking its timestamp halfway through drawing. Support geometry also remains available when held-ammunition geometry is temporarily absent. Brief input/tracking gaps cancel trigger input without moving an already owned, matching crossbow bolt back into the ammunition stack; actual player/weapon/slot/context changes still restore it normally.

The accepted left-guided supported orientation, palm calibration, one-hand sway, native reload skills, projectile damage/velocity and ammunition templates are retained. Bow charge/draw policies are byte-identical to 0.17.1; no native game assets or DLLs are replaced.

## Validation and headset probe

60/60 CTest checks pass. Added coverage checks zero ground contribution throughout 100 neutral frames, immediate restart/reversal, duplicate-frame idempotence, native speed modifiers, normalized diagonals, persistent controller ownership, cached palette application, frozen stereo support and held-bolt retention gates. Existing native binary/ABI checks, ranged progression, exact arm reach, melee, magic, animal return, room-scale, UI and stereo GPU checks also pass. All 13 launcher pages pass the built-in layout/capture check; Motion was visually inspected.

1. Keep Direct VR locomotion and Instant turn response On; try Movement start response at 60 ms. Walk, release, then immediately move backward or sideways. Check for extra steps, camera jerks and blocked restarts.
2. Repeat with physical steps, a two-hand weapon, on a slope and near a wall. Verify collisions, jumping and recovery still behave normally.
3. Hold the crossbow with both hands, guide it slowly, walk and fire at least six bolts. Watch the support hand and the loaded bolt for flickering. Check a menu and weapon switch afterward.

Automated checks validate the implementation and installed native profile; they do not establish headset comfort. Installation preserves the user's INI byte-for-byte and backs up replaced binaries. The game is not launched unattended.
