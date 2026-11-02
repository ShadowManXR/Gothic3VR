# Gothic 3 VR — autonomous batch 0.19.0

Implemented for the hash-verified CP 1.75 / PUP 1.1.1 installation. Win32 Release builds and 62/62 automated tests pass. The user confirmed the 0.18.1 exit fix; the new ground anchoring, gestures and finishing animation still require a headset probe. The installer does not launch the game. Existing calibration and the launcher binary are retained; the installed INI is preserved byte for byte.

## Controls

| Action | Quest Touch control |
| --- | --- |
| Radial | Press L3; select with right stick; A confirms; B cancels; Y changes page |
| Sneak | Radial utility page → Sneak (native sneak toggle and skill restrictions) |
| Walk / Run | Radial utility page → Walk / Run (native slow-walk/normal-run toggle) |
| Sprint | Hold R3 while moving the left stick forward; release to stop sprinting |
| Camera reset | Hold L3 + R3 for 0.5 s, taking priority over radial/sprint |
| Physical melee | Swing either equipped tracked melee weapon through a target |
| Power strike | Release the weapon hand's trigger first, then hold it until a short vibration; strike while holding, or within 0.3 s after releasing |
| Finish a downed NPC | With a melee weapon drawn, point the right controller at the downed human/orc nearby; release grip/trigger, then hold right grip + right trigger for 0.8 s |

Power charge timings are 0.90 / 0.65 / 0.45 seconds for native skill tiers I / II / III. These timings are a VR gesture policy, not a reconstructed native animation formula. The relevant learned skill is required. Charging alone does not deal damage. One trigger press prepares one strike. Unsupported heavy 2H handling cannot prepare a power strike. Left and right weapons use independent trigger/charge/stroke state.

## Ground anchoring

The direct ground motor now calls the same native gCCharacterMovement_PS::PutToGround(false, false) correction that the original controlled-translation generator uses. It restores native collider/pivot height, sensor/root correction, step and ground checks before calculating the VR ground contribution. If the correction changes to falling or another excluded mode, native movement handles that frame. It does not use an estimated headset-to-floor offset or teleport the root to a visual ray hit. Native ceiling/water/gravity, jumping, falling, knockdowns, transformations and navigation retain their own paths. Camera position filtering stays bypassed in direct mode.

## Radial

The panel is 0.66 m square instead of 0.80 m (17.5% smaller), at the existing distance. Brown stone grain, gold frames and cream serif text match the game's palette. The image is generated locally without bundling game textures/fonts. Both pages have ten sectors; quickslots 1–0 are retained, and Sneak and Walk / Run complete the utility page. Native 4K/5K menu capture and pointer mapping are unchanged.

## Stamina and native skills

Physical ordinary swings map to the installed native quick-attack stamina cost: 10 points in classic mode, 12 with Alternative Balancing. This mapping is a deliberate VR adaptation; the native basic animation attack can have a different cost. A charged power stroke costs the native 20 points. The native GetStaminaPoints / AddStaminaPoints script calls own attribute access, clamping and game rules. Costs apply once per meaningful stroke, including misses, not once per contact/target/render frame. If power stamina is insufficient, an affordable ordinary strike may occur; if even its cost is unavailable, that stroke cannot deal physical damage.

1H checks Perk_1H_1/2/3; staffs check Perk_Staff_1/2/3; large weapons, axes and the native Halberd use Perk_Axe_1/2/3. Skill ownership is read, never granted. Equipment/training requirements and native damage/armor/perk calculations remain in the game. Charged contact presents the native PowerAttack action and full physical stroke intensity to those calculations; no invented extra damage multiplier is applied. Native shield parrying and every native combo benefit are not yet transposed to VR.

Sprint presses the user's configured native SessionKey.Walk binding while R3 and forward movement are held. Native sprint speed, stamina drain/regeneration and Sprinter progression own the result; no additional VR stamina drain is imposed. Sneak and Walk / Run similarly use the configured SessionKey.Down and SessionKey.WalkToggle bindings. Menus, tracking/focus loss and reset release the owned input.

## Native finishing kill

The deliberate 0.8-second grip/trigger hold requires the same focused, downed, living human/orc within 1.8 m horizontally and 1.2 m vertically. It sets the native current target and requests PS_Melee_FinishingAttack, whose installed callback dispatches _AI_FinishingAttack. The game owns animation, eligibility/protection, witnesses, quest credit and death. There is no direct Kill() or manual HP zeroing. Tracked arms temporarily yield to the native finishing animation; physical damage and direct controller ground input are suspended until the native state ends, then tracked VR resumes. Not every native protection case has been tested in the headset.

## Return-time probe

1. Load the save, walk/run on flat ground, slopes and steps, and combine physical steps with stick movement. Check root/eye height, walls, falling and immediate stop/restart. Existing eye-height calibration remains yours.
2. Open both radial pages and test Sneak, Walk / Run and R3 sprint; compare stamina and the native learned Sprinter skill without changing the real save's skills.
3. Swing 1H, dual wield and supported 2H with/without trigger charge. Compare light/full/power hits and once-per-stroke stamina; check empty-stamina behavior and native knockdown recovery.
4. Knock down an eligible NPC and use right grip + trigger while focused. Verify native finishing animation and return to tracking. Use an expendable test save for the kill.
5. Recheck accepted bow draw and crossbow reload/support, menus, transformations and normal exit.

Diagnostic markers: vr_ground_motor with floor_delta; vr_melee_stamina with native_attribute_script=1; vr_power_strike_charged; vr_native_finisher_requested. A request log is not proof of a completed native kill.

## Evidence and verification

62 tests cover ground-motor transitions/ABI, the actual hash-verified PutToGround call/prologue/return, stamina instruction bytes, weapon skill-class switch, finisher registration, charge cancellation/deduplication/target holds, Touch haptic binding, controller release/reset conflicts and the existing stereo/hand/ranged/teardown regression suite. The radial utility image was visually inspected. The unchanged launcher retains the previous 13-page layout check. Native binary investigation excerpts accompany the package; they establish installed behavior, not a full runtime headset result.

The accepted BowCharge.h, PhysicalBow.h and CrossbowHandling.h match the accepted 0.17.1 package byte for byte. NativeBow.h and NativeTrackedBow.h match 0.18.1. No bow draw-speed, projectile, grip or reload policy was changed in this batch.
