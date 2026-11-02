# Union+ 1.7.3: VR mod compatibility

Union+ 1.7.3 - VR compatibility notes (local audit, 2026-10-25).
192 folders reviewed: descriptions, Mod.ini dependencies and payload/configuration names. This is a static compatibility review, not 192 headset tests or a decompilation of every third-party DLL. Recommendations below are based on documented changes and overlap with VR features.
Avoid = direct camera/time-speed overlap. Baseline = leave off until the overlapping player/combat system is tested. Check = validate the stated feature, not a confirmed incompatibility. Performance = optional cost warning only. Incomplete = documentation/partial folder, not a complete installable mod. No specific conflict = no issue identified, not a guarantee.
Do not remove mandatory dependencies or compatibility patches individually. Deselect the parent pack instead. Union+ remains responsible for mod-to-mod exclusions. These notes do not install, disable or remove anything.


Source: the `Mods` folder in the supported Union+ 1.7.3 Nexus package. No installed mods or engine settings were changed during the review.

**Important distinctions:** Intuitive Combat System is not ICS Classic Camera. Monsters Unleashed is an enemy-animation pack; its explicit conflict is with CombatAnimationOverhaul_2H, not with all of VR. No Feedback Effect removes a red screen effect. Quick Loot provider selection is already handled by the protected Union+ workflow.

Counts: Avoid: 2, Baseline: 44, Check: 40, Performance: 21, Incomplete: 7, No specific conflict: 78.

## Dependency traps

- `Balance Ultimate` requires `ICS Classic Camera` in Mod.ini. `Ultimate Mod Pack` requires Balance Ultimate.
- TotalRebalance and CasualRebalance require New Balance and Combat; their compatibility patches and compilations inherit that validation requirement.
- Zliden/Easy Game compilations recommend ICS Classic Camera, TimeMultiplier and/or player animation packs; required and recommended selections are different. Review both in Union+.
- The New Magic for NPCs readme requires New Balance and Combat even though its Mod.ini omits Required.
- CombatAnimationOverhaul_2H and Monsters Unleashed explicitly conflict. The overhaul author also warns against balance-mod combinations.
- High draw-distance presets only apply through recommended settings. The protected workflow preserves existing non-Game engine INI values.

Additional binary evidence: the local `Script_TimeMultiplier.dll` imports `eCApplication::SetTimeScaling` and `GetTimeScaling`, consistent with application time scaling rather than a texture change. Its supplied INI sets `Multiplier=2.5`. No third-party plugin was executed for this audit.

## Avoid: direct VR overlap

### ICS Classic Camera
Folder: `M_IntuitiveModClassicCamera`.

Not recommended with VR: changes backward/sideways/diagonal movement speed and forces instant actor turns toward the camera. This overlaps VR heading and native locomotion integration. This is ICS Classic Camera, not Intuitive Combat System.

Evidence: local readme `ReadmeEN.rtf`.

### TimeMultiplier
Folder: `Script_TimeMultiplier`.

Do not activate accelerated game time in VR. The supplied INI sets Multiplier=2.5 (hotkey Z); changing simulation/animation speed can conflict with physical combat timing and comfort. Installing the plugin does not prove that acceleration is active. Leave it off or at 1.0.

Evidence: local readme `ReadmeEN.rtf`.

## Not recommended for the VR baseline (unvalidated overlap)

### 2H_BlockingAbility
Folder: `M_1_2H_BlockingAbility`.

Not recommended for the baseline until tested: Script_OptionalGuard changes native 2H blocking rules, which VR already uses for physical guard and stamina. Union+ also declares it incompatible with New Balance and Combat.

Evidence: local readme `ReadmeEN.rtf`.

### Animations Redone - Sword And Shield
Folder: `M_10_0_AnimationsRedone_SwordAndShield`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`.

### Sword And Shield Last Attack (Slash Down)
Folder: `M_10_1_1_AnimationsRedone_SwordAndShield`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Animations Redone - Sword And Shield`.

### Sword And Shield First Attack (Horizontal Slash)
Folder: `M_10_1_2_AnimationsRedone_SwordAndShield`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Animations Redone - Sword And Shield`.

### CombatAnimationOverhaul_2H
Folder: `M_3_CombatAnimationOverhaul_2H`.

Not recommended for the baseline until tested: the 2H overhaul changes hero combat animations and weapon hitboxes and installs Script_Animation.dll. VR weapon contacts, native finishing moves and damage can disagree. Its author excludes balance-mod combinations; Monsters Unleashed also declares a conflict with this overhaul. The ForMain patch inherits the overhaul risk.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Animations Redone - Two Handed Swords`.

### FasterCombatMod
Folder: `M_4_FasterCombatMod`.

