# Optional Union+ mods and VR protection

**Supported version: [Union+ 1.7.3 from Nexus Mods](https://www.nexusmods.com/gothic3/mods/46?tab=files).** Executable fingerprints are checked internally for VR protection; they are not additional mod-pack release versions. This does not certify every optional mod combination.

Use **Optional mods → Open protected Union+** in the VR launcher. Union+ remains responsible for selection, dependencies, ordering, strings/world merging and uninstalling. No mod payloads are bundled into VR or enabled automatically. The VR mod can run without these optional mods.

UnionStarter requires administrator permission in its executable manifest. The guard requests the Windows UAC confirmation before acquiring locks or starting installation, then starts Union+ suspended under the elevated guard's job. The VR launcher and game do not need to run elevated. Cancelling UAC starts no installer; direct elevated Union+ launch is not used as a fallback.

Protection checks approved executable fingerprints internally. The public package version remains Union+ 1.7.3.

The first protected launch preserves an exact verified copy named `UnionStarter_G3VR_<checksum-prefix>.exe` beside Union+'s Mods folder. Later unknown updates, deletion of the original executable, or deletion of Union+'s temporary `_old.exe` do not invalidate that persistent copy. Copies are rechecked before every launch and held against replacement while running. Arbitrary executables are never approved by filename or version label. A fresh unsupported download with no verified copy still requires a compatible VR update or supported Union+ download.

Union+'s automatic self-update is temporarily suspended for the protected session, preventing a restart from escaping the guard's single-process job. Its prior update preference is restored after closing or through Verify / recover VR after an interruption, without overwriting newly selected mods. The external recovery receipt includes the selected executable, checksum, prior US.ini checksum and update transaction state. The temporary `_old.exe` is only a migration source; it is not a durable backup.

ItemLocator is supplied and managed exclusively by Union+ under **Technical Mods → ItemLocator**. No separate importer, enable toggle or filter editor remains in the VR launcher. Union+ can replace or uninstall its DLL, INI and icons; these are excluded from new core lock sets. Existing recovery receipts still recognize the older separately imported files. Original 1.0.7 and Union+ 1.0.8 are supported by the VR input/projection adapters. The VR shortcut reads the plugin's native configured hotkey in memory, so no F13 setting or INI rewrite is required. See [ITEM-LOCATOR.md](ITEM-LOCATOR.md).

The source's `TotallyRemoveOldMods` deletes `Data/*.m*` / `*.n*` plus absolute paths in `US.ini`'s `InstalledModsFiles`. Other source operations replace Engine/Game/Script/audio/PUP components. VR is not a Data mod archive, but a recorded script file or changed native DLL could still break it.

The separate guard, before allowing Union+ to execute:

1. Requires Gothic 3 closed, supported Engine/Game checksums and the VR startup files present. Refuses an already running Union+ or an unresolved earlier session.
2. Opens existing VR/core/backend/PUP/EDD/VR-settings files with read sharing only, denying ordinary writes, replacement, rename and deletion. Collects VR runtime libraries but excludes logs/captures. The native disk modules remain fixed to the currently supported profile. `Ini/ge3.ini` and `Ini/mountlist.ini` are backed up without persistent file locks because Union+ legitimately writes/merges them, including through exclusive handles.
3. Copies and SHA256-verifies those files to `%LOCALAPPDATA%/Gothic3VR/UnionProtection/<installation-key>/<session>/`, outside the game's resource search tree. Writes a Unicode recovery journal and backs up Union+'s configuration before selecting this launcher's Gothic3.exe path. Other Union+ choices are retained.
4. Starts Union+ suspended, attaches it to a job, then resumes it. The job kills the installer if its guard disappears and permits only that installer process; starting the game or child patch installers from Union+ is blocked. The guard survives closing the VR launcher.
5. After Union+ exits, verifies core hashes and reconciles the two editable INIs before marking the session verified. Existing non-`Game` ge3.ini values (including engine, ROI/distances, graphics, camera, controls and language) and existing mount mappings retain their pre-install values. New mod keys/mounts and Union+'s `Game` section choices are retained, with the Quick Loot compatibility exception below. Missing INIs are restored from backup. The launcher blocks settings writes/game launch during protection and blocks launch after an interrupted session until **Verify / recover VR** completes. Recovery validates every saved checksum and relative path before restoring changed/missing VR files. Older journals without the merge-policy marker retain their original exact-file recovery behavior.

Quick Loot uses **right grip**, mapped through the native `Use2` action (right mouse by default). Union+'s bundled default ge3.ini sets `[Game] QuickLoot=false`. On completion/recovery of a current protected session, the guard enables native Update Pack Quick Loot if `scripts/Script_QuickLoot.dll` is absent. If that optional replacement is installed, native Quick Loot remains disabled, as required by the replacement's English readme. The installed DLL determines the provider; saved selection flags alone do not. The completion message/journal identify the provider. Native combat/blocking and recovery contexts still take precedence over looting.

Always launch Union+ through that button. A direct UnionStarter.exe launch is **outside the guard**. This is protection against the audited installer's normal file operations, not a sandbox against administrator tools, injected code or malicious software. Future previously absent files are not denied by the existing-file locks. Protected native-patch replacement attempts may report access denied; do not reinstall CP/PUP/audio wrappers from this protected workflow. Close Union+, then launch Gothic 3 from the VR launcher.

## Full catalog guidance

Optional mods → **VR compatibility notes...** opens a searchable English review of all 192 folders in the local Union+ 1.7.3 catalog. Hover that button for a short recommendation. The full reference is [VR-MOD-COMPATIBILITY.md](VR-MOD-COMPATIBILITY.md). Direct camera/time overlap, unvalidated player/combat replacements, targeted checks and conditional performance costs are separate categories. These recommendations do not disable installed mods or certify untested combinations.

## Mod compatibility status

All 24 requested folders were inventoried locally; metadata/dependencies are known, but the complete combination is **not yet validated in VR**. Native module/profile verification does not certify gameplay or saves.

- BaseMod → ContentModBase → CM Graphic Arcania + CM Graphic G2 → CM Content are the declared dependency chain. CM Citizen Textures also needs ContentModBase. Union+ enforces the original metadata.
- Model replacements alter textures, meshes and some templates/animations; physical hit registration and skeleton attachment must be tested after installation.
- Monsters Unleashed changes animations/library/sound and installs Script_ZombieAnimation.dll. It declares incompatibility with CombatAnimationOverhaul_2H.
- AutoSave adds Script_G3Autosave.dll and its own INI. Its provided default is 10 minutes / 1 slot. Saving can itself pause gameplay; do not attribute save pauses automatically to texture streaming.
- Reduced Fog changes material resources and may expose more scenery. Keep draw-distance configuration fixed for performance comparisons.
- **ICS Classic Camera is not recommended with Gothic 3 VR**, as it may interfere with VR camera controls. The guard displays this recommendation when it is selected. It changes template data rather than a simple separately named camera DLL.
- Story/template changes (BaseMod/CM Content/QuestFix) should first be tested with a separate new game; preserve existing saves.

This integration does not copy third-party mods for redistribution. The user imports/installs their own local Union+ download; any future bundled release needs the applicable authors' permissions checked separately.

## USM texture audit (no conversion installed)

The 234,065,996-byte Smart Texture Pack archive has 219 entries / 215 XIMG textures. A read-only index/header audit finds 145 DXT3, 48 DXT5, 17 DXT1 and 5 RGBA8 images. All have multiple mip levels. Estimated image payload if **all** 215 textures were simultaneously resident is 476,321,228 bytes; this is not observed VRAM working set or actual upload policy.

Five Myrtana low-poly diffuse textures are 2048×2048 RGBA8 / 12 mip levels, about 21.33 MiB each. Current Enhanced Textures versions of those same names are DXT1 at the same dimensions, about 2.67 MiB each. These five are candidates for an optional compressed variant; compression may change image quality and must be visually checked, so no conversion or resolution reduction was installed.

For the 211 matched texture names across current pak/p00/p01/p02, current payload sums to 662,861,424 bytes and the hypothetical USM replacement sums to 468,456,820. Four incoming names lack a baseline match. **USM is not automatically heavier overall than the already-installed Enhanced Textures pack.** This compares package headers for one overlay, not the complete future mod stack, loaded assets, runtime memory or FPS. Future Union archives/loose overrides require a new inventory. Full GPU/streaming impact remains to be measured after the chosen combination is installed.

## VR startup artwork

The core G3 VR splash (`splash.bmp`) and startup advertising playlist (`Ini/logo.ini`) are backed up for protected Union+ sessions. Union+ may replace these non-code assets during its normal installation; the guard restores the exact saved VR files before completing verification. This avoids an artwork file-sharing error while retaining the VR startup branding. Native DLL and VR runtime protection remains unchanged.
