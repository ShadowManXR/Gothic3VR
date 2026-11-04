# Gothic 3 VR

Version **1.0.8**. An unofficial, fully stereoscopic VR modification for Gothic 3, with tracked head and hands, motion-based combat and controller-operated menus.

## Install and play

Use the release installer and follow the [installation and first-launch guide](release/START-HERE.md). A separately purchased copy of Gothic 3 is required; Forsaken Gods is not supported.

The supported native patch order is:

1. Community Patch **1.75.14**.
2. Update Pack **1.04.11**.
3. Complete Update Pack **1.05.10 Public Beta** archive.
4. Parallel Universe Patch **1.1.1**.

Windows 10/11 x64, an SSD, a PC VR headset and two tracked controllers are required. Select Virtual Desktop / VDXR, Meta Quest Link / Air Link, or SteamVR in the launcher. The reference hardware is Quest 3 / Touch and RTX 4080; other hardware needs separate validation. Steam, GOG and retail editions were tested with the supported native patch profile.

- [Release features and limitations](release/RELEASE-NOTES.md)
- [Controls and custom controller mapping](release/CONTROLLER-MAPPING.md)
- [Controller compatibility](docs/CONTROLLER-COMPATIBILITY.md)
- [Optional Union+ 1.7.3 integration](docs/OPTIONAL-MODS-UNION.md)
- [INI tweaks](docs/INI-TWEAKS.md)

The VR radial quick-slot page shows the native item icons and current stack quantities for slots 1â€“9 and 0. Slot numbers remain visible; empty slots keep their numbered placeholders.

VR chest interactions retain the calibrated first-person view and suppress the visible player animation. Native chest scripts and animation events continue internally, preserving the chest lid, lockpicking, loot, ownership and quest logic. The displayed hero pose is frozen during the interaction and restored after each eye; a cold entry without a previous gameplay pose hides the avatar until the interaction ends.

VR dialogue keeps the player's first-person viewpoint and live headset tracking, without the native side-view shots or dialogue camera orbit. Conversation scripts, dialogue choices, subtitles, voice and quest progression continue through the original game logic.

In VR, world-item pickups skip the native 500 ms wait and the animations around inventory transfer. The game's original pickup script still performs the transfer and applies its normal rules. NPCs, non-VR play, chests, mining, crafting, and other activities retain their existing behavior.

The **Hand HUD** page controls independent lower-arm displays: the original quick-slot artwork, item icons and quantities on the left, and health/stamina/mana bars on the right. **Show hand HUD (both arms)** is enabled by default. Each side has its own position X/Y/Z (cm), pitch/yaw/roll (degrees), and Scale (25–200%); Save applies changes live. Offsets use the display's local axes: X across, Y up, Z away from its face. The displays attach to `Hero_Left_Arm_Arm_2` and `Hero_Right_Arm_Arm_2` (the installed rig's lower-arm bones), including during frozen two-hand poses. Tracking loss or world transitions hide them. The native HUD stays in its original location and retains its separate visibility preferences. INI keys under `[Stereo]`: `HandHudEnabled=1`, `LeftHandHudXcm/Ycm/Zcm`, `LeftHandHudPitchDeg/YawDeg/RollDeg`, and the corresponding `RightHandHud...` keys. Calibrated defaults: left position -7/0/0 cm, rotation -90/180/-90 degrees; right position 7/0/0 cm, rotation 90/0/-90 degrees. Both scales default to 50% (`LeftHandHudScalePercent` and `RightHandHudScalePercent`). Scaling keeps the attachment center fixed and resizes the entire display, including icons and quantities.

The launcher's **HUD & menus** page has independent **Show compass**, **Show slot bar**, and **Show health / mana / stamina panel** options. All three are off for new configurations and Defaults. Save settings to apply changes live in VR. Existing explicit preferences are preserved. The INI keys under `[Stereo]` are `HideCompass` (default `1`), `ShowSlotBar` (default `0`), and `ShowVitalsPanel` (default `0`). The VR radial remains available when the native slot bar is hidden. The slot bar and health/mana/stamina block, including their frames and backgrounds, always appear while the inventory is open; closing it restores your saved visibility options.

## Report a bug