Not recommended for the baseline until tested: replaces combat animations and modifies Alternative AI blocking for dodges. This can overlap VR guard, native combat movement and recovery. This is a compatibility risk, not a reproduced failure.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Intuitive Combat System;CombatAnimationOverhaul_2H;Animations Redone - Staffs and Halberds;Animations Redone - Two Handed Swords;Animations Redone - Dual Wield`.

### Animations Redone - Dual Wield
Folder: `M_5_AnimationsRedone_DualWield`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod`.

### Animations Redone - Staffs and Halberds
Folder: `M_6_0_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod`.

### Normal Downward Slash Attacks
Folder: `M_6_1_1_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Normal Horizontal Slash Attacks;Normal Stab Attacks`.

### Normal Horizontal Slash Attacks
Folder: `M_6_1_2_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Normal Stab Attacks;Normal Downward Slash Attacks`.

### Normal Stab Attacks
Folder: `M_6_1_3_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Normal Downward Slash Attacks;Normal Horizontal Slash Attacks`.

### Quick Attack Upward Slash
Folder: `M_6_2_1_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Quick Attack Downward Slash;Quick Attack Stab`.

### Quick Attack Downward Slash
Folder: `M_6_2_2_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Quick Attack Stab;Quick Attack Upward Slash`.

### Quick Attack Stab
Folder: `M_6_2_3_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Quick Attack Upward Slash;Quick Attack Downward Slash`.

### Power Stab Attack
Folder: `M_6_3_1_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Power Jump Attack;Power Slash Attack`.

### Power Jump Attack
Folder: `M_6_3_2_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Power Slash Attack;Power Stab Attack`.

### Power Slash Attack
Folder: `M_6_3_3_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Power Stab Attack;Power Jump Attack`.

### Finishing Downward Slash Attack
Folder: `M_6_4_1_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Finishing Stab Attack`.

### Finishing Stab Attack
Folder: `M_6_4_2_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Finishing Downward Slash Attack`.

### Double Attack
Folder: `M_6_5_1_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Whirl Attack`.

### Whirl Attack
Folder: `M_6_5_2_AnimationsRedone_StaffsAndHalberds`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;Double Attack`.

### Animations Redone - Two Handed Swords
Folder: `M_7_0_AnimationsRedone_TwoHandedSwords`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod;CombatAnimationOverhaul_2H`.

### Two Handed Power Jump Attack
Folder: `M_7_3_1_AnimationsRedone_TwoHandedSwords`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod`.

### Two Handed Whirl Attack
Folder: `M_7_5_1_AnimationsRedone_TwoHandedSwords`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting player attacks, root movement, guard, finishing moves and tracked weapon contacts. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod`.

### Animations Redone - Unarmed Movement
Folder: `M_8_0_AnimationsRedone_UnarmedMovement`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting unarmed movement and camera/body recovery. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`.

### Animations Redone - Bows and Crossbows
Folder: `M_9_0_AnimationsRedone_BowsAndCrossbows`.

Not recommended for the supported baseline until individually tested: replaces hero animations affecting ranged sprint, sneak, shooting and attachment/reload transitions. VR still uses native animation states for some actions. This is an unvalidated overlap, not proof that this variant is broken.

Evidence: local readme `ReadmeEN.rtf`.

### CombatAnimationOverhaul_2H_ForMain
Folder: `O_CombatAnimationOverhaul_2H_ForMain`.

Not recommended for the baseline until tested: the 2H overhaul changes hero combat animations and weapon hitboxes and installs Script_Animation.dll. VR weapon contacts, native finishing moves and damage can disagree. Its author excludes balance-mod combinations; Monsters Unleashed also declares a conflict with this overhaul. The ForMain patch inherits the overhaul risk.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CombatAnimationOverhaul_2H;Main`; Mod.ini Incompatible: `TotalRebalance`.

### Animation Pack 'Rapier Style'
Folder: `R_Rapier Style v1.0`.

