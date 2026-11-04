# Gothic 3 VR

**Created and maintained by PeCOS2k5.**

[Official downloads](https://github.com/PeCOS2k5/Gothic3VR/releases) · [Official Nexus Mods page](https://www.nexusmods.com/gothic3/mods/115) · [Authors and credits](AUTHORS.md) · [Project identity](BRANDING.md) · [Verify a release](docs/VERIFY-RELEASES.md)

Version **1.0.1**. An unofficial, fully stereoscopic VR modification for Gothic 3, with tracked head and hands, motion-based combat and controller-operated menus.

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

Modified versions are welcome under the GPL. Preserve applicable notices and identify your changes; independently maintained builds must not be presented as official PeCOS2k5 releases. See [AUTHORS.md](AUTHORS.md) and [BRANDING.md](BRANDING.md).

Gothic 3 is the work of its original developers and rights holders. Gothic 3 SDK is by Georgeto and contributors. Thanks to the authors of the native patches, OpenXR/OpenVR, Union+ and the included graphics dependencies.
