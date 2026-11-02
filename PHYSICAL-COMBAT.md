# Weighted physical weapons —0.22.9

Hands/weapon move as one assembly; supported weapons share one filtered frame. Physical stroke orientation/travel follows the displayed grip; skill/stamina and threshold rules remain. 74/74 tests pass; headset acceptance pending. See FIXES-0.22.9.md.

## Previous release notes

# Consistent supported pose — 0.22.6

Current held pose is captured in world and tracking spaces for coherent melee rendering/combat. Existing damage/skills/stamina/weight remain. 71/71 tests pass; live acceptance pending. See FIXES-0.22.6.md.

## Previous release notes

# Native weapon selection restored — 0.22.5

0.22.4 weapon entry hook and equipment suspension removed. Already armed Sneak/Murder and damage/skill/stamina rules remain; radial no longer leaves selected Sneak to select a weapon. 71/71 tests pass; headset acceptance pending. See FIXES-0.22.5.md.

## Previous release notes

# Visible equipped weapons — 0.22.4

Sprint/jump first-person transparency is corrected during stereo rendering; weapons still use native damage/skill/stamina. Weapon selection temporarily suspends Sneak and restores it after native equip settles. 71/71 tests and15 launcher pages pass; live acceptance pending. See FIXES-0.22.4.md.

## Previous release notes

# Armed Sneak/Murder and extended velocity — 0.22.3

Y toggles native Sneak stance with melee equipped. Murder keeps the learned perk, charged coherent stab and native victim awareness/outcome checks. Swing/stab speed thresholds are independent and now reach 800 cm/s. See FIXES-0.22.3.md.

## Previous release notes

# Independent stab distance and speed — 0.22.2

Each melee profile has separate Minimum stab speed (50 cm/s default, 40–400, step 10) and Stab distance minimum (15 cm default, 5–100, step 5). Swing speed remains independent. Both damage/stamina require a coherent qualified physical gesture. Save settings applies live. See FIXES-0.22.2.md.

## Previous release notes

# Independent stab distance — 0.22.1

Stab distance minimum: default15cm, range5–100cm, step5. Applies to both damage and stamina for a fast coherent forward thrust; long swing thresholds remain. Minimum speed and native skill/cost/damage policy still apply. See FIXES-0.22.1.md.

## Previous release notes

# Swing velocity — 0.22.0

Minimum swing speed is independent for 1H, 2H and Staff/Halberd. Default 100 cm/s; range 50–400 in 10 cm/s steps. Damage and stamina require the configured distance at qualifying physical speed; slow movement and small tremor cannot debit or damage. Native costs, damage and learned skills remain. See BATCH-0.22.0.md.

## Previous release notes

# Rigid grips — 0.21.1

Native animation cannot recalibrate a held melee attachment. Weight rotates the whole visible2H hand/weapon, and supported staff binds both palms to the common frame. Damage/stamina/skill rules remain unchanged. See FIXES-0.21.1.md.

## Previous release notes

# Native defence / stealth — 0.21.0

Left trigger requests native shield/1H/2H parry outside dual wield. No physical swing damage/stamina while blocking. Learned Murder uses native Sneak + charged 1H blade thrust for native Pierce; Knock Out keeps native charged blunt Power. Native victim/protection/witness rules and configured swing gates remain. See BATCH-0.21.0.md.

## Previous release notes

# Finishing focus — 0.20.2

Select a downed living human/orc from native interaction ROI, not only dynamic colliders, using the controller ray and actual grounded-body sight. FINISH remains selection feedback; release controls, then hold right grip + trigger 0.8 seconds for the same eligible target and native animation. Swing gates are unchanged. See FIXES-0.20.2.md.

## Previous release notes

# Current swing gates — 0.20.0

1H, 2H and Staff/Halberd each expose independent damage and stamina travel minima (10–200 cm, 10 cm steps, default 50). Tiny adjustments below the damage gate cannot damage; stamina uses its separate gate. Tracking-space travel plus coherent excursion rejects accumulated tremor and world motion. Lower damage than stamina deliberately permits an affordable hit before the debit threshold; equal settings align both gates. See BATCH-0.20.0.md.

## Previous release notes

# Minimum physical stroke — 0.19.1

