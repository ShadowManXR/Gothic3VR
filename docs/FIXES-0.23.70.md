# Launcher 0.23.70 - Optional mod compatibility guidance

Optional mods now has a **VR compatibility notes...** button and a concise hover tooltip. The help window contains a searchable English review of all 192 folders in the locally supplied Union+ 1.7.3 catalog. Categories distinguish direct camera/time overlap, unvalidated combat/player animation replacements, targeted compatibility checks, possible performance costs and incomplete folders.

This is a static review of local descriptions, dependencies and payload/configuration inventory, with a targeted TimeMultiplier import check. It does not claim 192 headset tests or confirmed incompatibility for every recommendation. Full evidence/reference: [VR-MOD-COMPATIBILITY.md](VR-MOD-COMPATIBILITY.md); machine-readable entries: [union-vr-mod-audit-1.7.3.json](union-vr-mod-audit-1.7.3.json).

Important distinctions: ICS Classic Camera differs from Intuitive Combat System; Balance Ultimate requires the former. Texture/model packs are not automatically incompatible. Monsters Unleashed specifically conflicts with CombatAnimationOverhaul_2H. Optional Quick Loot remains supported through existing provider selection.

Only the launcher and documentation changed. The game plugin remains 0.23.69; no installed third-party mods, engine INIs, VR calibration or runtime DLLs are changed. Recommendations do not automatically disable or install anything.

Validation: Release launcher build; existing launcher process/working-directory test; all 18 pages checked at normal, compact and 150% DPI sizes; help category counts, case-insensitive search, empty results, close/reopen and visual inspection. No game session or save was needed.
