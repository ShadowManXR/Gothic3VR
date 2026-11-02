# Gothic 3 VR 0.16.3 — bow charge, animal view and casting grip

## Bow charge and first arrow

The installed CP/PUP `PS_Ranged_PowerAim` uses `0.2 + 0.8 * min(stateTime / 1.95, 1)` for charge. The player `Shoot` callback uses `200 * charge` for velocity. Previously the VR hook multiplied the cloned arrow's inherited charge and velocity again, using a square-root velocity curve. Thus the first projectile depended on the held arrow's stale/uninitialized charge, and later projectiles approached full velocity too quickly.

Every VR bow release now initializes the newly spawned player's actual arrow with this release's charge and native velocity constants. Velocity is linear: 25%, 50%, 75% and full charge yield 80, 120, 160 and 200 in native ShootVelocity units. The native charge floor is 0.2. A short/non-drawn gesture still does not fire. Crossbows retain their native velocity and damage multiplier. Actual ammunition templates, consumption, ownership, effects, attribute/skill bonuses and target armour remain native.

Bow charge is limited by both physical draw distance and game-time readiness. For equal short draws, a higher learned bow tier reaches more charge; holding a partially stretched string cannot exceed its physical draw fraction. Full draw defaults to the existing 50 cm setting. Readiness uses scaled native frame time; menus, pause, stale/lost tracking, identity/session changes and duplicate samples cannot accumulate or release invalid shots.

**VR progression policy:** active `Perk_Bow_3` takes precedence over `Perk_Bow_2`. Draw-speed factors are 1x / 1.5x / 2x, giving full-charge times approximately 1.95 / 1.30 / 0.975 seconds. The installed script's charge duration is fixed; these tier factors are an explicit VR adaptation requested by the user, not a claimed native bow-tier formula. Native perk damage calculations are not replaced or duplicated. No skill or attribute is granted.

## Animal third-person

Transformation detection was already observed in the native log, but `SetMode(Normal)` consulted the old first-person state and could redirect back into the first-person preset. Before selecting Normal while transformed, the mod now releases the persistent, requested and temporary FP latches on the active camera. Their offsets and the native read/write branches are checked against the hash-guarded installed Game.dll. Humanoid overrides remain suspended while animal form is active; the configured first-person policy resumes in human form. Native transformation duration and druid progression are unchanged.

## Scrolls, runes and other held Cast items

The native item-to-wrist translation changes between idle and walking and cannot define a stable VR grip. Casting items now retain their captured attachment orientation, but their local mesh centre is anchored to the calibrated palm before either eye's culling. This removes the floating wrist/carry offset, including the rune offset in front of the right hand. Existing hand calibration and exact controller reach stay active.

The render skeleton applies a consistent closed grip derived from the hero's bind finger chains while holding a Cast item. Segment lengths and knuckle bases are preserved. The native idle/walk/cast finger pose and item transform/visibility are restored after each eye; spell input, mana, scroll consumption and item ownership are unchanged. Bow, melee and supported 2H finger handling are unaffected.

## Validation and headset probe

54/54 automated tests pass. New checks cover zero/stale inherited first-arrow values, linear velocity and charge, tier-dependent identical draws, partial-draw limits, duplicate/pause/cancellation behavior, camera latch release, native binary constants/offsets and palm/finger contact across idle and walking. Existing ranged recovery, 2H, dual wield, magic, extended reach, locomotion, GPU stereo and SDK/native ABI checks pass. Launcher code/UI is unchanged. These changes have not yet been accepted in a live headset test.

1. Load the reproducing save and draw a bow directly from its quick slot. Fire the first arrow and several following arrows with equal draws; then compare short, medium and full draw at an existing learned tier. For a full-power arrow, allow the tier's readiness time as well as full physical draw.
2. Confirm the crossbow still fires repeatedly with its native reload timing. Check special ammunition separately if available.
3. Transform into an animal: the native third-person camera should show the animal. Return to human form and verify automatic first-person, camera height and controller alignment.
4. Hold a scroll while stationary, then walk, stop, rotate the wrist and cast. The paper should remain in the closed palm. Repeat with a rune and teleport item, checking menus and consumption.

Logs include `vr_bow_progression`, `vr_bow_draw` with power/velocity/tier, `vr_ranged_release` and `vr_animal_third_person_requested; FP_latches_released`. Native parrying, animation-combo gestures and physical attack-stamina parity remain the documented existing gaps; see PROGRESSION-AUDIT.md.