Not recommended for the baseline until tested: Script_RapierAnimation and replacement movement/block/stab animations change rapier handling. Its Script_Statuses integration applies bleeding through normal/piercing attacks; physical VR hits must be checked against those native attack classifications.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Script_Statuses`.

### ShieldRebalance
Folder: `S_ShieldRebalance`.

Not recommended for the baseline until validated: requires New Balance and Combat and changes shield effects and magical-staff blocking. Check physical guard, mana/stamina, shield perks and reflected spells with that parent overhaul.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `New Balance and Combat`.

### Sneaky Loot
Folder: `S_Sneaky Loot`.

Not recommended for the baseline until tested: replaces player stealth movement and speeds stealth looting animations. Validate native sneak state, root movement and the VR activity camera before using it.

Evidence: local readme `ReadmeEN.rtf`.

### Script_Statuses
Folder: `Script_Statuses`.

Not recommended for the baseline until its required New Balance and Combat parent is validated. Bleeding depends on native strong-attack classification, with special rapier normal/piercing-attack handling. Check physical VR damage and status effects; do not treat the dependency as an optional HUD-only feature.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `New Balance and Combat;G3Fixes`.

### New Adventures&TotalRebalance
Folder: `X_NewAdventures&TotalRebalance`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `readmeEN.rtf`; Mod.ini Required: `New Adventures;TotalRebalance`.

### New Balance and Combat
Folder: `Y_01_NewBalance`.

Not recommended for the supported VR baseline until integrated: replaces combat damage/stamina rules and introduces evasion/parry and new magic aiming. Its INI also changes bow animation speed by skill. Physical hits, native stab/murder, guard, casting and ranged progression need validation; incompatibility is not established by this static audit.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `UpdatePack Combat Fix;2H_BlockingAbility`.

### Balance Ultimate
Folder: `Y_03_Balance Ultimate`.

Not recommended with the current VR baseline: Mod.ini requires Balance Ultimate / its dependency chain, including ICS Classic Camera, Intuitive Combat System, Rapier Style and New Balance and Combat. Camera and combat risks are inherited, not just an optional texture choice. Do not remove mandatory dependencies independently.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Learning Threshold Tweaks;Animation Pack 'Rapier Style';Animations Pack 'Monsters Unleashed';ICS Classic Camera;Intuitive Combat System;AutoSave;Script_CustomRepPrices;Alternative Pickpocketing;Monster Damage;LivelyTownsMod;LTM_NPC;LTM Easy;LearnPoints;CM Content;Main;G3Fixes;New Game Configuration;CM Letters;Immersive World;Knight Chronicle;Consequences;CNQDragons;Extended Frying;Quick Loot;Refresh Trade Generated;New Balance and Combat;New Magic for NPCs Mod;Subtitles;QuestFix Mod;ItemLocator;Advanced Perk Info;Teleport Party Members`; Mod.ini Incompatible: `CNQMagicWeapons;2H_BlockingAbility`.

### Ultimate Mod Pack
Folder: `Y_03_Build_UltimateModPack`.

Not recommended with the current VR baseline: Mod.ini requires Balance Ultimate / its dependency chain, including ICS Classic Camera, Intuitive Combat System, Rapier Style and New Balance and Combat. Camera and combat risks are inherited, not just an optional texture choice. Do not remove mandatory dependencies independently.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Balance Ultimate`; Mod.ini Incompatible: `CNQMagicWeapons;2H_BlockingAbility`.

### TotalRebalance
Folder: `Y_05_TotalRebalance`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CM Rebalance;CNQDragons;ContentModBase;CM Graphic Arcania;CM Graphic G2;CM Content;Quest Edit;TameAnimal;CNQMagicWeapons;Main;Immersive World;LivelyTownsMod;G3Fixes;Pirate Dream;Knight Chronicle;New Game Configuration;Conquest;RGL;Monster Damage;Immersive Cook;TRLimitedStatues;Alternative Pickpocketing;New Balance and Combat;New Magic for NPCs Mod;CNQThiefQuests`; Mod.ini Incompatible: `CM End Game;Alternative Balance Uncapped;Magic No God To Be Chosen;LTMSmeltingMod;CNQEasyPaladin;LTM Easy;Citizen;Refresh Trade Generated;CombatAnimationOverhaul_2H_ForMain;Balance Ultimate;DRAGONS REWARDS;Finishing Stab Attack;Finishing Downward Slash Attack`.

### TotalRebalance & ShieldRebalance
Folder: `Y_08_TotalRebalance_&_ShieldRebalance`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `readme.rtf`; Mod.ini Required: `ShieldRebalance;TotalRebalance;EnchantStones`.

### TotalRebalance_&_WolfModel
Folder: `Y_10_TotalRebalance_&_WolfModel`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `TotalRebalance;Wolf Model`.

### TotalRebalance_&_OrcLite
Folder: `Y_11_TotalRebalance_&_OrcLite`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `TotalRebalance;Orc Mod Lite`.

### Zliden Mod Pack & NewBalance_Hard
Folder: `Y_99_Build_ZlidenModPack_&_NewBalance_Hard`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `TotalRebalance`; Mod.ini Incompatible: `Zliden Mod Pack & NewBalance_Light`.

### Zliden Mod Pack & NewBalance_Light
Folder: `Y_99_Build_ZlidenModPack_&_NewBalance_Light`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `TotalRebalance`; Mod.ini Incompatible: `Zliden Mod Pack & NewBalance_Hard`.

