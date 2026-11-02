# Local core 0.23.132 — controller mapping and adjustable world labels

Launcher adds Controller mapping with 21 named actions, normalized left/right OpenXR inputs, Hold / Single press / Double tap / Hold 0.5 s, optional two-button combinations, conflict / unavailable-input notices and controller-only default restore. Existing Quest-style defaults are retained; poses, locomotion axes, native skills and weapon restrictions stay unchanged. Shared-memory capture reads raw input from the existing game VR session (no second XR instance). Listening suppresses game input with an expiring lease, cancels on timeout/close/page change and requires neutral before resuming. The launcher must remain open; return focus to the VR game to capture. Reassignments apply live after Save settings.

HUD & menus adds World label size, 25–150% in 5% steps, default 100% of the current size. Native focus-name draw is scoped to a cached sibling font for characters/items/enemies/signs; dialogue/inventory/menu fonts and health bars are unchanged. Existing distant ranged-name reduction is composed with the new scale, including native holster fade. No entity scan, extra ray or added render pass. Headset validation pending. No installer built.

# Local core 0.23.131 — original native aiming reticle artwork

Bow, crossbow and magic keep their tracked aim, stabilization, world projection, ray tests and visibility toggle. Their shared reticle samples the installed native G3_HUD_CrosssHair.tga through eCImageAdmin atlas metadata. No copied image, additional ray, render target or per-frame texture upload. A textured quad replaces the procedural diamond, retaining native colors and alpha. Native atlas lookup is cached; frame references are released after the second eye or reset/fault. Previous reticle is a fallback if the graphic is unavailable. Compiled; headset visual confirmation pending. No installer.

# Local core 0.23.130 — native landscape menu canvas, square VR world

Reverts 0.23.129 compositor-only fitting. Native menus now lay out and draw in a 16:9 viewport within the existing UI texture. XR submits the same landscape subrect at the existing menu width; controller pointers and desktop mouse conversion use the matching canvas. The full square native viewport is restored before world processing and after menu drawing, including failed capture paths. HUD, world targets/projection, cinema, native assets and calibration remain unchanged. No extra render target/pass or installer.

# Local core 0.23.129 â€” fit menus at square VR resolutions

Menus use the existing 16:9 physical panel height as an upper bound and shrink uniformly for taller eye textures. All native UI pixels, aspect, placement settings and hit testing remain coherent. Controller menu rays use the same shared extent calculation as the actual XR quad. HUD, world resolution/projection, radial, cinematic sizing and native GUI layout are unchanged. No extra render target or pass. Plugin-only update; no installer.

# Local core 0.23.128 â€” adjustable world fog intensity

Graphics adds Fog intensity, 0â€“100% in 5% steps, default 50%. Save settings applies live through the verified native OnFogChanged uniform update and native fog-cache invalidation. 100% preserves existing fog, including Reduced Fog; 0% clears the native world-fog ramp. Terrain, objects, vegetation and water share the native fog parameters. Sky colors/scattering, draw distances, ROI, weather/save data and shader archives/cache remain unchanged. No added render pass or per-draw scene work. Compiled plugin and launcher; headset validation pending. No installer built.

# Local core 0.23.124 â€” independent staff/halberd and crossbow grip loss distances

Add Grip loss distance to Staff / Halberd and Crossbow, each independently saved as Stereo/StaffGripLossDistanceCm and Stereo/CrossbowGripLossDistanceCm. All three weapon families now default to 40 cm with 10â€“100 cm bounds and 5 cm steps. Staff support passes its own limit to the existing physical-to-constrained-palm check; crossbow passes its limit through the cached frame attachment before both eyes. Changes apply live without resetting an attached support hand unless the newly selected limit is exceeded. Three relevant rebuilt tests passed, including independent settings, actual crossbow contact boundaries, live limit changes and non-default world units. Interaction/layout checks passed on all 17 pages in normal and compact launcher sizes. No installer rebuilt; all existing user calibration and creator preset data preserved.

# Local core 0.23.123 â€” configurable 2H grip loss distance

Add 2H melee â†’ Grip loss distance, saved under Stereo/TwoHandGripLossDistanceCm. Missing keys default to 40 cm, configurable from 10 to 100 cm in 5 cm steps, with live Save settings application and no support reset merely from editing the value. The check measures physical support-hand displacement from the locked weapon contact. Staff/halberd and crossbow retain the accepted 30 cm limit. Existing weapon handling, loading continuity and primary-wrist protection remain. Setting bounds/round-trip and grip-distance tests passed; launcher interaction/layout checks passed for all 17 pages at 1320Ã—940 and compact 1320Ã—820. No installer rebuilt; creator preset file unchanged.

