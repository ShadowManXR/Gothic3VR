# Third-party notices and asset scope

The Gothic 3 VR project code is GPL-3.0-or-later; the full license text is in [LICENSE](LICENSE). Third-party components retain the terms in their original notices. Including a file in this repository does not change its ownership or license.

| Component | Notice / source record |
|---|---|
| Gothic 3 SDK, Georgeto and contributors | Separate dependency: [upstream](https://github.com/georgeto/gothic3sdk), `LICENSE` and `COPYING` in that checkout; GPL-3.0-or-later |
| Khronos OpenXR loader and headers | [third_party/openxr/LICENSE](third_party/openxr/LICENSE) |
| Valve OpenVR | [third_party/openvr/LICENSE](third_party/openvr/LICENSE) |
| Vulkan headers | [third_party/vulkan/LICENSE.md](third_party/vulkan/LICENSE.md) and notices in individual headers |
| DXVK | [third_party/dxvk/LICENSE](third_party/dxvk/LICENSE) |
| Modified optional DXVK/FFR backend | [license](third_party/dxvk-ffr/LICENSE), [upstream revision and patch order](third_party/dxvk-ffr/UPSTREAM.txt), local patches in the same directory |
| NVIDIA NIS, AMD FidelityFX CAS and SMAA | [combined notices](third_party/upscalers/UPSCALER-LICENSES.txt); notices in individual headers also apply |
| NVIDIA FXAA | [third_party/upscalers/fxaa/LICENSE.txt](third_party/upscalers/fxaa/LICENSE.txt) |
| SMAA, iryoku contributors | [license](third_party/upscalers/smaa/LICENSE.txt), [source record](third_party/upscalers/smaa/README.md) |

Inno Setup is an external tool used to compile the installer; its compiler is not bundled. No NVIDIA DLSS/NGX runtime is supplied as a working release feature.

Gothic 3, its original artwork, trademarks, native binaries and other commercial game content remain with their respective rights holders. A separately purchased game is required. The GPL declaration for project code does not grant rights to redistribute the commercial game or its decompiled source.

Custom visual assets and main-menu music under `assets/` are creator-supplied presentation material, not covered by a blanket grant under the project's code license. Files derived from original game artwork retain the original rights holders' interests. Confirm applicable asset permissions before redistributing or reusing those assets separately.

Community Patch, Update Pack, Parallel Universe Patch, Union+, EDD and commercial VR providers are separate dependencies, not part of the project-code license. Free download availability alone does not grant redistribution permission. The release packaging tool collects notices for the components actually distributed.