### CasualRebalance
Folder: `Y_CasualRebalance`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `readmeEN.rtf`; Mod.ini Required: `CM Rebalance;Intuitive Combat System;LTM Easy;Pirate Dream;Extended Frying;Consequences;New Balance and Combat`; Mod.ini Incompatible: `Balance Ultimate;TotalRebalance;CNQArenas;CNQDragons;Alternative Bonuses;CNQMagicWeapons;Balance Ultimate`.

### Easy Game Mod Pack
Folder: `Y_Y_Build_EasyGame`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CasualRebalance`.

### TotalRebalance_&_ArmorAndWeaponRebuilt
Folder: `Z_1_1_TotalRebalance_&_ArmorAndWeaponRebuilt`.

Not recommended for the baseline until the parent combat overhaul is validated: mandatory dependencies include New Balance and Combat, TotalRebalance or CasualRebalance. Changed skills, damage, stamina, spells and transformations need VR integration checks. Compilation recommendations can additionally select ICS Classic Camera, animation packs and TimeMultiplier. Compatibility patches must stay with their parent mods; do not remove a required patch alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `TotalRebalance;Armor And Weapon Rebuilt`.

## Requires a targeted compatibility check

### CM Letters
Folder: `C_CMLetters`.

Includes additional transformation items/forms. Check animal camera mode, return-to-human controls and recentering for the added forms; ordinary letters and quests have no identified VR conflict.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CM Content`.

### Advanced Perk Info
Folder: `D_Advanced Perk Info`.

The descriptions are written for, and Mod.ini requires, New Balance and Combat. The text itself does not change tracking, but that mandatory combat overhaul is not validated for the VR baseline. Do not install the descriptions as a substitute for the required mechanics.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `New Balance and Combat`.

### Demon Model
Folder: `F_DemonModel`.

The author explicitly warns that the model has invisible legs that can be hit with arrows. Visible geometry and hittable geometry differ; check both physical weapon contacts and ranged aiming. This is an author-documented model caveat, not a VR crash.

Evidence: local readme `ReadmeEN.rtf`.

### Hero Head PreAlpha
Folder: `H_HeadAlpha`.

Replaces the hero head/associated model resources. Check first-person head hiding and third-person knockdown, activity and transformation recovery. No failure is established by the description alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Head CM;Head Arcania`.

### Head Arcania
Folder: `H_HeadArcania`.

Replaces the hero head/associated model resources. Check first-person head hiding and third-person knockdown, activity and transformation recovery. No failure is established by the description alone.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Head CM;Hero Head PreAlpha`.

### Intuitive Combat System
Folder: `L_IntuitiveCombatSystem`.

Changes melee strike animations/timing only; it is NOT ICS Classic Camera. Test native stab/finish/recovery and physical damage before adding it. No camera change or confirmed VR failure is established by its readme.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `FasterCombatMod`.

### Animals Hardcore Mod
Folder: `M_2_AnimalsHardcoreMode`.

Speeds enemy power attacks. The author recommends specific combat fixes or Update Pack 1.04 and warns about excessive difficulty otherwise. This is enemy animation/balance content, not a replacement of VR hand tracking; validate the required combat combination.

Evidence: local readme `ReadmeEN.rtf`.

### CNQMagicWeapons
Folder: `Q_MagicWeapons`.

Adds elemental staves, shields and weapon effects and a perk allowing a 2H sword to be used in one hand. Check weapon-category/grip selection, physical damage, guard and spell effects; the new perk must not be assumed supported.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`; Mod.ini Incompatible: `Balance Ultimate`.

### Menu Updated
Folder: `R_2_ModMenu Updated`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme;Menu Ultimate`.

### Menu Updated ForNewSkills
Folder: `R_3_ModMenu Updated ForNewSkills`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Menu Updated`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Compas Returning 2
Folder: `R_4_UpdatedReturningCompas2`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Menu Updated`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### DRAGONS REWARDS
Folder: `R_DRAGON REWARDS`.

Adds new armor, shield and weapon models/items. Check skinning, palm contacts and weapon hit bounds. The readme requires LTM and Conquest_Swords&Dragons although Mod.ini omits Required; use the intended parent combination and its matching model patches.

Evidence: local readme `ReadmeEN.rtf`.

### Immersive World
Folder: `R_Immersive World`.

Large world/gameplay changes introduce extra interactions and interface/content dependencies. Check new activity cameras, custom inventory/skill pages and transitions. New Adventures additionally requires the high draw-distance preset; protected installation preserves existing engine INI values. Content is not declared incompatible solely because it adds locations.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main;Extended Frying`.

### Immersive World Updated MenuDark
Folder: `R_Immersive World Updated MenuDark`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuOld;Night Theme`.

