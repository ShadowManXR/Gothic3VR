# Native INI tweaks

The launcher's **INI tweaks** page edits the selected game's `Ini/ge3.ini` and the Windows Documents `Gothic3/UserOptions.ini`. Opening the page, reloading it or saving an unchanged page does not apply a preset. Existing values are the initial values; the creator's VR Defaults preset is unchanged.

The page offers:

| Control | Native setting |
|---|---|
| High-detail draw distance | Active `DistanceHigh.fFarClippingPlane_*` tier |
| Distant landscape distance | Active `DistanceLow.fFarClippingPlaneLowPolyMesh_*` tier |
| Vegetation draw distance | Active `VegetationViewRange.fViewDistance_*` tier |
| Detail / distant streaming grids | `Render.PrefetchGridCellSize` / `Render.PrefetchGridCellSizeLowPoly` |
| NPC / creature ROI | `Entity.ROI` |
| Engine frame-rate limit | `FpS.Max` |
| Vegetation density | `VegetationQuality` |
| Shadow quality | `ShadowQuality` |
| Texture filtering | `TextureFilter` |
| Depth of field / film grain | `DepthOfField` / `Noise` |
| Native shader cache | Inverse of `Render.DisableShaderCache` |

Hover controls for the key and its effect. Distances are native game units (100 units = 1 metre). Increased distances, streaming grids or ROI are not advertised as performance improvements. ROI affects dynamic entity processing; it is not a replacement for landscape distance. Native clamping of ROI to the detailed-world far plane still applies.

The editor reads the effective graphics preset and its active detail tiers. It does not silently force all tier values to match. Distant scenery that is Off stays Off. If applying edits converts a fixed native preset to Custom, the preset's other effective native options are carried over. Inactive tiers, unrelated mod keys, comments, controls and language settings are retained. Missing, malformed or out-of-range values are left alone and disabled in this curated editor.

**Save settings** applies edited keys only, with Gothic 3 and protected Union+ closed. Changes require a game restart. A changed value that was edited externally while the launcher was open triggers a reload request. Windows permissions/locks and destination changes are checked. Original files are copied and verified outside the game resource tree before replacement; ordinary commit failures roll back already-replaced files. The external originals remain available even if recovery itself cannot complete.

**Reload INIs** discards unsaved edits on this page, without resetting VR calibration. **CP defaults** stages native CP distance/grid/ROI/cap/cache values for the active tiers and the exposed Very High graphics defaults; it does not restore the entire ge3.ini or apply until saved. The ordinary Defaults button on other pages continues to restore the creator's VR preset.

**Open backups** shows `%LOCALAPPDATA%/Gothic3VR/IniTweaks/<installation-key>/<save-session>/`. Backups are named `ge3.ini` and/or `UserOptions.ini` according to the files changed in that save. To undo a save, close Gothic 3 and the launcher and copy the applicable original back to its original location; preserve later edits you want to keep. Native settings are user preferences and remain after removing the VR mod.

After explicit distance/grid edits (or CP defaults), `[Gothic3VR.IniTweaks] OwnDistances=1` records that the native INI values take precedence over the legacy EDD distance import. This prevents launch from overwriting the changes just made. To return to importing a separately installed EDD profile, close the launcher and remove this flag or restore the earlier ge3.ini backup. Other EDD/native engine behavior is unchanged.

Old memory/cache-size and removed renderer keys are deliberately absent. CP's included `Changelog.txt` states that resource cache is calculated automatically from v1.75. The installed CP `ge3.ini` comments provide the supported native keys, tiers, ranges and ROI behavior; `UserOptions.ini` provides the current effective selection.

This is a launcher-only change. No VR rendering plugin, game binary, texture, distance or gameplay setting is modified merely by installing/opening the updated launcher. Installer packaging remains on hold until requested.
