# Gothic 3 VR 0.21.0 — shields, stereo, movies and native skills

## Shields and parry

Equip the shield and draw the native 1H weapon. The shield follows the calibrated left palm, with a cached native attachment rotation and back-centre contact based on the model's thin axis. Its pose and hand palette prepare before culling for both eyes, then restore to native values. Inventory and ownership are untouched.

Launcher **Shield** has independent live pitch/yaw/roll and Grip toward fingers / palm depth / across palm. Save settings applies these without rotating the hand. Defaults are zero; final alignment for different shield models needs headset confirmation.

**Hold left trigger for native shield block, or native 1H/2H weapon parry without a second 1H weapon.** Native defence timing, facing, attack types, stamina and Shield Parry requirements/tier effects remain in the native scripts. Moving the shield alone does not guarantee a block: this version requests the native defensive state with a button, rather than adding automatic physical collision blocking. No skills are granted and no damage is unconditionally cancelled.

Native Use2 also makes a quick attack on a short click. Physical melee now permits native parry but its release-only message copy prevents that short-click attack. Held messages preserve actual duration and native wind-up. Physical swing damage/stamina stop while requesting/performing parry. Dual wield keeps left-trigger power charging; bow/crossbow/magic and menus retain their controls.

## Both-eye 2H rendering

2H swords/axes, staff/halberd and shield rigs/meshes prepare once before both eye frusta. Both eyes use an owned actor/factory/count snapshot, including native palette cache hits. This removes per-eye support solving and late mesh placement. Native palettes/transforms/render flags are restored. Actual intermittent visibility still needs a headset check.

## Movies / cinematics

Hash-guarded Engine PlayVideo keeps native Bink decoding/audio/timing. During playback, D3D9 Present/PresentEx copies the completed backbuffer on the GPU and submits an opaque, aspect-preserving VIEW-space panel to both eyes. It no longer depends on native GUI drawing or UI alpha. World input, camera/hand overrides and combat pause until native playback returns. Y sends native Escape to skip after a neutral sample.

Capture failures preserve native playback; native errors/return remain native. Logs: vr_movie_begin, vr_movie_frame, vr_movie_end. Movies played before the scripting plugin loads are outside this bridge. Actual game movie playback in Quest is not yet verified. Dialogue and activities keep existing native third-person handling.

## Native skills

| Skill | Operation / native rules |
|---|---|
| Lockpicking | Holster if needed and use native chest interaction. Perk_LockPick1/2/3, difficulty, required items, consumption and result remain native. |
| Prospecting | Native mining activity; internal Perk_Mining doubles yield in the original script. Rewards/requirements are unchanged. |
| Murder | Learned Perk_Assassin + native Sneak + charged 1H blade forward thrust request native PierceAttack11. Native unaware-victim/FOV, protection, witnesses and death decide the result. No direct kill. |
| Knock Out | Learned Perk_KnockOut + charged blunt/Impact hit uses native PowerAttack2. Native unaware-victim checks and knocked-down outcome remain. |

Murder previously could not enter its branch because physical hits requested only Attack/Power. A coherent forward handle stroke now qualifies Pierce; wrist rotation or accumulated tremor do not. Existing configured minimum swing travel applies. Native skill evidence and module hashes are packaged. Automated gesture checks do not prove a live stealth kill.

## Verification / return-time probe

64/64 tests pass: real GPU movie copy/native state, opaque both-eye movie layer, native PlayVideo x86 ABI, shared palette owner checks, shield contact/calibration, parry release/native binary layout/skill checks and stealth thrust qualification. Fifteen launcher pages pass layout/capture; Shield is visually inspected. Actual headset behavior awaits testing; Gothic 3 was not launched unattended.

Finalized BowCharge.h, PhysicalBow.h, CrossbowHandling.h, NativeBow.h and NativeTrackedBow.h are byte-identical to 0.20.2. Native locomotion remains; no direct VR motor returns. FINISH selection/ranged recovery from FIXES-0.20.2.md are included. Installer preserves the current user INI byte-for-byte; missing Shield keys use defaults until Save settings writes them.

Test shield palm alignment and held/released left-trigger defence against native attacks; compare stamina/skills, 1H without shield, 2H parry and dual-wield charging. Check supported 2H per eye, native movie/Y-skip/world return, native chest/mining and learned/unlearned stealth skills. Recheck crossbow reload/reticle, settled FINISH victim and normal exit.