### Immersive World Updated MenuOld
Folder: `R_Immersive World Updated MenuOld`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Night Theme`.

### Menu Elder
Folder: `R_Menu Elder`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Menu CM
Folder: `R_MenuAkaCM`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu Dark;Menu Old;Menu QP;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Menu Arcania
Folder: `R_MenuArcania`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu Dark;Menu Old;Menu QP;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Menu Dark
Folder: `R_MenuDark`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Old;Menu QP;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Menu Old
Folder: `R_MenuOld`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu QP;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Menu QP
Folder: `R_MenuQP`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Compas Returning
Folder: `R_MenuReturningCompas`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu Updated;Menu Ultimate;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Night Theme`.

### Clear Dialog Box
Folder: `R_ModClearDialog`.

Transparent dialogue windows can reduce text contrast in VR. Check readability and controller navigation; there is no identified renderer or locomotion replacement.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Updated;Menu Ultimate;Compas Returning 2;Night Theme`.

### Animations Pack 'Monsters Unleashed'
Folder: `R_Monsters Unleashed v1.4`.

Enemy animation pack, not a blanket VR exclusion. The readme explicitly declares incompatibility with CombatAnimationOverhaul_2H. Check changed enemy contacts/telegraphs; do not combine those two packs.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `CombatAnimationOverhaul_2H`.

### EnchantStones
Folder: `S_EnchantStones`.

Adds or changes item/status/combat rules. Validate enchantment effects, normal/power/stab classification, bleeding/freezing/burning and native progression with physical VR hits. Do not assume a blanket incompatibility or disable prerequisites required by another selected mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `G3Fixes`.

### DialogueSkipper
Folder: `Script_DialogueSkipper`.

Default skip shortcut needs MOUSE WHEEL plus right mouse. The controller does not provide that exact chord by default; changing it to MOUSE RIGHT may overlap right-grip Use2. Configure and test dialogue navigation rather than assuming plug-and-play controller support.

Evidence: local readme `ReadmeEN.rtf`.

### ItemLocator
Folder: `Script_eCDELocator`.

Optional local import of original eCDELocator 1.0.7 by fyryNy is supported in the VR launcher. Hold left trigger and tap X to toggle markers. Radius and grouped filters are configurable; ROI is not raised. Stereo marker placement and CPU cost still need a headset check. No third-party DLL or artwork is bundled. See [ITEM-LOCATOR.md](ITEM-LOCATOR.md).

Evidence: original Nexus description, local 1.0.7 configuration and keys list.

### Floating Damage
Folder: `Script_FloatingDamage`.

Changes HUD text or focus labels. Check size, stereo placement and overlap with VR status icons. No specific native camera/combat conflict is identified.

Evidence: local readme `ReadmeEN.rtf`.

### Font Scale
Folder: `Script_FontScale`.

Changes HUD text or focus labels. Check size, stereo placement and overlap with VR status icons. No specific native camera/combat conflict is identified.

Evidence: local readme `ReadmeEN.rtf`.

### G3Fixes
Folder: `Script_G3Fixes`.

Adds or changes item/status/combat rules. Validate enchantment effects, normal/power/stab classification, bleeding/freezing/burning and native progression with physical VR hits. Do not assume a blanket incompatibility or disable prerequisites required by another selected mod.

Evidence: local readme `ReadmeEN.rtf`.

### ItemMap
Folder: `Script_ItemMap`.

Custom map/locator uses extra keyboard shortcuts and the F10 scripts menu, which are not all assigned on the VR controller. Check accessibility and stereo overlays. Locator search radius also adds work while enabled; avoid assuming it is free.

Evidence: local readme `ReadmeEN.rtf`.

### Quick Loot
Folder: `Script_QuickLoot`.

Supported provider selection: the VR Union+ guard disables native Update Pack QuickLoot when this replacement DLL is installed; otherwise it enables native QuickLoot. Right grip uses native Use2. Verify looting once after installation; do not enable both providers.

Evidence: local readme `ReadmeEN.rtf`.

### QuickSaveBackuper
Folder: `Script_QuickSaveBackuper`.

Rotates native quicksave backups. VR radial Quicksave sends the configured native QuickSave action, not a separate save implementation. Check slot rotation; no inherent VR conflict is identified.

Evidence: local readme `ReadmeEN.rtf`.

### StatBarValues
Folder: `Script_StatBarValues`.

Changes HUD text or focus labels. Check size, stereo placement and overlap with VR status icons. No specific native camera/combat conflict is identified.

Evidence: local readme `ReadmeEN.rtf`.

### New Adventures
Folder: `X_NewAdventures`.

Large world/gameplay changes introduce extra interactions and interface/content dependencies. Check new activity cameras, custom inventory/skill pages and transitions. New Adventures additionally requires the high draw-distance preset; protected installation preserves existing engine INI values. Content is not declared incompatible solely because it adds locations.

Evidence: Mod.ini Required: `Main;LivelyTownsMod;Alternative World;Immersive World;RB_Mod;Extended Frying;ContentModBase;CM Content;Orc Mod Lite;INI Draw Distance Normal`.

### Night Theme
Folder: `X70_NightTheme`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Menu Updated;Compas Returning 2;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Menu Updated ForNewSkills`.