Weapon endpoints must travel at least 50 cm in XR space, with a coherent 35 cm excursion, before contact or stamina processing. Small repeated tremors, locomotion and camera/body motion cannot pay stamina or deal damage. Existing stroke generations/per-target budgets and charged power strikes remain; deliberate finishing now has its own downed-target selection and FINISH badge. See FIXES-0.19.1.md.

## Previous release notes

# Current physical combat — 0.19.0

Meaningful strokes now pay native quick-attack stamina (10 classic / 12 Alternative Balancing) once per stroke, including misses. Trigger-held, skill-dependent charge prepares one native PowerAttack-classified stroke at 20 stamina; actual physical contact remains required. Costs use native attribute scripts and full native damage/perks remain available. The quick-attack mapping and charge timing are explicit VR policies. Right grip + trigger held on a downed focused human/orc requests native finishing animation. Shields/parries and all native combos remain incomplete. See BATCH-0.19.0.md; new gestures await headset validation.

## Earlier release notes

# Physical melee combat — 0.11.7


The user confirmed the 0.14.0 headset probe: both dual wield 1H weapons remain visible and physical combat works with both hands. This records the tested setup; it does not validate every weapon template or physical parrying.


0.14.0 adds dual wield for two distinct drawn 1H entities. Left and right hands have separate sample consumption, swing energy, rest/reversal state and once-per-target stroke hit sets. Both may hit the same target on the same tracking frame; each contact uses the actual hand's weapon as inflictor and its own native damage maximum. Queries and damage remain sequential on the application thread with one shared exact-contact damage hook. Tracking loss or changing the left weapon resets only the left channel. Menus/recovery/recentring suspend both. The native held slots are used directly, not equipped/sheath fallback entities. Physical parrying is still not implemented. Dual wield visibility and combat were accepted in the 0.14.0 headset probe; see DUAL-WIELD.md.



The user confirmed 0.10.2 one-handed sword combat functional and usable. The launcher tab is now 1H Melee Weapon, with Weapon pitch/yaw/roll; original INI keys remain compatible. Two-handed weapons and other melee meshes still require individual validation. Bow drawing/release is implemented as a separate prototype with palette and nocking corrections in 0.11.7; see PHYSICAL-BOW.md. Earlier investigation notes follow.



The user confirmed recovery and device calibration in 0.10.1, then reported missed close contacts and vertical swings with the sword held upright. 0.10.2 adds a forward sword grip (hand and weapon together), independent Sword pitch/yaw/roll settings, accurate native capsule overlaps and angular reversal rearming. Headset acceptance for these corrections remains pending.



The native engine's hash-verified CollectEntitiesInCapsule (RVA 0x174910) uses the capsule's Y orientation row, half-height and radius; it converts endpoints/radius to PhysX metres. The call uses accurate overlap (first boolean false), dynamic shapes, player exclusion, an output entity array, collect-all (sixth boolean false), no property filter and the default group mask. A 2 cm radius capsule follows the measured blade segment at interpolated poses with 3.5 cm sampling, bounded to 25 queries per moving frame. Returned physics entities resolve to live NPC damage receivers; this covers an already overlapping blade whose rays start inside the target. It is not a broad focus/distance hit. Closest-point witnesses on the actual blade are checked against obstacles with rays in both directions; static world geometry, other creatures and once-per-target/stroke suppression remain active. Swept rays still supplement overlap for fast crossings. Live logs distinguish source=ray/capsule and report overlap query/contact totals. No claim of live close-contact acceptance is made by these geometry tests.



Stroke reversal also tracks signed angular delta in fixed tracking space, so opposite wrist pitch/yaw/roll can rearm without translating the controller. This uses the existing speed/gesture/rest thresholds and 180 ms reversal guard. New tests exercise pure wrist reversals on each rotational axis, translated swings on each positional axis, capsule endpoints in all orientations, enclosed contacts, and forward weapon axes across differing native attachment/hand frames. 40/40 tests pass; all six launcher pages passed layout verification.



0.10.0 was accepted for physical contact but exposed a native damage recalculation, a recovery-input block and hand displacement. 0.10.1 corrects these paths and adds independent device calibration; headset confirmation is pending. Native SitKnockDown, LieKnockDown, LieKnockOut and GetUpAttack/GetUpParade states preserve trigger inputs and native arm/body animation. Physical melee, calibrated render overrides and room transfers pause during recovery and resume afterward. See HAND-CALIBRATION.md.



