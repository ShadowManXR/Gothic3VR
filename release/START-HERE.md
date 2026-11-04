# Gothic 3 VR — installation and first launch

Version 1.0 · English setup guide · November 2026

See [release notes](RELEASE-NOTES.md) for the launch feature list and [controller mapping](CONTROLLER-MAPPING.md) for custom bindings.

## What you need

**Game editions tested:** Steam, GOG and CD/retail. The creator confirmed GOG and retail working on 29 October 2026. Startup videos were not skipped in the tested retail installation; this is a known cosmetic limitation. All editions must still use the supported native patch profile below.

- A legitimate Windows installation of **Gothic 3**, not Forsaken Gods, with the exact patch profile below.
- **Windows 10 or 11, 64-bit**, Windows PowerShell 5.1, an SSD and a PC capable of running PC VR. The game itself remains 32-bit.
- A headset, two tracked controllers and one configured PC VR provider: **Virtual Desktop / VDXR**, **Meta Quest Link / Air Link**, or **SteamVR**. Choose your provider in the launcher before launching.
- A current GPU driver. The native graphics path requires Direct3D 11 interoperability. The optional bundled DXVK/FFR backend needs Vulkan 1.4; its fixed foveation path has been tested on NVIDIA RTX hardware.
- [Microsoft Visual C++ v14 Redistributable, x64](https://aka.ms/vc14/vc_redist.x64.exe), needed by the separate 64-bit Meta Link host. [Microsoft download reference](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist).

The development reference system is Quest 3 / Touch controllers and RTX 4080. This is a tested configuration, not a minimum specification or an FPS guarantee. Other headsets, controller layouts and AMD/Intel GPUs need separate validation. Binding candidates are declared for Index, PICO 4/Neo 3, Steam Frame and Khronos Generic controllers; see [controller compatibility](CONTROLLER-COMPATIBILITY.md). A declared binding is not a hardware test. Gothic 3 can remain CPU limited in towns and battles.

## Fresh installation — follow this order

### Four separate native patch packages are required

Update Pack **1.04.11** and **1.05.10 Public Beta** are separate packages. Install 1.04.11 first, then apply the complete 1.05.10 archive over it. The Public Beta does not replace the initial 1.04.11 installation step.

| Order | Required package | Exact download filename |
|---|---|---|
| 1 | [Community Patch 1.75.14](https://www.worldofgothic.de/dl/download_478.htm) | `Gothic_3_EE_Patch_v1.75.14_Int_Full.exe` |
| 2 | [Update Pack 1.04.11](https://www.worldofgothic.de/dl/download_523.htm) | `Gothic_3_EE_v1.75_Int_Update_Pack_v1.04.11.exe` |
| 3 | [Update Pack 1.05.10 Public Beta](https://forum.worldofplayers.de/forum/threads/1347969-Release-Gothic-3-v1-75-Update-Pack/page15) | `3. Update Pack v1.05.10 Public Beta.zip` |
| 4 | [Parallel Universe Patch 1.1.1](https://www.worldofgothic.de/dl/download_678.htm) | `Gothic_3_Parallel_Universe_Patch_v1.1.1.exe` |

Download names can differ between mirrors. The filenames above identify the locally verified reference packages; their SHA-256 fingerprints are recorded in the included `Prerequisites.json`. These packages remain separate downloads from the VR installer.

1. Install Gothic 3 on an SSD in a folder your Windows account can write to. Start the flat game once, then exit it.
2. Install **[Community Patch 1.75.14](https://www.worldofgothic.de/dl/download_478.htm)**. Select the actual game folder. Start and exit the flat game again.
3. Install **Update Pack 1.04.11** using its EXE installer.
4. Extract the **complete Update Pack 1.05.10 Public Beta ZIP** into the same game folder, replacing the files supplied by the archive (author's thread, post #293). Copy all supplied files, not only Script_Game.dll.
5. Install **Parallel Universe Patch 1.1.1**. Follow its included readme; install it after the native patches. Confirm the flat game works before adding VR.
6. Install the x64 Visual C++ runtime above and set up your chosen PC VR provider. For wireless Virtual Desktop, use a wired PC-to-router connection and a suitable 5 GHz network; see [Virtual Desktop's requirements](https://www.vrdesktop.net/).
7. Close Gothic 3, its VR launcher and Union+. Run **Gothic3VR-1.0.4-Setup.exe**. Select the folder containing Gothic3.exe. The installer checks the supported DLL hashes before installing and explains any mismatch.
8. Keep **VR core** selected. The **DXVK / NVIDIA FFR backend**, **VR main-menu artwork**, and **custom main-menu music** are separate optional components. The graphics backend is selected initially to match the creator preset. On an unvalidated GPU, omit it and turn FFR Off in the launcher. Existing graphics proxies must be removed separately before setup; setup refuses unknown d3d9.dll, dxgi.dll and d3d11.dll wrappers.
9. Open **Gothic 3 VR** from its shortcut. Check **Game executable**, then choose your provider in **Camera & controls → OpenXR runtime**. The creator preset initially selects **Virtual Desktop / VDXR**; select **Meta Link** if using Quest Link / Air Link, or **SteamVR** if using SteamVR. The installer does not switch the Windows-wide runtime or install OpenComposite.
10. Activate the headset/controllers in the selected provider, then **Save and launch**. Start a **new game** for a fresh patch/content installation. Keep backups of existing saves before changing story or content mods.
11. Stand comfortably facing forward and hold **L3 + R3 for 0.5 seconds** to recenter. Adjust camera height and hand calibration for your own posture. Test movement, both hands, a conversation, an equipped weapon and saving before adding optional mods.

The native patches, Microsoft runtime, VR provider, Union+ and third-party content mods are separate downloads. The VR installer does not bundle or automatically download them. The supported DLL profile is recorded in the payload manifest; a different version requires validation rather than bypassing the check.

## Defaults and upgrades

New installations use the creator's settings frozen on 1 November 2026 for version 1.0. **Defaults** restores the same launcher preset. Updates preserve the existing `scripts/Script_Gothic3VRDiagnostics.ini` and launcher preferences; they do not replace your calibration.

The preset includes 2560 × 2560 square output per eye with landscape menus, FXAA, Performance VR effects, and FFR mode 2 with a 60% full-detail diameter and 3% transition width. It is a personal reference preset, not an automatic recommendation for every PC. Reduce output resolution first if GPU time is too high. FFR has an oval full-detail region and adjustable Transition width. SMAA Medium / High / Ultra is available instead of FXAA; DLAA and DLSS are not supplied as working release features.

Installation leaves native distance, ROI, vegetation, language and control settings intact. It only reconciles `[Game] QuickLoot`: native Quick Loot is enabled unless the separate Script_QuickLoot.dll provider is installed. A separately installed EDD distance profile can still be imported by the launcher; EDD and its assets are not bundled.

### Startup logos

VR core replaces the native desktop startup splash with the G3 VR emblem. The splash and startup playlist are also protected during managed Union+ sessions. The original `splash.bmp` is backed up outside the game folder and restored on uninstall if the installed replacement is unchanged. No executable or engine hook is needed.

VR setup automatically skips the startup advertising logos (`G3_Logo_01.bik`, `Publisher.bik` and `CPT.bik`) through the game's native `Ini/logo.ini` playlist. The original playlist is backed up outside the game folder; uninstall restores it if the installed empty playlist has not been edited. The video files, story intro, in-game cinematics and credits are retained. VR becomes active at the main menu.

## Quest / Touch controls

| Input | Action |
|---|---|
| Left stick | Native movement; normal movement uses Run |
| L3 tap (press left stick) | Sprint until movement stops; native stamina rules apply |
| Right stick | Turn; select entries while the radial is open |
| R3 tap (press right stick) | Open radial |
| A / X / B in radial | Confirm / cancel / change page |
| B outside radial | Draw / sheath weapon, subject to native game restrictions |
| X outside radial | Inventory |
| Double-tap Y | Toggle learned Sneak; eye icon shows native state |
| Left trigger, double-tap | Quick heal using a health potion from inventory |
| Right trigger | Interact / cast / fire crossbow; hold and release for bow string |
| Right grip | Native secondary action, Quick Loot / block; in trade, transfer one item |
| Left grip near a supported weapon | Attach the support hand |
| Left grip with a one-handed melee weapon + shield | Block with the raised shield; right grip also remains available |
| Left controller menu button | Main menu / back; use radial Main menu if the runtime intercepts this button |
| L3 + R3, hold 0.5 s | Recenter camera |
| X + Y, hold 0.5 s in animal form | Return to human form |
| Hold left trigger + tap X | Toggle optional Union+ ItemLocator markers, when installed |

Melee attacks use physical swings and stabs. Distance and velocity thresholds, stabilization and weight can be adjusted per weapon category. Holding the right trigger charges a skill-dependent power strike. Blocking requires a raised weapon or shield; the parry icon confirms it. Only a one-handed melee weapon plus shield also accepts left grip for blocking. Bows and crossbows cannot block; left grip retains its support function. Skills, ammunition, stamina and native progression still apply. The red skull indicates an available finishing action. The radial also offers numbered quick slots, Quicksave and Main menu.

Optional **Camera & controls → Toggle support grip (2H / staff / crossbow)** selects Hold or Toggle; the release preset uses Toggle. Click left grip near the weapon's support contact to attach; click again to release. **Grip loss distance** is independently adjustable under **2H melee**, **Staff / Halberd** and **Crossbow**. It controls how far the physical support hand can move from the locked contact before detaching (2H melee and Staff / Halberd default to 40 cm, Crossbow to 20 cm; 10–100 cm in 5 cm steps). A zone-loading render stall does not count as a release. Save settings applies these preferences live. Changing weapons, entering a menu or losing tracking releases support. These settings do not change shield blocking or the bow string.

**Controller mapping** lets you assign logical controller buttons, optional two-button combinations, double taps and held gestures. Use **Listen in VR...** with the game already running, or select inputs manually; save to apply live. Runtime-reserved System buttons cannot be assigned. See [mapping instructions](CONTROLLER-MAPPING.md).

During third-person conversations, activities and recovery, the right stick orbits the camera around the character. Physical crouching lowers the visible body; a sustained headset drop of about 30 cm limits movement to Walk until you stand back up. Deep crouching hides the body while retaining tracked hands. Recenter while standing normally.

Camera/tracking features approved for the mod are always enabled. Optional **Camera & controls → Hands only (hide body)** hides the first-person body while retaining tracked hands and held items. Third-person views retain the complete character. The option is Off by default; Save settings applies it live. Bow grip calibration is under Bow; Motion no longer has a separate page. HUD & menus includes a World label size slider for NPCs, enemies, items and signs; the release preset uses 50%. Graphics includes Fog intensity, defaulting to 30%, without changing native draw distances. Native distance/ROI controls are under INI tweaks; hover over their labels for help. [INI tweaks reference](INI-TWEAKS.md).

## Optional mods

First confirm VR works without extra content. Union+ is optional; use your own download. **Supported version: [Union+ 1.7.3 from Nexus Mods](https://www.nexusmods.com/gothic3/mods/46?tab=files).** Install optional mods through the protected workflow below.

Use **Optional mods → Open protected Union+** in the VR launcher. Do not run UnionStarter directly after installing VR: its normal cleanup can remove or replace files. Do not ask protected Union+ to reinstall native patches, graphics wrappers or audio DLLs. After it finishes, close Union+ and let VR verification complete. Use **Verify / recover VR** after an interrupted protected session.

**ICS Classic Camera is not recommended with Gothic 3 VR.** Camera, combat-animation and player-skeleton replacements require special care. Content changes may require a new game. See the launcher's searchable **VR compatibility notes** and the included [full catalog review](VR-MOD-COMPATIBILITY.md) and [protected Union+ instructions](OPTIONAL-MODS-UNION.md).

**ItemLocator** is optional and managed by Union+ (Technical Mods), including its filters and search radius. Hold left trigger and tap X to toggle VR markers. There is no separate Locator installer in this package. [Locator shortcut and compatibility](ITEM-LOCATOR.md). **Reduced Fog** is also an optional Union+ choice; distance sliders do not directly control every atmospheric fog effect.

Texture/model packs remain optional and can increase memory or streaming costs. Keep changes incremental and compare the same save/location. Do not raise cache sizes, ROI or draw distances blindly. G3CU is an optional separate preprocessing tool mentioned in the PUP readme; follow its author instructions after finalizing the mod set.

## Troubleshooting, repair and removal

- **Only flat mode:** select the correct provider in Camera & controls, activate the headset in that provider and inspect the latest `diagnostic-*.jsonl` and `stereo-game-*.jsonl` under `scripts/Gothic3VRDiagnostics/logs`. Meta Link also needs the included `meta64` host and the x64 Microsoft runtime. Do not copy random runtime DLLs over the supplied files.
- **Unsupported prerequisite:** install the listed native patch versions first. A display version alone does not prove the expected DLL is installed. Setup writes no game payload when preflight fails.
- **Duplicate filenames / resource error:** move old DLL/INI backup folders entirely outside the game directory. Gothic scans resource subfolders. Do not keep another copy of the VR package under the game folder.
- **Access denied / locked file:** close the game, launcher and Union+. Check the destination is writable. Setup starts without administrator rights and offers elevation if the selected location requires it.
- **Poor performance:** begin with the same scene and lower per-eye resolution. Keep graphics proxies and overlays to a minimum. FFR needs the supplied compatible backend; turning its slider alone cannot enable unsupported hardware.
- **Corrupt/interrupted VR install:** run this setup again against the same folder. Installation transactions keep verified rollback snapshots outside the game tree and recover an incomplete transaction before copying again.
- **Uninstall:** use Windows Installed apps → Gothic 3 VR, or its uninstall shortcut. Owned, unchanged files are removed or restored to their pre-install state. Modified files and saved configuration are retained. Saves and separately installed mods are not removed. If an older manually installed VR file existed before setup, uninstall restores it; it does not delete that prior installation blindly.

Backups and install journals are stored in `%LOCALAPPDATA%\Gothic3VR\Installations\<installation-key>`. Setup metadata and the installed guide are under `%LOCALAPPDATA%\Gothic3VR\Setup`. Keep backups until you are satisfied with installation/removal. Selecting fewer components on a later run does not remove previously installed components; uninstall first to remove them safely.

This is the version 1.0 launch package. The setup is not code-signed. Compatibility on untested headsets and GPUs is not guaranteed. No files are published or uploaded automatically.

The installer also includes the [corresponding source archive](Gothic3VR-1.0.4-Source.zip) and third-party license notices, installed outside the game resource tree.

Support the project: [ko-fi.com/pecos2k5](https://ko-fi.com/pecos2k5).