### New Magic for NPCs Mod
Folder: `Y_02_NewMagicforNPCs_Mod`.

Readme requires New Balance and Combat, although Mod.ini does not declare that requirement. NPC spell changes are not inherently incompatible; the stated parent combat overhaul needs validation. It is mutually exclusive with UpdatePack Combat Fix and 2H_BlockingAbility.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `UpdatePack Combat Fix;2H_BlockingAbility`.

### Menu Ultimate
Folder: `Y_03_Ultimate Menu`.

Interface replacement/patch: check stereo HUD placement, focus-name readability, controller cursor, inventory and lockpicking. These are not automatically incompatible with VR. Respect Union+ GUI mutual exclusions and the required parent content/balance mod.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Menu Elder;Menu CM;Menu Arcania;Menu Dark;Menu Old;Menu QP;Compas Returning;Clear Dialog Box;Immersive World Updated MenuDark;Immersive World Updated MenuOld;Menu Updated`.

### Armor And Weapon Rebuilt
Folder: `Z_1_0_ArmorAndWeaponRebuilt`.

Replaces hundreds of equipment models, including animated bows/crossbows. Recheck palm contacts, support grips, arrow/bolt alignment, visibility in both eyes and physical hit bounds. A model replacement is not automatically incompatible; use matching compatibility patches when required.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `LivelyTownsMod;ContentModBase`; Mod.ini Incompatible: `CM Citizen Textures`.

### INI Draw Distance Original
Folder: `Z_3_INI Draw Distance Original`.

Restores stock draw distances when recommended settings are applied. It is not a VR incompatibility, but replacing tuned settings changes the image/performance comparison. Protected installation retains existing non-Game ge3.ini values.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `INI Draw Distance Normal`.

## Optional performance caution

### Smart Monsters Skins
Folder: `E_SmartMonstersSkins`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`.

### G3 Retextured Final
Folder: `J_G3 Retextured Final`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `QP Texture Pack;PreAlpha;Myrtana Tekstures Pack;Autumn Textures;Smart Texture Pack;World Texture Mix`.

### QP Texture Pack
Folder: `J_GlobalQPTextures`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `PreAlpha;Myrtana Tekstures Pack;Smart Texture Pack;World Texture Mix;G3 Retextured Final`.

### PreAlpha
Folder: `J_ModPreAlphaTextures`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `QP Texture Pack;Myrtana Tekstures Pack;Smart Texture Pack;World Texture Mix;G3 Retextured Final`.

### Myrtana Tekstures Pack
Folder: `J_MyrtanaTeksturesPack`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `QP Texture Pack;PreAlpha;Autumn Textures;Smart Texture Pack;World Texture Mix;G3 Retextured Final`.

### Autumn Textures
Folder: `J_TexturesAutumn`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Smart Texture Pack;World Texture Mix;G3 Retextured Final;Myrtana Tekstures Pack`.

### Smart Texture Pack
Folder: `J_USMTexturesPack`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `QP Texture Pack;PreAlpha;Myrtana Tekstures Pack;Autumn Textures;World Texture Mix;G3 Retextured Final`.

### World Texture Mix
Folder: `J_WorldTextureMix`.

Texture/model workload depends on selected overlays, image compression and actual residency. Watch streaming, memory and frame time; do not label this pack incompatible or necessarily slower without a matched measurement. USM was already tested in this VR installation without an obvious FPS loss.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `QP Texture Pack;PreAlpha;Myrtana Tekstures Pack;Autumn Textures;Smart Texture Pack;G3 Retextured Final`.

### Atmospheric Weathers
Folder: `K_AtmosphericWeathers`.

Weather/fog/material changes alter visibility and can change scene cost. Shader-cache rebuilding after installation may cause initial stalls. No universal FPS gain/loss or VR incompatibility is established; keep engine distances fixed during comparison.

Evidence: local readme `ReadmeEN.rtf`.

### Reduced Fog
Folder: `K_Reduced Fog`.

Weather/fog/material changes alter visibility and can change scene cost. Shader-cache rebuilding after installation may cause initial stalls. No universal FPS gain/loss or VR incompatibility is established; keep engine distances fixed during comparison.

Evidence: local readme `ReadmeEN.rtf`.

### CNQLeadership
Folder: `O_Leadership`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### LivelyTownsMod
Folder: `O_LivelyTownsMod`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### LTM_NPC
Folder: `O_LivelyTownsMod_NPC`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `LivelyTownsMod`.

### TameAnimal
Folder: `O_TameAnimal`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### Conquest
Folder: `Q_Conquest`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### CNQEnclaves
Folder: `Q_Conquest_Enclaves`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Conquest`.