With VR, Tracked arms and a drawn one/two-handed melee weapon, right-hand movement drives contacts and damage. Use1/Use2 from controllers, keyboard or mouse no longer start native melee attack animations. Menu triggers, radial selection, unarmed object interaction and ranged inputs retain their bindings. Sheathe the sword to interact with objects. Bow string drawing and physical arrow release are still pending.



The cutting segment comes from the equipped entity's local mesh bounds and the same final weapon transform used by the tracked-arm renderer. Rendering only publishes a sample; collision queries and damage run after native OnProcess on the application thread, once per controller sample. Both eyes restore native hand palettes/weapon matrices. Physics traces cover points along the moving blade and intermediate blade segments; closest world geometry occludes targets, and a second hand-to-contact trace rejects blades protruding through a wall. The prototype uses sampled line traces, not a volumetric sword collider.



Swing intensity uses controller translation relative to HMD translation, plus actual controller rotation. Raw controller motion gates translation so head-only movement cannot attack. Joystick/body movement does not add swing energy. Activation requires 0.35 m/s effective motion and at least 3 cm of travel or 8 degrees of rotation. Smaller movements are ignored. Damage increases with accumulated travel/rotation, reaching full intensity around 35 cm of hand travel or 86 degrees of rotation. Each target is hit once per stroke; rest or a deliberate reversal rearms the next stroke. Tracking loss, menu/radial, pause, recenter, session/weapon change and large jumps discard the stroke. New tracking/equipment settles for 300 ms.



`Entity(target).DoDamage(Entity(weapon), weaponMaximum, weaponDamageType)` enters the real weapon damage path. The weapon remains the inflictor, allowing scripts to resolve its owner. Offline inspection verified `Entity::DoDamage` -> `gCEntity::DoDamage` -> `DoLogicalDamage` -> `AssessHit` -> `gCEntity::OnDamage`. OnDamage invokes AssessHit again, overwriting the earlier amount. The verified Script_Game ChangeHitPoints entry (RVA 0x35b50) therefore intercepts the final signed HP delta before the native HP setter and subsequent death checks. Only the current physical contact's exact receiver and application thread are scaled; positive healing, NPC hits and unrelated targets pass unchanged. Native post-armor damage becomes `floor(nativeDamage * swingIntensity) in 0.16.1 (the earlier weapon-base clamp was removed because it suppressed native attribute/perk bonuses)`, with a cumulative budget based on the first final native damage result for the contact, rather than the weapon base stat. Native bonuses therefore remain effective at full intensity; duplicate HP callbacks cannot exceed that native contact budget. The native function still performs HP mutation, clamping and subsequent checks. Slot 96 also wraps the damage event; its value alone is insufficient to cap HP damage. Logs include actual HP before/after and hp_callback. No synthetic click or forced attack animation is used.



This first prototype does not spend extra attacker stamina, implement physical parrying, strikes against scenery, or finishing moves against downed humans. Damage scripts may reject a contact for native game-state reasons. Live NPC/animal reactions, kill credit and practical sensitivity require headset validation. Logs report blade geometry, stroke speed/intensity, native/submitted damage and before/after HP; no live acceptance is claimed by automated tests.



Game.dll SHA256: `4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00`.

Script.dll: `03C483148D0EA88E007D2F18EAB30400A6012439EBBE4FEA6EBF837AA2ABD981`.

Engine.dll: `991CF674A9CF1BC60CDDC204BE0993E8251336F4D3471543A37846CC6840BF5A`.

Script_Game.dll: `4D29189281EC26EAC7C704FA67ED323DF5C090D50B068626CF600AECF4262A24`.

The hooks require these module files, the exported SDK virtual slot and the six complete position-independent prologue bytes at the verified native HP entry. Unsupported profiles retain native combat and report the refusal.



Tests cover light/full/capped and resisted damage, native callback Win32 ABI and passthrough, swing deduplication, rest, jitter, movement, tracking changes and contacts crossing entirely between two frames. Headset trial: equip a sword, test a short and a broad swing against the same target, hold the blade still against it, then verify menu clicks, radial changes and recentring without phantom hits.

