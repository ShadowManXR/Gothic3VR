# Gothic 3 VR 0.20.0 — movement, controller sneak and configurable swings

## Controls

| Action | Control |
| --- | --- |
| Normal movement | Left joystick: Run; Walk is removed from controller locomotion |
| Sprint | Hold R3 while pushing the left stick forward; native stamina/skills apply |
| Sneak | Hold left grip and move, with the left hand free and native Sneak learned; release grip to exit |
| Main menu / Back | Y |
| Radial | L3 opens; right joystick selects; A confirms; B cancels; Y changes page |
| Camera reset | Hold L3 + R3 for 0.5 seconds |
| Animal return | X + Y |
| Power strike | Hold the equipped melee hand trigger until ready feedback, then physically strike |
| Finishing kill | Melee drawn: point at a downed human/orc until FINISH appears, release buttons, then hold right grip + trigger for 0.8 seconds |

Left grip near a two-handed weapon or crossbow handle keeps its support function. Sneak waits 120 ms before activation so support can attach first; holding grip then retains Sneak while stationary. The native learned Sneak perks are checked. Missing skill shows NO SNEAK SKILL instead of forcing progression. The SNEAK badge reflects actual native movement mode; native entry/exit requests a 160 ms left-controller vibration. Delivery results are logged in controller_haptic; focus loss cannot be reported as successful vibration. Actual headset feedback remains to be tested.

## Movement changes

The direction now uses the full rendered VR camera basis plus relative headset orientation, shared by the native wished direction and the ground motor. Native Euler yaw alone folds beyond 90 degrees and could invert left/right when other rotation components were discarded. The new heading retains the full basis and the analog joystick direction instead of reducing it to eight digital directions. Neutral controller ownership continues through ordinary render delays, with zero movement, preventing animation-driven motion from taking over at release. The native movement input hook writes the SDK-owned wished direction after normal input processing; direct VR mode avoids steering that camera a second time.

Normal ground movement selects Run and blocks the Walk toggle in the owned VR context. Sprint is retained only when the native game approves it. GetCurrentMaxSpeedOfDir is consumed once, retaining the previous correction for duplicate sprint multipliers. Native collisions, PutToGround, slope limits, jumps, falls, wading, knockdowns, animals and scripted activities retain their native handling. The accepted floor correction is preserved. No new camera smoothing filter was added.

These changes are active with Direct VR locomotion / stable locomotion enabled in the current installation. User settings, including Movement start response = 20 ms and Instant turn response = Off, are preserved. The latter remains configurable in Motion. Direction and stop/restart tests pass, but comfort, responsiveness and all native animation transitions require a headset test.

## Damage and stamina sliders

Each launcher page — 1H Melee Weapon, 2H Melee and Staff / Halberd — now has independent Damage swing minimum and Stamina swing minimum sliders. Range: 10–200 cm, in 10 cm steps; default: 50 cm each. Both 1H hands use the 1H values. Save settings applies live. Slider positions and saved numeric values snap to the step.

The gate measures actual blade endpoint travel in XR tracking space, cancelling world movement and native body/camera motion. It also requires a coherent excursion of at least 70% of the chosen minimum, so repeated small tremors cannot accumulate a deliberate strike. Below the damage minimum there is no damage/contact processing. Below the stamina minimum there is no stamina debit. Native damage, armor, perk checks, power strike rules, per-target deduplication and once-per-stroke cost remain.

The two thresholds deliberately operate independently: setting damage below stamina allows an affordable hit before the stamina threshold is reached; the debit occurs only if that stroke later reaches the stamina minimum. Setting stamina below damage spends stamina earlier in a sufficiently large stroke, before a damage-qualified contact. Equal values keep the gates aligned. Native affordability is still checked before allowing damage. Tiny adjustments under the configured damage gate do no damage, not merely zero stamina.

## Activities use third-person

Native object activities such as campfire/cooking, saw, smithing, grinding, alchemy, seated activities and sleeping use their native third-person camera. Routine action, animation and interaction task/state identify an active activity; simply pointing at a workstation does not change camera. VR locomotion and tracked arms yield to the activity's native animation. On exit, automatic first-person recenters while preserving the human eye-height anchor. Dialogue and animal third-person behavior remain. Ordinary item pickup, weapon equip, magic and knockdowns do not count as workstation activities.

## Validation and installation

64/64 automated tests pass. They include all view quadrants and folded native camera representations, continuous joystick heading, neutral ownership/restart, run-only ground modes, native sprint eligibility, Sneak skill/support/context guards, activity camera transitions, independently stepped settings, deliberate damage/stamina gates and OpenXR haptic focus-result handling. Existing native binary, rendering, collision, combat and recovery checks remain green. Fourteen launcher pages pass their layout/capture check; the three melee slider pages, Motion and Radial were visually inspected. Automated tests do not confirm headset comfort or physical vibration delivery.

BowCharge.h, PhysicalBow.h, CrossbowHandling.h, NativeBow.h and NativeTrackedBow.h are byte-identical to 0.19.1. The finalized bow policy is unchanged. Native game module hashes are verified and no native game assets are bundled.

Installation backs up binaries, preserves the existing plugin INI byte-for-byte and updates the game launcher and standard development launcher entry point. Gothic 3 is not started unattended.

Return-time probe: walk left/right and diagonally while facing different bearings; stop and immediately reverse; compare Run and native R3 Sprint; try left-grip Sneak with a free hand, badge/vibration and Y menu; verify support grip does not Sneak; adjust each weapon's damage/stamina sliders and compare tiny versus deliberate strokes; use a campfire, saw, forge and bed and check first-person restoration; check floor, physical steps and normal exit.