### RB_Mod
Folder: `R_RB_Mod`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `LivelyTownsMod`; Mod.ini Incompatible: `RB_Mod_Without_LTM`.

### RB_Mod_Without_LTM
Folder: `R_RB_Mod_Without_LTM`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `RB_Mod;LivelyTownsMod;Conquest;Balance Ultimate;`.

### AutoSave
Folder: `Script_AutoSave`.

Optional autosaving can cause native save stalls even if textures are cached. It is not inherently incompatible with VR. The bundled INI is the authority for actual interval/slot count, not assumptions based on the readme.

Evidence: local readme `ReadmeEN.rtf`.

### INI Draw Distance Normal
Folder: `Z_2_INI Draw Distance Normal`.

High draw-distance preset; not recommended as an automatic replacement for the tuned VR configuration. It takes effect through Union+ recommended settings. More visibility can increase CPU/render/streaming costs. Protected installation retains existing non-Game ge3.ini values; no distances are changed by these notes.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `INI Draw Distance Original`.

### Alternative World
Folder: `ZZ_Alternative World`.

Extra NPCs/companions, large battles, world geometry or vegetation can increase CPU simulation, visibility and streaming work. This is a conditional performance warning, not proven VR incompatibility. Compare the same location/save before changing engine distances.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

## Incomplete / documentation-only folders

### Animations Redone - Magic Movement
Folder: `M_11_0_AnimationsRedone_MagicMovement`.

This folder has no Mod.ini in this copy and no installable payload. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

Evidence: local readme `ReadmeEN.rtf`.

### Animations Redone - One-Handed Weapons
Folder: `M_12_0_AnimationsRedone_OneHandedWeapons`.

This folder has no Mod.ini in this copy and no installable payload. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

Evidence: local readme `ReadmeEN.rtf`.

### PowerAttacks (Horizontal)
Folder: `M_12_1_AnimationsRedone_OneHandedWeapons`.

This folder has no Mod.ini in this copy and no installable payload. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

Evidence: local readme `ReadmeEN.rtf`.

### R_1_Immersive World
Folder: `R_1_Immersive World`.

This folder has no Mod.ini in this copy and only partial resources. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

### Undead
Folder: `X75_Undeads`.

This folder has no Mod.ini in this copy and only partial resources. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

Evidence: local readme `ReadmeEN.rtf`.

### New Adventures
Folder: `Y_01_NewAdventures Main`.

This folder has no Mod.ini in this copy and only partial resources. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

Evidence: local readme `readmeEN.rtf`.

### New Adventures - Quests
Folder: `Y_02_NewAdventures Quests`.

This folder has no Mod.ini in this copy and no installable payload. It is not a complete standalone selectable mod. Do not manually copy it into the game. Its description alone cannot establish compatibility.

Evidence: local readme `readmeEN.rtf`.

## No specific VR conflict identified

### Readme
Folder: `A_01_Readme`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### QuestPaket
Folder: `A_1_QuestPaket`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### BaseMod
Folder: `A_2_BaseMod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### QP Mini Quests
Folder: `A_QuP MiniQuests`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Achievements`.

### Streaming Music Mod
Folder: `A_Streaming Music Mod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### ContentModBase
Folder: `B_ContentModBase`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `BaseMod`.

### CM Animal & Mushroom Retexture
Folder: `C_CM Animal Retextured`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### CM Graphic Arcania
Folder: `C_CM Graphic Arcania`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `ContentModBase`.

### CM Graphic G2
Folder: `C_CM Graphic G2`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `ContentModBase`.

### CM Content
Folder: `C_CM Kontent`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CM Graphic Arcania;CM Graphic G2`.

### CM Rebalance
Folder: `C_CM_Rebalance`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CM Content`.

### CM Citizen Textures
Folder: `C_CMCitizenTextures`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `ContentModBase`; Mod.ini Incompatible: `Armor And Weapon Rebuilt`.

### CM End Game
Folder: `C_CMModEndGame`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `CM Letters`; Mod.ini Incompatible: `TotalRebalance`.

### Citizen
Folder: `D_Citizen`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `TotalRebalance`.

### Orc Mod Lite
Folder: `D_OrcLite`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Orc Mod Full`.

