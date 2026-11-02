# Gothic 3 VR 0.17.1 — bow mastery, crossbow guidance and nonblocking braking

## User acceptance retained

The user accepted the crossbow grip/orientation and transformation disposal in 0.17.0, and scroll/rune contact in 0.16.4. Those attachment, cleanup and casting policies remain unchanged. This release addresses supported crossbow jitter and the locomotion regression introduced in 0.17.0.

## Bow progression

Base draw speed is now 1.4 times the original VR policy; the middle skill is 1.5 × 1.4 = 2.1. This is **40% above the original**, rather than multiplying the previous 1.3 by another 1.1. At full physical draw, readiness takes approximately 1.393 / 0.929 seconds for the first two tiers.

Master Archer (Perk_Bow_3) uses physical draw directly: no time-limited readiness, minimum hold duration or artificial 300 ms repeat cooldown. The string follows effective physical draw, and the existing charge-to-arrow-velocity/damage curve still reflects how far it was drawn. Fresh tracking, a deliberate string re-grab/release, minimum physical draw, actual ammunition/ownership and menu cancellation remain required. Native skill damage bonuses and ammunition consumption are unchanged.

## Crossbow support guidance

Supported orientation is now computed directly from the captured grip and current tracking-space hand line and left wrist twist. It does not integrate a previous solved weapon pose, accumulate roll around a moving axis, or recapture the right-hand aim merely because one frame took over 100 ms. The existing initial contact/orientation is retained, including the fix for the approximately 45-degree grip jump. The right wrist still supplies only the rear position while supported.

Aim stabilization is applied once to the resulting tracking-space orientation. Gun, reticle and bolt share it; native reload progression and accepted unsupported sway remain. Actual loss of tracking/focus, weapon change, session change or release still clears the attachment through existing outer gates.

## Locomotion regression

The explicit native StopMovement call added in 0.17.0 is removed. It could enter a native stopping transition that temporarily blocked new movement input. There is no mod-owned stop action or pending animation to finish before another direction is accepted.

The additional output speed filter and forced horizontal zero on release are also removed. They changed the speed after the engine had planned its movement, potentially provoking corrections from its goal/frame-history calculations. Both native cached velocity and rigid body now receive the native horizontal speed magnitude without a second smoothing stage. Direction stays aligned to controller intent; during brief native braking, the last requested direction is retained instead of following incidental backwards/sideways animation displacement. Fresh input immediately replaces that direction. Vertical physics and zero native speed pass through.

The Motion field is renamed **Movement response** (existing LocomotionResponseMs key/value retained). It controls native acceleration/braking itself using the maximum ground speed and the chosen response interval. Default is 60 ms; 0 selects the fastest bounded response (20 ms internal minimum), and values up to 150 ms permit gentler braking. It is an approximate response target, not a guarantee that every animation/root impulse completes in that time. Native movement properties restore after each callback, including exceptions. No entity teleport, root-history rewrite, forced mode or animation stop is introduced.

Player-only context guards, NPC/collision/terrain handling, recovery/jump/animal exclusions, room-scale transfers and accepted instant turn behavior remain. Horizontal camera smoothing stays off while Stable locomotion is enabled; vertical stabilization and immediate XR pose tracking remain as before. Actual stop comfort and residual native root-motion effects require headset validation.

## Validation and probe

58/58 tests pass. Regression coverage includes immediate neutral-to-reverse input, unchanged native speed and Y during braking, exception-safe restoration of response properties, 4,000 support-motion samples returning to the same captured orientation, a 300 ms render gap without recapture of right-hand aim, and three rapid Master Archer draw/releases without timed readiness or artificial repeat cooldown. Existing native profile, projectile/ammo/progression, room-scale, tracked arms, menus and stereo GPU tests also pass. All 13 launcher pages pass the built-in layout/capture check; Motion was visually inspected. Installation preserves the current INI byte-for-byte and backs up binaries.

1. In Motion, keep Stable locomotion On and Movement response at 60 ms. Walk, release and immediately command another direction. Check that no stopping animation blocks input and no backwards correction jerks the camera. Compare 30 / 60 / 100 ms if useful.
2. Repeat while physically stepping, with a 2H weapon held, and near a wall/on a slope. Rotational tracking and collisions should remain correct.
3. Attach the crossbow support hand and guide it slowly, then through a wider movement. Its accepted grip should remain and it should not pull back toward the primary wrist. Fire repeated bolts with the existing native reload skills.
4. Compare bow base/middle skill charge, then test Master Archer with quick physical draw/release and a short partial draw. Arrow power should follow physical draw at the final tier.

New visual/comfort behavior awaits the user's headset test. Game was not launched by the installer.
