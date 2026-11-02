# OpenXR controller compatibility candidates

Local 0.23.99 adds suggested bindings beyond Oculus Touch. These are compatibility candidates based on published profiles, **not certification on untested hardware**. Gothic 3 runs on the Windows PC; a standalone headset must connect through a compatible PC VR runtime.

| Profile | Declaration | Logical button mapping |
| --- | --- | --- |
| Oculus Touch | Core OpenXR profile; existing bindings unchanged | Left X/Y, right A/B; left Menu |
| Valve Index | Core OpenXR profile | A/B on each hand become primary/secondary; main menu through radial |
| PICO 4 | Optional `XR_BD_controller_interaction` | Left X/Y, right A/B; left Menu |
| PICO Neo 3 | Optional `XR_BD_controller_interaction` | Left X/Y, right A/B; left Menu |
| Steam Frame | Optional `XR_VALVE_frame_controller_interaction` | Left d-pad down = X; up/left/right = Y. Right A = A; B/X/Y = B. Left View = Menu |
| Khronos Generic | Optional `XR_KHR_generic_controller` | Per-hand primary/secondary; main menu through radial |

Every profile uses the same underlying actions for sticks, triggers, squeeze, grip/aim poses and haptics. This changes neither movement interpretation nor combat logic. Index/Pimax combinations using Index controllers can use the Index profile; the headset manufacturer alone does not identify its controllers. There is no claim of a dedicated native Pimax controller profile. Controllers with missing or different controls may still require runtime remapping. Traditional Vive wands and single-controller/3DoF devices are not newly declared supported.

## Runtime and fallback behavior

Only controller extensions actually advertised by the selected native runtime are requested at instance creation. Duplicate extension names are removed. Extension-dependent profiles are skipped when their extension is disabled. Touch and Index remain available without additional extensions.

The runtime selects the active interaction profile. A normal rejection of an unsupported profile does not prevent other suggested profiles from attaching. No accepted profiles, or an unexpected runtime error, produces a clear initialization failure. No System buttons are bound.

The existing Meta 32-to-64-bit bridge/protocol/host installation is unchanged. It retains the core Touch/Index suggestions; optional vendor extensions are not assumed across the bridge. SteamVR/VDXR/native runtime paths negotiate extensions from their own advertised capabilities.

`controller_profile_bindings` startup records distinguish `accepted`, `unsupported` and `extension_disabled`, with the numeric OpenXR result. Accepted means the runtime accepted a suggestion, not that physical hardware or tracking alignment was validated. No per-frame profile probing or additional action spaces are introduced.

## Validation

Mock-runtime tests cover all six declarations, original Touch binding preservation, extension gating/deduplication, Index and Frame face buttons, Generic semantic paths, reserved System buttons, unsupported profile rejection, no usable profiles, unexpected runtime errors, focus loss, pose validity, haptics and cleanup. The shared code also compiles for the 64-bit Meta host and its wire test remains compatible. Testing on actual Index/Pico/Frame/Pimax hardware remains pending.

References:

- [Khronos PICO controller profile specification](https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/sources/chapters/extensions/bd/bd_controller_interaction.adoc)
- [Khronos generic controller specification](https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/sources/chapters/extensions/khr/khr_generic_controller.adoc)
- [OpenXR core interaction profiles](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#semantic-paths)
- [Valve Steam Frame controllers and compatibility mapping](https://partner.steamgames.com/doc/steamframe/controllers)
- [Valve native Frame OpenXR input paths](https://partner.steamgames.com/doc/steamhardware/steamframe/input)