# Local core 0.23.122 â€” support continuity and dominant-wrist protection

Keep established 2H/staff and crossbow support through a same-item/session render timestamp gap rather than treating a >100 ms loading stall as a release. Resynchronize button edges across such gaps; never queue a hidden toggle click. Same-player rig rebuilds retain the support constraint and convert its orientation to the new left-hand calibration. Full lifecycle/context/item/session invalidation still cancels support, and combat stroke histories retain their independent freshness gates.

Release 2H/staff/crossbow support when the physical support contact moves more than 30 cm from the actual constrained contact. Inter-controller separation alone is not the criterion, since long staffs legitimately separate the hands. Standard 2H swords/axes now solve absolutely from the captured right-controller frame, exclude all support-wrist orientation input and constrain positional support steering to a 45-degree cone relative to the dominant hand. This prevents accumulated wrist twist and backwards bending while allowing both hands/player yaw to rotate the complete assembly. Degenerate coincident controller centres retain the last rigid orientation until a usable direction returns; the contact-distance release remains active. Staff/halberd keep their existing orientation solver. Ten rebuilt relevant tests passed, including multi-second gaps, left pitch/yaw/roll, contact release at distance and return without automatic reattachment. Headset validation pending. No installer rebuilt.

# Local core 0.23.121 â€” close melee, armor bindings and support handling

Correct the native overlap capsule adapter: the engine halves bCCapsule height internally, so pass the full cutting-segment length rather than its half-height. Keep contact radius, sweep count, occlusion, velocity/distance/stamina gates and per-stroke hit limits unchanged. On a same-player armor skeleton rebuild, convert cached weapon bindings and staff profiles between the old/new calibrated palm frames instead of recapturing a transient native animation. Six inverse-bind landmarks also detect in-place hand-rig changes without scanning bone names every frame.

Standard two-handed swords/axes preserve their acquired pose, compensate the controller-centre/shaft offset, follow the resulting two-hand line and take shaft roll from the dominant wrist. Independent support-wrist rotation no longer steers the blade. Staff/halberd orientation retains its accepted solver. Optional Camera & controls â†’ Toggle support grip (2H / staff / crossbow) makes left grip attach/release on deliberate clicks at the real support contact. Hold remains the default. Context/tracking/item loss cancels attachment; raw shield block and bow-string inputs are unchanged. No installer rebuilt. Twelve relevant tests and launcher interaction/layout checks passed; headset validation of these changes remains pending.

# Local core 0.23.120 â€” optional first-person Hands only

Camera & controls adds Hands only (hide body), default Off for existing and new configurations. The setting saves independently of fixed tracking/camera features and applies live. Reuse the existing deep-crouch render-palette mask: keep tracked hand/finger matrices and held-item poses, collapse the other hero skin weights in the current render scope, and restore the original palette after each eye. An explicit first-person gate prevents the option from hiding the third-person body in conversations, recovery, activities or ordinary third-person view. Camera calibration, collisions, locomotion and crouch behavior remain independent. No installer rebuilt; 0.23.119 remains the most recent packaged installer. Headset validation is pending.

# Authorized installer refresh 0.23.119 â€” 30 October 2026

The user requested an installer containing the latest left shield grip change and removal of old installer packages/very old project backups. Preserve the frozen creator preset, recent rollback snapshots and all managed uninstall/recovery originals. The ranged-name fade is headset-confirmed; the new shield input still needs headset validation.

# Local plugin 0.23.119 â€” left shield grip block, melee only

Left grip supplies an additional native block request exclusively with a drawn right-hand 1H melee weapon and left-hand shield. The same raised-shield height gate, native skill/stamina/parry rules, icon and left-hand haptics apply; right grip remains supported. Left support grip for 2H/staff/bow/crossbow and menu/trade input do not become Use2. Both right- and left-hand bow/crossbow loadouts reject parry; the existing ranged-weapon suppression is reused before guard decisions, without another weapon lookup. Physical melee suppresses hits during the left shield block wind-up just as for the existing right grip. No installer is automatically rebuilt; the 0.23.118 package remains immutable. Headset confirmation is pending.

