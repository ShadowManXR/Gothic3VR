# Loaded and launched bolt visibility — 0.22.7

One bounded per-eye pose/visibility owner for nocked ammo; native reload, ammo consumption, aiming and special bolt behavior retained. 72/72 tests pass; headset acceptance pending. See FIXES-0.22.7.md.

## Previous release notes

# Current ranged recovery — 0.20.2

A shared bow-update fault no longer permanently removes crossbow loading and reticle. Current native weapon context recovers after a bounded fault delay; a different weapon/session does not inherit the latch. Trigger release is required after recovery. Native reload duration, skills, ammunition and projectile stats remain unchanged. See FIXES-0.20.2.md.

## Previous release notes

# Crossbow render follow-up — 0.18.0

Support contact is prepared once before both eyes, and crossbow hand/bolt palettes are reapplied even when native animation reports a cache hit. A brief pose gap cancels trigger input without unnocking an already owned, matching loaded bolt. Weapon/context changes still restore it normally. Accepted guidance, grip, unsupported sway and native firing/reload progression are preserved. See FIXES-0.18.0.md; the flicker correction awaits a headset test.

## Earlier crossbow behavior

# Crossbow — 0.16.4

0.16.2 fixes stale ranged samples across save/camera transitions and replaces the shared permanent bow/crossbow render fault latch with bounded automatic recovery. Model caches follow native asset names; replacement actors never receive old palette snapshots. Calibration and native firing/progression are preserved. See FIXES-0.16.2.md. 52/52 automated tests pass; the reported older-save/quick-slot sequence awaits headset validation. Previous release notes below retain their original validation scope.

The crossbow is held and aimed with the right controller. Press the right trigger to shoot a bolt. Holding the trigger does not repeat fire. Release it before the next shot. Reloading follows the selected native hero Cock animation duration divided by the active native crossbow perk speed multiplier. The verified installation uses base speed 1, Crossbow II speed 1.5, and Crossbow III speed 2, with the highest active tier taking priority. The logged hero alias is Hero_Stand_CrossBow_Bolt_P0_Cock_Begin_O_Fwd_00_; its actual compiled resource appends %_00_P0_0.xmot and measures 2.8 seconds. Its resulting intervals are approximately 2.8 / 1.867 / 1.4 seconds. Loose/patched resources are read through the native VFS. The 1.8-second Mike animation tested in 0.16.0 was a different motion, and its direct-prefix lookup prevented shooting. These corrected values were checked offline, not timed in a headset. The duration parser accepts both LR quaternion and LP vector tracks stored under native chunk ID 2. There is no manual reload interval setting; a legacy CrossbowReloadMs key is ignored. Reload uses scaled game time, pauses with the game, and cannot be cleared by menus/recentering. A reload starts only after a successful native shot. Unknown or unreadable native motion data refuses physical fire and logs the reason instead of inventing a delay. Reloading remains automatic rather than a physical cocking gesture.

Bring the left palm to the stock's foregrip and hold left grip to attach it. Release grip to free the hand. With support attached, the left hand position guides the barrel relative to the rear right hand, and the left wrist supplies roll; right wrist rotation does not drive the supported aim. Supported aim uses the shared tracking-space stabilization. Without support the right controller guides the crossbow with visible sway (approximately up to 4.8 degrees yaw / 3.8 degrees pitch); gun, reticle and bolt share this direction. The handling penalty does not alter native damage, velocity or reload. See FIXES-0.16.4.md. Loss of left-controller tracking does not disable right-handed aiming or fire.

Crossbow pitch/yaw/roll and three palm contact offsets are independent of the bow and hand calibration. They move the weapon without twisting the tracked palm. Save settings applies live. The existing Bow aim stabilization value and the shared Ranged aiming reticle checkbox apply to both ranged weapons. Reticle visibility does not control firing.

The actor and Cock motion are read from the installed game through its virtual filesystem. The model's native -Z bore, Bone04/Bone05 string centre, fully cocked nock and rigid stock foregrip define the mesh, bolt and support contact. Spatial transforms are applied before frustum culling; animated world palettes and local transforms are restored per eye. No game assets are packaged.

Native inventory slots remain unchanged (left-held ranged weapon, right-held ammunition). Only the VR render pose is mapped to the right palm. The verified native Shoot callback clones the current held Bolt template, assigns its owner, keeps weapon/ammo damage and effects, consumes one bolt, and supplies the next native ammunition stack. The projectile hook gives it the same evaluated nock and stabilized bore direction as the reticle. Crossbow shots use full native velocity and damage. Menus, radial selection, recovery, stale poses, recentering or lost right tracking cancel input; reopening while holding the trigger cannot shoot automatically. Unknown model geometry refuses physical firing.

Validation: the user accepted crossbow firing, right-hand pose and optional support in 0.15.0. The 0.16.0 batch passes 51 automated tests, including native animation parsing, perk priority/constants, successful-shot reload commitment, paused clock, trigger edges, ranged geometry, magic imports, arm reach and existing regressions. Both actual compiled hero Cock resources (with Bolt and with None) passed the duration parser at 2.8 seconds. A synthetic mixed LR/LP regression catches the old parser failure. All 12 launcher pages passed layout checks. The changed runtime VFS motion lookup and perk-dependent shot timing still require headset validation.

Probe: equip the crossbow from the radial; aim with the right hand, shoot at least six deliberate presses, and confirm one bolt per accepted shot and that bolts reach the reticle's direction. Test optional left support through walking and rotation, menus/recenter with trigger held, switching to bow and dual wield, and the shared reticle On/Off. Calibration values are preserved on installation.
