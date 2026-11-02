# Native Sprint permission retained — 0.22.5

Sprint input waits until native movement is ready, avoiding native idle Walk. Only wished Walk3 is normalized to Run4; no forced Sprint5 or stamina/skill bypass. Native weapon selection/refusal restored. 71/71 tests pass; headset acceptance pending. See FIXES-0.22.5.md.

## Previous release notes

# Native equip and Sprint — 0.22.4

Sprint uses native input, speed/stamina and skill checks. Native OnPlayerSelectWeapon still owns inventory and equip tasks; only the owned VR Sneak refusal is bridged temporarily. No progression values changed. 71/71 tests and15 launcher pages pass; live acceptance pending. See FIXES-0.22.4.md.

## Previous release notes

# Armed native Sneak and lockpick input — 0.22.3

Sneak executes native Down input and uses actual AniState3; armed Parade routing is replaced only for an owned VR player request. Murder qualification accepts actual stance3, retaining perk/charge/thrust/native outcome restrictions. Lockpick latch changes input delivery only; no sequence/status/cost/reward changes. See FIXES-0.22.3.md.

## Previous release notes

# Native input and independent stab speed — 0.22.2

Sneak now executes native OnAction/OnPlayerGameKeyPressed rather than authoring mode 2. Native skill checks and perception remain. Object interaction A/D does not alter locks/sequence/costs. Separate stab velocity preserves native melee costs, damage and learned skill qualification. See FIXES-0.22.2.md.

## Previous release notes

# Stab and lockpick input — 0.22.1

Short stabs use independent physical distance while retaining speed/native stamina budget, damage and learned-skill conditions. Murder retains its original charged15cm sneaking blade qualification. A/D puzzle input changes no native lockpick sequence, skill checks, consumption or unlock state. See FIXES-0.22.1.md.

## Previous release notes

# Input and velocity gates — 0.22.0

Lockpick forwards directional input without changing native difficulty, skill checks or lockpick consumption. Physical melee adds a tracking-space speed qualification before native debit/contact; native cost, power, Murder/KnockOut and finishing rules remain. Ranged policy headers are unchanged. Real puzzle acceptance and animation need headset verification. See BATCH-0.22.0.md.

## Previous release notes

# Additional native skills — 0.21.0

Lockpicking keeps native tiers/items/consumption; Prospecting keeps Perk_Mining double yield. Murder needs native Perk_Assassin/Sneak/qualified blade Pierce; Knock Out needs Perk_KnockOut/charged Impact Power. Native victim/FOV/protection/outcome remains. Shield skill tiers are not replaced or granted. See BATCH-0.21.0.md.

## Previous release notes

# Native locomotion mode selection — 0.20.1

Native Run/Sneak wishlist is selected before original movement processing. Sneak learned perks remain required. Sprint uses the configured native Shift action and never forces Fast, speed or stamina. Ranged/combat progression remains unchanged. See FIXES-0.20.1.md.

## Previous release notes

# Current native skill and cost handling — 0.20.0

Sneak checks native Perk_Sneak_1 or Perk_Sneak_2. Sprint mode and stamina remain native. Melee damage/stamina gates are independently configurable; native affordability, costs, power skill checks, armor and deduplication remain. Finalized bow policy is unchanged. See BATCH-0.20.0.md.

## Previous release notes

# 0.19.1 input and stamina follow-up

The minimum 50 cm weapon-path gate now precedes both native stamina and contact handling, so small hand adjustments spend nothing. Native sneak/sprint bindings and learned skills remain; native resolved speed is consumed only once, correcting duplicated sprint multipliers. Native finisher selection now works independently of weapon focus and retains the original finishing state. Bow policy and native ammo/skills are unchanged. See FIXES-0.19.1.md.

## Previous release notes

# Current progression additions — 0.19.0

The accepted bow policy is unchanged. VR melee now maps ordinary strokes to native quick-attack cost (10 classic / 12 AB) and charged strokes to native power cost (20), through GetStaminaPoints/AddStaminaPoints. Charge time 0.90/0.65/0.45 s is a VR policy gated by actual learned Perk_1H, Perk_Staff or Perk_Axe tiers; native Halberd belongs to Perk_Axe despite its render profile. Power contact uses the native PowerAttack action, with no extra invented damage multiplier. Native sprint stamina and Sprinter progression are activated by the configured native binding. Native finisher state owns its kill animation/protection/witness/quest effects. Shield/parry and full native combo parity remain incomplete. The historical gap table below predates these additions; see BATCH-0.19.0.md for current costs, controls, evidence and limitations.

## Earlier release notes

# Native skill progression audit — 0.16.3