# Local plugin 0.23.118 â€” retain ranged label size during native sheathing fade

The user confirmed the local 0.23.117 trial, then spotted a large-name flash when sheathing the crossbow. The native HUD can draw its old ranged focus after the current weapon/focus query has changed. Keep the half-size font for that exact previously captured distant NPC during a bounded two-second fade window. An actual close interaction target clears the retained style immediately, including the same NPC; a different entity never inherits it. Empty queries do not refresh the deadline. Existing lifecycle/menu gates and native font restoration remain. No added raycast, entity scan, weapon lookup or rendering pass; non-target labels return before the new time check. Plugin-only local update; no installer rebuilt. The user confirmed the sheathing transition is correct in the headset on 29 October 2026.

# Local core 0.23.117 â€” scoped ranged names and required startup splash

Only the current automatic bow/crossbow focus target, when it is an NPC farther than max(2 m, configured interaction reach) from the aim origin, uses a half-size native font. The original DrawFocusName renders it with a cached sibling font, restoring its renderer font index in finally. The current update serial/pointer gate excludes other targets, nearby names, ordinary interaction, magic, stale frames and menus. The existing native health bar is unchanged; there is no additional world scan or raycast and no per-label weapon check. Native theme ownership handles the additional font resource.

The Steam splash had returned to the exact original bitmap. Reapply the known G3 VR bitmap while retaining its original external uninstall backup. The splash is now a required core payload file. Protected Union+ sessions snapshot splash.bmp and Ini/logo.ini without holding write locks on those two non-code assets, then restore the exact saved VR bytes before completing. This permits Union+ artwork replacement without an access-denied installation error. Launcher and guard are rebuilt for that protection. Thirteen targeted CTests and 26 isolated deployment checks passed, including core-only splash installation and original restoration at uninstall. Local core files were applied to Steam with all native INIs and calibration preserved. The user confirmed the 0.23.116 ranged health focus works; the 0.23.117 font appearance still needs a headset check. Existing 0.23.114 installer artifacts are immutable; no installer is automatically rebuilt for this local core update.

# Local plugin 0.23.116 â€” ranged-focus crash correction

The user reported a crash when aiming at a distant enemy. Saved Lastlog_GE3.log for PID 20100 maps EIP 02EFE02F to Game.dll RVA 6E02F in FindFocusEntity. Disassembly is `fld dword ptr [edx+edi*4]`; EDX and EDI were both zero. The native ray branch read a missing distance report. Local 0.23.115 had incorrectly promoted the automatic false query to true.

Preserve the original crosshair argument for every native call. Bow/crossbow automatic focus uses the native candidate branch (temporarily Hard rather than Shape when necessary), without the VR close-interaction reach filter. Native ranged scan limits, priorities and line-of-sight rules remain active. The original enemy HUD bar consumes that native focus; no custom health bar is created. Evaluated projectile-direction alignment and removal of redundant weapon/camera work are retained. Ten targeted regressions passed, including all crosshair/ranged/magic argument combinations. Headset validation remains pending. Steam is the active target; no installer rebuilt.

# Local plugin 0.23.115 â€” ranged enemy health focus

Performance follow-up: the caller now passes its weapon classification into the aim builder. With a ranged weapon this reduces two NativeBow::Weapon calls and one NativeMagic::Selected call to one weapon lookup and no spell lookup per focus query. The existing world-space weapon sample returns before recalculating hero position and comfort-camera anchoring. Non-player focus dispatch remains pointer-only after its existing per-application-frame resolution. There is still exactly one original FindFocusEntity invocation per successful hook call; ranged focus changes that invocation to the native ray branch instead of the interaction candidate path. The already-existing reticle query is unchanged. No distances, ROI, background worker, extra per-eye operation or runtime profiler is introduced. Ten targeted regressions passed again; no in-headset FPS comparison was recorded, so no numerical FPS guarantee is claimed.

Native AutoCollectFocus invokes FindFocusEntity(false). VR previously treated that as short-reach interaction even with a bow/crossbow, clearing the persistent focus read by gCGUIManager::OnProcess. Drawn ranged weapons now use the native crosshair ray branch for that automatic query as well. Existing explicit crosshair queries, close-range interaction gates and magic query behavior are retained. Focus camera orientation is built from the evaluated projectile direction, including string-hand / support-hand corrections, rather than the uncorrected controller matrix. Camera state is restored in the existing scope. No extra per-eye trace, custom health overlay, ROI/distance increase or native module edit is introduced. Ten targeted tests passed; headset validation of the health bar is pending. The user returned to Steam during deployment. The GOG attempt stopped before changing files because its active managed journal no longer matched; the update was applied to the Steam installation. No installer package rebuilt.