Open a [bug report](https://github.com/PeCOS2k5/Gothic3VR/issues/new?template=bug_report.yml) and describe how to reproduce the problem. The form asks for the mod version, headset/controllers, VR provider, PC/settings, installed mods and relevant logs or screenshots. Search [existing issues](https://github.com/PeCOS2k5/Gothic3VR/issues) before submitting the same problem again.

## Build from source

The game/plugin are **32-bit**. Meta Link uses a separate **64-bit** compositor host. Build both architectures; a standalone plugin DLL is not a complete installation.

Requirements: Visual Studio 2022 C++ Build Tools (x86/x64), Windows SDK including `fxc.exe`, CMake 3.24 or newer, Python 3.10 or newer for packaging/tests, and Windows PowerShell 5.1. Build tools must be available on PATH for the commands below.

Obtain [Gothic 3 SDK](https://github.com/georgeto/gothic3sdk) separately. The validated SDK revision is `90bfd344de4510dda7ac9da7461cc7f1eac911f7`. Place its checkout beside this repository as `../gothic3sdk`, or pass its path explicitly. From the repository root:

```powershell
git -C ../gothic3sdk checkout 90bfd344de4510dda7ac9da7461cc7f1eac911f7
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DGOTHIC3SDK_DIR=../gothic3sdk -DBUILD_TESTING=ON
cmake --build build --config Release --target Script_Gothic3VRDiagnostics Gothic3VRResolution Gothic3VRLauncher Gothic3VRUnionGuard -- /m:1 /nr:false
cmake -S tools/MetaBridgeHost -B build-meta -G "Visual Studio 17 2022" -A x64
cmake --build build-meta --config Release --target Gothic3VRMetaHost64
```

Use fresh build directories for production builds. Experimental captures, profiling and renderer trials default to **OFF**; do not enable them in a public build. No headset or game is required just to compile. SDK bindings and required vendored headers/runtime loaders retain their upstream licenses.

### Tests

Tests are registered when `BUILD_TESTING=ON`. Build the test targets before invoking CTest. The full suite contains GPU, local native-profile and runtime-dependent checks; a missing headset/runtime is not evidence of a gameplay regression. To run the headless Meta transport/wire checks specifically:

```powershell
cmake --build build --config Release --target DiagnosticMetaPipeTests DiagnosticMetaBridgeWireTests
ctest --test-dir build -C Release --output-on-failure -R "diagnostic_meta_(pipe|bridge_wire)"
```

`tools/Build-Diagnostics.ps1` is a convenience build/full-test wrapper. `-SdkDirectory` and `-BuildDirectory` accept explicit paths. It never installs unless `-Install` is supplied with both `-GameDirectory` and `-PackageDirectory`.

### Packaging and developer deployment

`release/Build-Package.py` creates a hash-validated payload from a production build and a supported local reference installation. It intentionally checks the native prerequisites and accepted runtime components; it is not a generic copier for arbitrary DLLs. Its currently pinned Meta host must be revalidated and its pin updated when that host is rebuilt. Never bypass a hash mismatch just to generate a package.

```powershell
python release/Build-Package.py --game "<supported game directory>" --build build --sdk ../gothic3sdk --output "<new empty package directory>"
```

The source packaging tool does not install into the game. Compile the Inno Setup script separately to produce a user-facing installer. For developer deployment of a complete, validated payload matching the selected build:

```powershell
./tools/Install-Diagnostics.ps1 -GameDirectory "<game directory>" -BuildDirectory build -PackageDirectory "<package directory>"
```

This delegates to the same `release/Deploy.ps1` used by Setup, including process checks, complete Meta/Union components, preference preservation, external backups and rollback. It installs core components; add `-FfrBackend` to include the optional graphics component. Close Gothic 3, its launcher and Union+ first. Do not copy development backups into the game resource tree: duplicate resource basenames can prevent the game from starting.

## Repository contents

`src/` contains the plugin, launcher and runtime bridges; `tests/` contains validation; `tools/` contains development helpers; `release/` contains deployment and packaging; `third_party/` contains dependency material and notices. Experimental source is kept under `experiments/`; generated captures, game binaries/decompilations and build outputs are excluded from Git.

## License and credits

Project code is **GPL-3.0-or-later**; see [LICENSE](LICENSE). This declaration does not relicense third-party code, commercial game content, trademarks, custom artwork or music. See [third-party notices](THIRD-PARTY-NOTICES.md) for dependency notices and asset scope. The commercial game and its native patch binaries are not supplied by this source repository.

Gothic 3 is the work of its original developers and rights holders. Gothic 3 SDK is by Georgeto and contributors. Thanks to the authors of the native patches, OpenXR/OpenVR, Union+ and the included graphics dependencies.
