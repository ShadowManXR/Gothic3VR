# Gothic 3 VR 0.19.1 — radial, deliberate swings, sneak, sprint and finishing target

The user confirmed ground anchoring, radial styling, stamina and power strike in 0.19.0. The accepted floor correction and power classification are retained. This follow-up addresses the reported small-stroke stamina drain, direct controller actions, sprint speed and inability to focus downed enemies with a weapon drawn.

## Controls and settings

| Action | Control |
| --- | --- |
| Sneak | Press Y in gameplay; press again to exit, subject to native skill/state rules |
| Sprint | Hold R3 while pushing the left stick forward |
| Radial | Press L3; select with right stick; A confirms, B cancels, Y changes page |
| Camera reset | Hold L3 + R3 for 0.5 seconds |
| Power strike | Hold the equipped melee hand trigger until ready vibration, then physically strike |
| Finishing kill | With a melee weapon drawn, point right controller at the downed human/orc until FINISH appears; release the buttons, then hold right grip + right trigger for 0.8 seconds |

Y still closes native menus and changes radial pages in their own contexts. Animal return keeps its X + Y chord. Sprint has its direct R3 button. Sneak and Walk / Run are removed from the radial utility page, which now contains eight actions; all ten quickslots remain.

Launcher → Radial → Radial menu size: 40–120%, default 80%. 100% matches the previous 0.66 m panel; 80% gives 0.528 m. Save settings applies live, independently of menu/HUD size and resolution. The launcher has fourteen pages, all checked for layout/capture. The Radial page and new eight-sector utility image were visually inspected.

## Deliberate melee strokes

A stroke must accumulate at least 50 cm of actual weapon endpoint travel in tracking space before it can cost stamina or deal damage. World locomotion, camera movement and body animation are cancelled by converting the rendered blade endpoints relative to the matching world controller into raw XR metres. Both blade ends are measured, so a real wrist arc can qualify. A coherent excursion of at least 35 cm additionally prevents repeated 2 cm tremors from accumulating a strike. Rest/reversal generations, weapon/session/tracking discontinuities and independent dual-wield histories remain authoritative.

The gate runs before native stamina reads/spending and before damage/contact queries. Once qualified, costs remain once per stroke even across multiple target contacts and duplicate stereo renders. Power charging remains skill-gated and one strike per trigger press; the same deliberate-stroke gate applies to charged contact. Existing weak/full damage scaling, native perks and armor remain intact.

## Real native sneak feedback

The SNEAK badge is a small, stable VIEW-space overlay shown only when native gCCharacterMovement_PS::IsSneaking() is true. Native rejection cannot leave a fake toggled VR state. Entering/leaving sneak emits a short left-controller vibration. Menus, radial, unavailable world and animal form suppress this badge. The actual installed IsSneaking helper at Game RVA E82F0 compares the current movement mode to 2. No skill or movement mode is forced by the VR button.

## Sprint speed correction

The live 0.19.0 log reported native_max about 600 units/s, but generated speed about 1350 units/s: the motor multiplied an already resolved value by 1.5 and 1.5 again. In the installed native GetCurrentMaxSpeedOfDir, speed modifier +0x160 and terrain scale +0x380 already contribute to the returned directional maximum. Its native caller at E2E6D uses that resolved result. StepNative now consumes it once; no further mode/speed/terrain multipliers are applied.

For the reported forward sprint this restores about 600 units/s, or 6 m/s at the existing 100 units/metre, instead of 13.5 m/s. The native result still depends on direction, slope, states and modifiers. Native sprint stamina/Sprinter progression remain unchanged. The floor correction, immediate stop/restart and optional starting response remain as accepted. This is correction of duplicated factors, not a new arbitrary speed cap.

## Downed target and finishing animation

Finishing selection now performs its own nearby physics-candidate query, resolving child shapes to NPCs and accepting only downed, living humans/orcs within the existing player reach. It rates their bounds along the calibrated right-controller interaction ray and retains native line of sight. It can select while the melee weapon is drawn, independently of the ordinary native weapon/interaction focus filter. The FINISH badge identifies this eligible candidate before the hold. It takes temporary priority over the SNEAK badge.

Right grip + trigger held 0.8 seconds on the same eligible target requests native PS_Melee_FinishingAttack with the native current target. It retains the original animation, protection, witnesses, quest/death handling and suspension of tracked arms until the native state ends. Drawing a melee weapon is required; bare-handed trigger keeps the usual loot/interact behavior. The request still needs headset confirmation with a real downed NPC; no game session or kill was triggered by this update.

## Validation and return-time probe

63/63 automated tests pass, including 2 cm oscillations with zero stamina calls, the 50 cm threshold, rotation-only/vertical strokes, exact tracking conversion across translated/rotated worlds, duplicate/reset handling, direct Y context, resolved sprint speed, the hash-verified native modifier/terrain/caller bytes, down-facing target bounds and actual D3D11 status uploads/VIEW submission. Fourteen launcher pages pass the capture/layout check. Accepted BowCharge.h, PhysicalBow.h, CrossbowHandling.h, NativeBow.h and NativeTrackedBow.h are byte-identical to 0.19.0.

Check: save/live radial size; Y sneak and the SNEAK badge; normal run versus R3 sprint and stamina; tiny weapon adjustments versus ≥50 cm swings; charged hits; downed target FINISH badge and deliberate finishing hold; ground/slope/room movement and normal exit. These new headset results remain pending. The installer backs up binaries, preserves your current INI exactly and does not launch Gothic 3.
