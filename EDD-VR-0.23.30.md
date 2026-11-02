# EDD compatibility — 0.23.30

Inspected the user's EDD Launcher 2.6.0 package without running it. Official
description: https://www.nexusmods.com/gothic3/mods/16 . UPX was used to unpack
analysis copies, and embedded ZIP resources were inspected locally. No EDD
code, DLL, shader, texture or other asset is included in this implementation.

## Findings

- EDD bundles a 2021 DXVK D3D9 DLL (SHA256
  6D6917BF92AE9A03456B2ED4CE0EF5CA712E8154B0FBF3DD0B2D3945D213DE7D),
  ReShade 4.9.1, SMAA, color/sharpening effects, a material patch, an optional
  texture workflow, audio emulation and launcher configuration changes.
- Native launcher code at 0x1408ab1d0 / 0x1408ab48e uses SH / SL as the
  high/low prefetch grid sizes; 0x1408ab8cb / 0x1408ab971 uses SH+CLIP H and
  SL+CLIP L for the high detailed / low-poly clip settings. Vegetation uses VR.
  Local resource and annotated code evidence is under work/edd-analysis.
- The installed EDD profile has SH=40000, SL=280000, VR=20000 and zero clip
  correctors. The latest VR log PID13376 still has native [13,10000] clip depth.
  Its interop is D3D9Ex_shared_BGRA8_to_D3D11; the earlier PID13616 used
  DXVK_Vulkan_import_native_D3D11_BGRA8. This backend change is real, but does
  not establish which change caused the user's reported +10 FPS.

## Actual stereo defect and fix

Engine.dll SHA991cf674a9cf1bc60cddc204be0993e8251336f4d3471543a37846cc6840bf5a
RenderLowPoly (RVA5ec80) temporarily sets near = detailed_far - 3700 and far =
low_poly_distance * (1 - 0.15000000596). It calls virtual slot28,
UpdateProjection, both when switching ranges and restoring the main range.
The VR callback used a frozen projection with the detailed depth range. This
clipped distant geometry even when the engine streamed it successfully.

StereoProjectionUpdate now refreshes only the depth coefficients from the
current native camera clip planes. XR asymmetric FOV, eye position and lens
geometry are preserved. It follows each native pass and its restoration,
rather than extending the near-world projection to a huge global distance.
A once-per-range marker `vr_distant_projection_ready` confirms the actual
pass and its clip ranges in ordinary gameplay logs. No capture request needed.

## Launcher integration

Before a normal game launch, the VR launcher reads the installed G3_edd.ini.
When ULTRA DISTANCE is TRUE and values are within the documented native
distance limits, it imports exactly five existing ge3.ini entries:

| Entry | Current EDD value |
|---|---:|
| Render.PrefetchGridCellSize | 40000 |
| Render.PrefetchGridCellSizeLowPoly | 280000 |
| DistanceHigh.fFarClippingPlane_High | 40000 |
| DistanceLow.fFarClippingPlaneLowPolyMesh_High | 280000 |
| VegetationViewRange.fViewDistance_VeryHigh | 20000 |

It preserves the other quality tiers and UserOptions selections. The user's
current DistanceHigh=3, DistanceLow=3, VegetationViewRange=4 select these entries.
The EDD radius is 2800 metres; the verified native low-poly pass trims its far
plane to about 2380 metres. Detailed geometry extends to 400 metres.

The first pre-import INI is `Ini/ge3.before-vr-edd.ini`. Replacement is atomic
using the existing settings writer; an already matching profile is not
rewritten. If the EDD desktop launcher restores its default ge3.ini, the next
normal VR launcher run reimports the selected distances. Absent or disabled
EDD profiles are skipped; FALSE does not automatically roll back a previous
import. To restore the prior five settings, disable ULTRA DISTANCE and restore
those entries from the backup (or the whole backup if no newer edits matter).
Malformed or excessive profiles stop launch with an error and preserve the INI.

ROI, AI/audio range, game progression, shader settings and renderer DLLs are
not imported. Increasing detailed/vegetation ranges adds engine and GPU work;
this is a visibility improvement, not a verified FPS gain. Tune SH/SL/VR in
G3_edd.ini before launching VR to choose the desired visibility/performance.

## AA and validation

EDD's SMAA runs on desktop presentation; VR eyes are exported before that
presentation. It is not DLAA. No EDD ReShade is applied to VR eyes by this change.
The existing in-game DLAA integration remains unfinished and disabled.

Eight relevant CTest cases pass, including depth-range switching and restore,
the real PUP/occlusion checks, and native EDD import on actual temporary INI
files (validation, backup preservation, no-op repeated import and reimport
after desktop restoration). FMOD compatibility audit passes. Headset visual
validation and a controlled same-scene performance comparison remain pending.