### Orc Mod Full
Folder: `D_OrcModFull`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Orc Mod Lite`.

### Orc Textur Mod
Folder: `D_OrcTexturMod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Orc Mod Lite`.

### UpdatePack_SupportMod
Folder: `D_UpdatePack_SupportMod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Bloodfly Model
Folder: `F_BloodflyModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Boar Model
Folder: `F_BoarModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Crawler Model
Folder: `F_CrawlerModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Goblin Model
Folder: `F_GoblinModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Lurker Model
Folder: `F_LurkerModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Scavenger Model
Folder: `F_ScavengerModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Shadowbeast Model
Folder: `F_ShadowbeastModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Snapper Model
Folder: `F_SnapperModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Troll Model
Folder: `F_TrollModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Varan Model
Folder: `F_VaranModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Wolf Model
Folder: `F_WolfModel`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Head CM
Folder: `H_CMHead`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Hero Head PreAlpha;Head Arcania`.

### Music G2
Folder: `I_G2Soundtrack`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Smart Music Mod;Smart Music Menu`.

### German Voice
Folder: `I_GermanVoice`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Polish Voice Mods
Folder: `I_GPolishVoice_Mods`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Old Monster Sound
Folder: `I_MonsterSound`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Better Sound Effects;Smart Sound Pack`.

### Old Orc's Sound
Folder: `I_OrcSound`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Scriming Sound
Folder: `I_ScrimingSound`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Smart Music Menu
Folder: `I_SmartMusicMenu`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Music G2`.

### Smart Music Mod
Folder: `I_SmartMusicMod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Music G2`.

### Better Sound Effects
Folder: `I_SoundBetter`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Old Monster Sound`.

### Smart Sound Pack
Folder: `I_SoundPack`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Old Monster Sound`.

### FeedBackEffect
Folder: `K_FeedBackEffect`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### QuestFix Mod
Folder: `L_QuestFix_Mod_0.1.8`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Main
Folder: `N_Main`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `BaseMod`.

### New Assassine Textures
Folder: `O_AssassineTextures`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Consequences
Folder: `O_Consequences`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### LTM Easy
Folder: `O_LivelyTownsMod_Easy`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `LivelyTownsMod`; Mod.ini Incompatible: `TotalRebalance`.

### New Mercenary Textures
Folder: `O_MercenaryTextures`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Pirate Dream
Folder: `O_Pirate_Dream`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### RGL
Folder: `O_RGL`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### LTMSmeltingMod
Folder: `O_SmeltingMod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`; Mod.ini Incompatible: `TotalRebalance`.

### LTMTeleportNPCMod
Folder: `O_TeleportNPCMod`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### CNQArenas
Folder: `P_Arenas`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### CNQDragons
Folder: `P_Dragons`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### CNQMoreTeleportationStones
Folder: `P_TeleportationStones`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### CNQThiefQuests
Folder: `P_ThiefQuests`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### NoRevolutionCounter
Folder: `Q_NotRevolutionCounter`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Knight Chronicle
Folder: `R_(ARP)_Knight Chronicle`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main`.

### Immersive Cook
Folder: `R_Immersive_Cook`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `Main;Immersive World`.

### Alternative Balance Uncapped
Folder: `S_AlternativeBalanceUncapped`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `TotalRebalance`.

### TRLimitedSleep
Folder: `S_LimitedSleep`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### TRLimitedStatues
Folder: `S_LimitedStatues`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Magic No God To Be Chosen
Folder: `S_Magic_No_God_To_Be_Chosen`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `TotalRebalance`.

### Chszonszcze
Folder: `Script_Chszonszcze`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### UpdatePack Combat Fix
Folder: `Script_CombatFix`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `New Balance and Combat`.

### Script_CustomRepPrices
Folder: `Script_CustomRepPrices`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Extended Frying
Folder: `Script_ExtendedFrying`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### LearnPoints
Folder: `Script_LearningPoints`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Learning Threshold Tweaks
Folder: `Script_LearningThresholdTweaks`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `Alternative Balance Uncapped`.

### Monster Damage
Folder: `Script_MonsterDamage`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### New Game Configuration
Folder: `Script_NewGameConfiguration`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Refresh Trade Generated
Folder: `Script_RefreshGeneratedTrade`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `TotalRebalance`.

### Subtitles
Folder: `Script_Subtitles`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Teleport Party Members
Folder: `Script_TeleportPartyMembers`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Achievements
Folder: `X71_Achievements`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Incompatible: `QP Mini Quests`.

### Quest Edit
Folder: `X72_QuestEdit`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Alternative Pickpocketing
Folder: `X73_AlternativePickpocketing`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### Alternative Bonuses
Folder: `X74_AlternativeBonuses`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`.

### CNQEasyPaladin
Folder: `Z_EasyPaladin`.

No specific VR conflict identified in the local description/metadata/payload inventory. This is not runtime certification. Keep Union+ dependencies and normal mod-to-mod exclusions.

Evidence: local readme `ReadmeEN.rtf`; Mod.ini Required: `ContentModBase`; Mod.ini Incompatible: `TotalRebalance`.

