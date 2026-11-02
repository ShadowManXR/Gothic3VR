# Gothic 3 VR 0.20.2 — finishing focus and ranged recovery

## Finishing focus

The user reports FINISH only during the instant an enemy falls, disappearing once the enemy stays down. Selection now enumerates the native gCInteractionAdmin interaction ROI, which is also used by the native Script entity query. It no longer requires a downed NPC to appear in a dynamic-physics capsule query. Current native IsDown, IsDead, human/orc species and reach requirements remain. No state, health or progression gate is bypassed.

The controller ray rates the NPC's world tree boundary. Visibility is tested from the controller toward a point inside that grounded body boundary, above its lower face, rather than using the NPC's standing sight/eye helper. Player/victim-owned shapes are ignored; intervening objects and walls still block selection. No stale target pointer is retained between scans. The sampled vr_finisher_focus message records ROI, eligible-down, ray, sight and selected counts for further headset diagnosis.

With a melee weapon drawn, point at the downed living human/orc within reach until FINISH appears. Release buttons first, then hold right grip + right trigger for 0.8 seconds while aiming at the same target. The native PS_Melee_FinishingAttack state owns the animation, protected-victim rules, death, witnesses and quest consequences. Mere selection does not execute a kill. Actual on-ground selection and finishing animation still require user testing.

## Crossbow / ranged fault recovery

The latest 0.20.1 session diagnostic-20261013-233655-20464.jsonl shows six successful bow shots, then EXCEPTION_ACCESS_VIOLATION in physical ranged update at 238802.826 ms. Subsequently the crossbow model and tracked arms keep rendering, but the global gBow.fault latch prevents ammunition update and the shared aiming reticle. This explains the reported crossbow failure; the exact faulting native instruction was not captured in this session.

RestoreAmmo now clears borrowed ownership before invoking inventory callbacks. It restores only when the owner is still the current native player and both the current right-hand entity and stack index match the saved borrow. Fire forgets the old held ammo before the native Shoot callback, which may delete/replace it. The native script continues to own ammunition consumption and projectile damage.

A guarded ranged exception abandons retained pointers and schedules recovery for the currently validated player/weapon/session. The same weapon can retry after 500 ms; a different equipped ranged weapon or new player/session does not inherit that failure. Missing/stale samples cannot erase the retry delay. Recovery resets physical trigger edges and never queues or replays a shot: release the trigger and press/draw again. This is a recovery delay after a fault, not a crossbow reload timer. Native reload motion duration and skill multipliers remain unchanged.

The shared reticle becomes available again when the shooting context recovers. The finalized bow charge/draw math, crossbow handling, aiming stabilization and tracked weapon rendering are unchanged. NativeBow.h changed only for ownership/fault lifecycle. BowCharge.h, PhysicalBow.h, CrossbowHandling.h and NativeTrackedBow.h are byte-identical to 0.20.1.

## Validation and test

64/64 automated tests pass. New regressions cover missing-sample recovery delay, same-weapon retry, bow-to-crossbow and save-player fault isolation, and grounded/thin-body sight points. Existing trigger-release/no-queued-shot, native reload progression, ammunition, rendering, melee and native-locomotion tests remain green. Fourteen launcher pages pass their capture/layout check. Tests do not confirm actual NPC discovery, native finishing completion or fault-free crossbow operation in a headset.

Installation backs up binaries and preserves the current user INI byte-for-byte. Native locomotion from 0.20.1 remains; no direct movement motor is reintroduced. Gothic 3 is not launched unattended.

User probe: equip an available bow, fire and switch to crossbow; check the reticle, aim with/without left support and fire at least six separate trigger presses, respecting native reload readiness. Switch/load and re-equip without restarting. Knock a non-hostile human/orc unconscious, let the body settle, then select and deliberately finish; check logs if FINISH still fails at one of its stages. Check normal exit.