Primary evidence is the installed CP/PUP game, its SDK bindings and the current native modules, not the pasted generic skill/pricing list. progression-audit-0.16.1.json indexes all 73 Perk identifiers found in Script_Game.dll with code/data references and module hashes. This is an offline audit of the mod's affected paths, not a headset proof of every perk. No skill is taught, removed or re-priced; equipment/stat/training requirements are native.

| Group | Native behavior and VR handling |
| --- | --- |
| Bow tiers, animal/large-game/orc hunting bonuses | The native GetPlayerSkillDamageBonus handler checks Perk_Bow_2/3 and Hunter/OrcHunter against the native owner and target. Both Bow tier bonus branches add their native 25 points when active. VR still calls native Shoot, clones the actual held ammo, retains its owner/template/effects and lets native collision/damage resolve perks, dexterity and armour. 0.16.3 initializes each release using the installed PowerAim charge curve (0.2 + 0.8 * charge) and Shoot velocity (200 * that charge), independent of the held arrow's old multiplier. Full charge preserves full native damage. Charge is limited by physical draw distance plus readiness from the fixed native 1.95 s duration. Requested VR draw-speed progression adds explicit 1 / 1.5 / 2 factors for base / Bow II / Bow III: this is a VR policy, not a verified native tier speed formula. Native damage bonuses are still calculated by the game. |
| Crossbow tiers | Same native damage/attributes/ammo path. Native Cock action speed checks Crossbow III before II (2 / 1.5 / base 1). Timing now reads the actual compiled selected hero resource: 2.8 s here, not the 1.8 s Mike fixture. No user reload timer or instant reset/recenter reload. |
| 1H, dual wield, large weapons, staffs/halberds | The real equipped weapon is the damage inflictor. Native DoLogicalDamage/AssessHit/OnDamage calculate owner attributes, active combat perks, item upgrades, weakness, target protection and race-specific bonuses. The previous VR min(nativeDamage, weaponBase) cap clipped legitimate bonuses; 0.16.1 scales the final native result directly. A full stroke keeps all native damage, a light stroke scales it, and deliberately unsupported heavy 2H applies the existing 20% VR modifier. Per-contact target deduplication and a native-result HP budget prevent repeated damage callbacks. |
| Magic schools, staff magic and druid transformation | Native Cast/QuickCast/PowerCast keep requirements, mana, scroll use, charge/reload, damage/effects, targeting and transform duration. Controller aiming changes only the fresh matching owned projectile launch pose/direction. The native staff/spell setup determines available casts. Animal third-person is a camera policy; it does not alter species, status bits or the druid duration. |
| Smithing, sharpening, poison/ammo crafting, alchemy, thieves, trophies, trade, permanent attributes, regeneration/resistances | Training, item/stat updates, recipes, harvesting, timers and native effects are not replaced. Native held-ammo templates carry special projectile effects. Native item/attribute/target contributions now survive the corrected melee scaling. Special ammunition combinations still need in-game probes. |
| Shields, native animation unlocks and attack stamina | These remain explicit VR parity gaps. Physical shield blocking/parrying, native combo/finisher gestures and physical melee attack-stamina consumption are not fully implemented. Learned skill state and native calculations are preserved, but this release cannot claim that every animation/block/stamina benefit has an equivalent physical VR action. Existing native movement stamina, healing and mana paths are untouched. |

Verification: 54 automated checks pass, including recalculated damage greater than weapon base through the actual Win32 hook ABI, integer extremes, healing/NPC/thread passthrough, independent hand strokes, native perk branches, mixed native motion tracks and alias matching. Both real compiled hero Cock variants parse at 2.8 seconds offline. Headset regression should compare the same weapon/target with known existing native bonuses and full vs light strokes, along with bow/crossbow perks and special ammo. Do not change your real save's skills for this audit.

The pasted skill descriptions include claims that are not established by the installed native branches. This mod keeps those native branches rather than reproducing unverified gold/LP prices, tier bonuses or animation-speed formulas.


## 0.16.4 handling follow-up

Bow visible draw reflects the existing effective charge and tier readiness; no new velocity/damage/skill multiplier is added. Crossbow one-hand sway and left-guided supported aim are explicit VR handling adaptations, independent of native reload progression. X+Y calls native EndTransform for manual human return; the native implementation owns restoration, and automatic transformation duration is unchanged. Magic grip correction is render-only. See FIXES-0.16.4.md.

## 0.17.1 bow handling update

User requested 40% above original VR draw speeds for base/middle tiers (1.4 / 2.1), and unrestricted physical draw at Master Archer. The last tier has no readiness/hold/repeat timer; physical draw still sets charge, projectile speed and scaled damage. Native skill damage bonuses, ownership and ammunition remain native. Crossbow reload progression is unchanged. See FIXES-0.17.1.md for the current policy and its regression validation.