# Game edition validation â€” 2026-10-29

Bogdan confirmed both GOG and CD/retail tested and working, alongside Steam, with the required native patch profile. Startup videos were not skipped on the tested retail installation; the user accepted this cosmetic limitation and did not request a fix. These are the creator's reported test results, not a separate automated certification of every storefront/language build. The new G3 VR native startup splash was also confirmed working. No installer rebuild is triggered by this documentation update.

# Pending artwork addition â€” native startup splash

The existing approved G3 VR logo is converted to the native 300x300 RGB24 splash.bmp format, retaining the engine's exact 0x808080 transparent color key. Local application journals and externally backs up the original splash for uninstall. Future packages include it in the optional branding component. The already-built 0.23.114 distribution remains unchanged; no installer has been rebuilt for this artwork addition.

# Authorized package refresh â€” 2026-10-29

The user explicitly requested a new installer package for users, lifting the earlier packaging hold for this build. Package 0.23.114 uses the current validated production plugin, launcher 0.23.107, guard 0.23.105 and unchanged frozen creator preset. No live game deployment is part of this packaging operation. Validation receipts accompany the output.

Local headset feedback confirmed that the 0.23.114 world-gap fix removed the remaining Vengard flashes/black frames. The 0.23.113 auxiliary camera exclusion and 0.23.109 native GUI-canvas exclusion are retained. All development capture/profiling flags are OFF.

## Local plugin 0.23.109 â€” remove native GUI canvas tint from XR panels

Actual clean-runtime UI captures identified the first fixed-function GUI quad: atlas2048x2048,FVF0x142,standard source-alpha blend,alpha test,on full viewport,no shaders. It wrote RGBA(1,0,0,10) at nine empty panel samples;88.6%of the HUD image had alpha10. The same first pass appears before menu artwork. Exclude only that initial base-canvas replay using this narrow profile; native desktop calls, later same-atlas sprites, opacity of real widgets and menu artwork remain unchanged. No global alpha threshold, color correction or extra GPU full-screen pass. Paired-eye streaming change0.23.108 retained. Production build disables the temporary UI composition capture option. Headset confirmation still required; no installer built.

## Local plugin 0.23.108 â€” paired-eye optional streaming publication

The existing exact-profile PUP optional-work hook no longer runs optional publication during the right-eye render. Work remains queued for the next pair with the same 2 ms shared allowance. Forced loading, native background workers, per-eye frustum/culling and ordinary non-stereo work remain native. This closes a code-level opportunity for different resident resources between eyes; it does not establish the cause of Vengard dome flicker without the matched headset test. No distances, effects or Meta settings changed. The horizontal color seam is still under investigation; current user trial is Quest Link, FFR Off, SMAA Medium. No installer package built.

## Local launcher 0.23.107 â€” Union+ only optional-mods page

Removed the Item Locator information/credit card from the launcher. Optional mods now displays only Union+ information and supported Nexus pack version 1.7.3. The Locator controller shortcut remains in docs/ITEM-LOCATOR.md; runtime support is unchanged. Executable fingerprint details stay internal and are no longer presented as package-release versions. No installer package built.

## Local launcher 0.23.106 â€” explicit supported Union+ versions

Optional mods now states pack Union+ 1.7.3 and starter builds 1.2026.10.28 (recommended), 1.2026.10.10 (also supported). START-HERE and protected Union+ instructions contain the same distinction. Display/documentation change only; no change to the accepted checksum policy, plugin, guard, runtime or existing settings. No installer package built.

## Local 0.23.105 â€” Union+ updater resilience and Locator ownership

No installer package built. Separate Locator import/settings controls retired; Union+ owns the plugin. Existing live Locator files/config remain unchanged. Runtime accepts exact original 1.0.7 and Union+ 1.0.8 profiles and follows the native configured hotkey. Current author starter 1.2026.10.28 audited; durable verified sibling copy replaces reliance on the updater-deleted `_old.exe`. Session update preference is journaled and restored, including interrupted-session recovery. Core file locking, job restrictions, native INI reconciliation and Quick Loot policy remain active.

# Previous packaging hold (lifted for the explicitly requested 0.23.114 package)

The user requested on 2026-10-27: do not build another installer or release package until explicitly asked. Continue local fixes and appropriate tests. Do not run Build-Package.py or ISCC as part of routine fixes.

Pending for the next authorized package: the native startup advertising playlist (`Ini/logo.ini`) is empty. No Bink video files are changed. The source deployment tests cover skipping logos, preserving story/credits files, restoring the original playlist on uninstall and retaining a playlist subsequently edited by the user.

The current local game has received this one-file change separately, with an external original backup and an owned-file entry in its existing uninstall journal. This local operation does not require reinstalling the VR package.

Local 0.23.93 adds an optional local importer and VR controller shortcut for eCDynamicEntity Locator 1.0.7 (fyryNy). Original third-party DLL/icons remain outside the release sources; import requires the user's own download. No installer has been rebuilt. See docs/ITEM-LOCATOR.md.

Local plugin 0.23.95 adds Locator HUD projection alignment. The launcher stays at 0.23.93; no launcher settings or installer have changed. The third-party DLL remains byte-identical.

Local launcher 0.23.96 exposes nine separate Locator categories. Bogdan's existing profile hides plants, living NPCs, animals/monsters and quest-kill targets, retaining other selections including lootable corpses and quest items. Existing radius and plugin 0.23.95 are preserved. No installer rebuilt.

Local 0.23.97 adds render-only third-person camera orbit and progressive physical body crouching with deep-crouch hands-only visibility. Existing centering/calibration and Locator settings are preserved. Bogdan validated both features in the headset. See docs/CAMERA-ORBIT-PHYSICAL-CROUCH.md. No installer rebuilt.

Local launcher 0.23.98 removes the creator-approved camera/tracking feature switches, including controller input. These eleven features remain enabled when loading/saving launcher settings. Camera position, turn speed, runtime selection and all unrelated settings remain adjustable and unchanged. Plugin 0.23.97 remains installed. No installer rebuilt.

Local 0.23.99 declares Index, Pico 4/Neo 3, Steam Frame and Khronos Generic controller binding candidates alongside unchanged Touch bindings. Optional profiles are gated by advertised extensions; expected unsupported-profile errors do not block other profiles. These additional hardware paths are not headset-validated. The Meta bridge/protocol/host installation and calibration remain unchanged. See docs/CONTROLLER-COMPATIBILITY.md. No installer rebuilt.

Local launcher 0.23.100 merges all Bow grip offsets into Bow, removes the Motion page and keeps InstantControllerTurn enabled as fixed behavior. Navigation now contains 17 categories; numeric field routing and native INI hover help use named page identifiers. Normal/compact UI checks verify every numeric field remains visible on its intended page. Saved calibration and plugin 0.23.99 remain unchanged. No installer rebuilt.

Local 0.23.101 adds a live FFR Transition width control (0â€“30%, default 10%). Static ordered coverage gradually changes hardware shading outside the unchanged full-detail oval; zero preserves the legacy pattern. No DXVK backend, runtime, tracking calibration or installer changes are required. Headset appearance validation is pending. See FFR-0.23.8.md.


Local 0.23.102 removes the 0.23.101 ordered tile transition following headset feedback. The original contiguous hardware FFR rate map is restored. Transition width now controls a real per-pixel, linear-color blur after scene AA/upscaling, with zero-width bypass and UI/cinema exclusion. GPU, real DXVK runtime and launcher checks passed; Bogdan confirmed headset appearance. Existing backend, runtime and calibration are preserved. No installer rebuilt.

Local 0.23.103 adds native Walk while physically crouching: enter after a sustained 30 cm headset drop, exit within 18 cm, with 120 ms dwell. It cancels the sprint latch without modifying direction or activating native Sneak. Tracking origins, visual crouch behavior, FFR, settings and special movement handling are preserved. Automated checks passed; headset validation is pending. See docs/CAMERA-ORBIT-PHYSICAL-CROUCH.md. No installer rebuilt.

Local guard fix 0.23.104 handles an updated UnionStarter.exe by selecting the sibling UnionStarter_old.exe only when its SHA256 matches the already-audited 1.2026.10.10 binary. The new 1.2026.10.28 executable is not approved or altered. Protection/recovery tests and read-only resolution against the user's actual files passed. Only Gothic3VRUnionGuard.exe is updated; launcher/plugin 0.23.103 and all settings remain unchanged. No installer rebuilt.
