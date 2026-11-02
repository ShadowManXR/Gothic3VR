# Gothic 3 DX11 static-scene prototype

Current opt-in trial0.23.52: mode4 conditionally replaces ONLY the known Ardea
stairs02 stone DIP after a live per-eye isolated RGB/depth comparison passes.
All other game geometry stays native. Normal launches do not enable the trial.
This is not a completed renderer or demonstrated FPS improvement. See
../../docs/FIXES-0.23.52.md for evidence, precision tolerances and fallback.
Use -CompareOnly for the previous mode3 with all original draws preserved.


Prototype of a potential renderer replacement. Offline probes are isolated;
VR plugin 0.23.41 also contains an explicitly enabled, bounded single-object
comparison trial. Normal game launches still use the existing renderer.
No native game draw is suppressed and no game FPS gain has been established.

Stage 2 now adds diffuse/alpha material previews from the installed game's
actual material graphs and textures. This is still not native shader parity.

Implemented and exercised on the RTX 4080 with a Win32 hardware D3D11 device:

- Read-only, bounded PAK v0/v1 and Genome static-mesh import, patch precedence,
  positions, normals, UVs, colors and native material references. Native
  centimeter coordinates become meters without converting into artist space.
- Twelve original Ardea meshes: 120,402 vertices and 72,080 triangles. Static
  vertex/index buffers occupy 6,644,256 bytes. Geometry uploads once per content
  version and is reused for both eyes and subsequent frames.
- Prepare the union of both eye frusta once; instances visible to either eye
  remain. Group identical meshes for indexed instancing and keep instance data
  unchanged on the GPU if transforms/visibility do not change.
- Direct D3D11 render-target/depth-view input, suitable for subsequent OpenXR
  integration. The standalone test uses independent per-eye targets and real
  stereo camera matrices. It performs no D3D9 replay or color-image transfer.
- Fourteen real material graphs with diffuse texture identity resolved by GUID
  links, not guessed from filenames. XIMG BC1/BC2/BC3 mipmaps are reordered from
  native smallest-first to D3D11 largest-first, with source/output hashes and
  patch/loose-file provenance. Original installed textures, sRGB sampling,
  anisotropic filtering, sampler wrap/clamp/mirror and masked alpha threshold
  are supported for this preview. Native texture content stays resident.
- Transactional content replacement, explicit eviction, bounded geometry
  memory, stale-content/duplicate-entity rejection and stereo ownership guards.
- Native SDK adapter copies entity identity, world transform, resource name and
  material switch from live static contexts. Its DLL import references link
  against the SDK and resolve in the actual installed native binaries. **It has
  now been executed in the live single-object trial.** Native draws remain active.

The GPU test compares 120 repeated original Ardea houses, not the actual town
simulation. Instanced and unbatched color images are pixel-identical. Stereo
draw submissions decrease from 1,440 to 12. Timing data in `results/results.json`
is only a microbenchmark of this prototype; it is not a Gothic 3 FPS forecast.

## Deliberate current limits

Known Ardea materials now use their installed diffuse textures, including
masked roof alpha. Missing materials retain diagnostic colors and a UV checker.
Native lighting, normal/specular mapping, parallax, lightmaps, culling states,
shadows, terrain blending, vegetation and animation remain unimplemented.
A static visual's class or
dynamic flag alone does not prove its material/vertices can be replaced.

The next integration must establish exact mesh/material/LOD identity and
streaming lifecycle, then compatible color **and depth** output for dynamic
native geometry. Suppressing a native color draw while leaving its depth or
shadow pass unchanged is insufficient. Suppressing `Render` alone also does
not eliminate earlier `FillSolidRenderOps`/visibility/queue work.

Preparation sharing must use the union of asymmetric eye frusta and retain
per-eye transparent ordering. Resource arrival/eviction, load transitions and
material-switch changes invalidate the affected cache entries. Lighting data
and world simulation remain native until specifically replaced and verified.

The prototype never sets `materialsExact` or `depthCompatible` as an assertion
about a real game object. It cannot presently authorize native-draw omission.

## Reproduce locally

1. Run `tools/import_gothic_mesh.py --game "<Gothic 3 folder>" --output local-assets`.
2. Run `tools/import_gothic_materials.py --game "<Gothic 3 folder>" --assets local-assets`.
3. Configure with VS2022, `-A Win32`, `-DG3DX11_ASSET_DIR=<absolute local-assets>`
   and optionally `-DG3DX11_SDK_DIR=<SDK checkout>` for the adapter link check.
4. Build Release. Run `G3DX11StaticProbe.exe <local-assets> <results folder>`.
5. `ctest -C Release --output-on-failure` runs the GPU test. Setting
   `G3DX11_GAME_DIR` and a Python interpreter also enables mesh/material import
   regression, malformed-input rejection and native mip-chain verification.

The probe saves stereo images, all-mesh gallery, batching comparison, native
masked/opaque roof comparison and JSON metrics. Its alpha fixture verifies
31,412 transparent pixels using the native mask threshold, no repeated texture
upload when only the mask mode changes, and transactional invalid-update recovery.
Its only writes are in the specified results folder. It does not start Gothic
3, connect a headset, inject a process or load Engine.dll.

## Hybrid native color/depth writeback (isolated)

`NativeWriteback` now transfers a complete DX11 eye/pass back into a native
D3D9 render target. `G3DX11HybridProbe` creates its own hidden D3D9Ex window and
matching-adapter DX11 device. It does not attach to or modify the game.

Both transports are exercised on hardware: legacy D3D9Ex shared BGRA textures,
and the project's DXVK backend using imported D3D11 NT textures in DXVK's own
Vulkan device. DXVK reverse copies have their own acquire-1/release-0 keyed
mutex protocol, bounded GPU fences, external queue ownership and native layout
transitions. The established production forward bridge is unchanged. This
experimental and forward transports share private `src/DxvkInteropResources.h`
image-import helpers. Public forward bridge symbols are compiled only once.

DX11 depth is packed into 24 RGB bits with coverage in alpha, entirely on GPU.
The native PS3 compositor writes both color and `oDepth`, uses the same [0,1]
projection range, preserves existing native foreground/depth/stencil, and lets
later native geometry occlude correctly. Persistent textures, shaders, fences
and state block are reused. Stream 0 is explicitly restored because a cached
state block alone failed to restore it in the actual GPU test. This is intended
as one batched composite per eye/pass, never a transfer for every object.

The caller must provide a dedicated immediate DX11 context, BGRA8 encoded color,
R32 typeless depth with an R32_FLOAT SRV, and a same-size single-sample native
BGRA8/D24 target. Reverse Z, MSAA, HDR, nonstandard viewport ranges and unrelated
projection passes are not supported. The pass stamp binds frame, resource epoch,
eye and camera/projection identity; stale, wrong-eye and consumed passes refuse
to draw. It does not prove that a caller-supplied projection matches the game.
Normal and low-poly native passes must be integrated separately with their own
near/far planes. Material equivalence is still false.

Run `G3DX11HybridProbe.exe <local-assets> <results> native` or replace `native`
with the absolute custom DXVK x32 `d3d9.dll` path. CTest runs both when present.
Tests cover the real textured Ardea house in both eyes, actual native foreground
and later front/back geometry, exact encoded color, stencil, shader/texture/FVF/
sampler/stream/viewport/scissor restoration, seven depth cases with planes
1e-5 apart, 24 GPU-only consecutive frames, invalid pass rejection, and 4K
transport. On DXVK, a separate diagnostic native depth readback compares both
eyes against the source with a maximum tolerance of two D24 quantization steps.
Readbacks are assertions only and are not part of the transport.

Reports include 720p and 4K per-eye transport times, including depth packing and
bounded waits. They exclude the scene render and the native composite draw;
they are not an end-to-end frame measurement or a predicted Ardea FPS gain.
Those tests do not establish native hook execution, streaming replacement or
exact material/lighting fidelity.

## Bounded live-scene comparison trial (VR plugin 0.23.41)

`tools/Start DX11 Trial.cmd` starts the installed game with process-local trial
variables. It checks plugin/Engine hashes and DLAA Off. It never changes INIs,
launches a headset test automatically, or persists environment variables.
`Start-NativeStaticTrial.ps1 -ObserveOnly` renders offscreen; the default mode
also composites the simplified diffuse preview at its native scene position.
The regular launcher keeps this feature off. Use Ardea/New Game and face its
buildings for 15-20 seconds. A different lighting appearance on the selected
building is expected; it is a material preview, not production replacement.

The existing exact-version mesh Render hook copies whitelisted native static
candidate entities, stable property identity, resource/material-switch and world
matrix. A raw context address is only a comparison cookie; only the live
callback argument is dereferenced. No I/O or GPU locks occur in that callback.
The original native Render always runs. On the application render thread, at
the verified main solid-pass boundary, the trial checks that view/projection
and full-target viewport match before using the tested GPU color/depth path.
Native camera translation and projection depth are converted from cm to meters.
Normal/detail and low-poly projections never mix. Unresolved material variants,
dynamic entities, unrelated contexts/passes and menus are ignored.

The trial owns a separate matching-adapter DX11 device/context so it cannot
alter the normal VR blit/UI state. Hardware FFR is disabled only around the
comparison composite and restored for native rendering afterward. Original
native geometry, depth and shadows remain active; no replacement proof is
asserted. It stops after 600 world frames or 60 seconds after object selection,
releases its GPU resources, and restores native rendering on errors. No saved
SDK pointers survive load/menu transitions; identity/context selection resets.
Up to 16 eye records include actual world/camera matrices for offline replay.
Logs are under `results/live-<timestamp>/native-static-trial.jsonl`, outside
the game's recursively scanned resource tree.

Standalone GPU tests exercise the exact live-trial class and actual transport
in both modes/backends, with stereo cm-space cameras and copied native PODs.
They also check mismatched passes, menu disable, and the property/resource gate.
Those standalone tests do not establish actual native hook execution or
headset appearance; the first real-game result is recorded below.

The first real-game trial on 2026-10-21 completed with the Ardea palisade:
508 left-eye and 494 right-eye renders, without a recorded trial fault. This
establishes callback/transport execution, not visual/material equivalence.

### Recorded raster validation and automatic images

`tools/prepare_live_replay.py` converts the bounded trial matrices into a local
replay, verifying mesh SHA256 against the import manifest and admitting only
resolved opaque materials. `G3DX11RecordedRasterProbe <assets> <replay> <output>`
uses an independent D3D9 shader-3 raster with original centimeter transforms,
then compares its silhouette against DX11 in the same recorded eye cameras.
The 16 real 4K eye samples exposed the D3D9/D3D11 pixel-center difference.
`NativeRasterCameraMeters` corrects it without changing source projection/pass
identity or depth. Coverage disagreement decreased from 21,600 to 53 pixels
across 132,710,400 checked pixels. The remaining edge samples are within the
bounded floating-point raster tolerance. This is opaque geometry alignment,
not a validation of native game shaders, lighting or alpha-tested materials.

Comparison mode now automatically saves one native-before/composited-after/
DX11-preview/coverage image set per eye, sampled down to at most1280x720, beside
the log. These are diagnostic GPU readbacks and disk writes, attempted only
once per eye; the affected timing samples are explicitly marked. No images
are read back in offscreen mode or regular game launches. A capture failure
is logged and leaves the trial/native rendering intact. Large targets exceeding
the capture memory budget skip images. Captures distinguish full native scene
color from the simplified preview; they do not assert material parity or permit
native-draw suppression. No repeat manual screenshots are needed.

The initially selected palisade has three opaque materials: stone, wood and tree rings.
Diffuse identities are resolved, but native normals/specular/lighting/shadows
and stone parallax remain incomplete. Native draw omission is still disabled
pending that separate material/pass verification; no FPS gain is claimed.

The second live trial saved all eight images and completed without a trial
fault, but its first palisade sample was hidden behind native buildings in the
left eye and outside the right-eye viewport. Both native-before/after images
were byte-identical, so that sample cannot validate visible material parity.
Version0.23.40 therefore delays selection until the compatible solid boundary.
It ranks projected central bounds across at most64copied candidate/pass records,
with4096maximum context probes per eye while selecting. A chosen entity stays
the same across both eyes and subsequent frames. Offscreen/behind-camera/edge
boxes are rejected; no opaque-visibility or material proof is inferred from
this heuristic. Up to8alternative candidate transforms are logged per selection
for later offline inspection. Native draw suppression remains disabled.

The third live trial confirmed visible composition in both eyes on stairs_02:
971left and547right sampled pixels changed, all within the producer's geometry
coverage, and every changed RGB value equalled the DX11 producer exactly. This
validates the sampled transfer, not native lighting or a performance improvement.

### Original material-state evidence (0.23.41)

The selected object's original Render calls now establish a bounded material
scope. Its native material filename comes from typed, guarded SDK getters. The
existing D3D9 draw hooks observe the actual bound vertex/pixel programs and
float/integer/boolean constant registers just before the original API call.
They also copy declarations, render flags and sampler/texture descriptions,
including sRGB sampling. A foreign nested context masks the outer material.
No native draw is repeated, suppressed or changed. No GPU readback/file I/O
occurs in the draw observer. Only16distinct material/eye records can be captured;
further observation stops after the selected mesh's material/eye pairs are seen.
Shader COM references are released after deferred export at the eye boundary.
This observer is enabled only by the process-local trial, with normal-launch
callback pointers null. It does not enable DLAA or the earlier temporal probe.

`native-materials/materials.jsonl` and content-identified CSO files appear beside
the existing images/log. `tools/analyze_material_capture.py --directory <folder>
--output <report.json>` verifies bytecode identity/token/CTAB bounds and resolves
only constants declared by that exact bound SM3 shader. It includes structured
directional-light/SH declarations when present. Unused stale registers are not
interpreted as lighting. Texture pixels are not copied. This supplies evidence
for the next native-lighting implementation; the diffuse preview is unchanged
and material-equivalence/replacement flags remain false.

Hardware tests cover both backends,16-record budget, two eyes, deferred writing,
nested identity isolation, depth-only refusal, copied constants surviving later
native mutations and exact original bytecode. The production draw-hook test
also verifies all four original draw APIs are observed exactly once without UI
pixel/state changes. The decoder is exercised against the hardware fixture and
13preserved native programs, including structured lighting declarations.

### Deferred native batches (0.23.42)

The 0.23.41 live run selected and composited the object in both eyes but
captured **zero native material draws**. Its Render-context scope did not cover
the later ExecuteInstance draw path. The original native code queues 20-byte
entity records and sorts/batches them before filling shader matrices.

The trial now also observes that existing ExecuteInstance hook. It copies
the selected entity identity/world transform from the queue and obtains the
pool's material through the SDK getter. At the actual draw it recognizes the
exact native vertex program using the existing bytecode-validated position
profiles, matches the selected transform against active shader matrix rows,
and refuses foreign groups, stale inactive rows, unknown programs, and
ambiguous duplicate rows. It never assumes queue order equals shader order.
Captures include deferred-pool provenance and the matched row; the offline
decoder independently checks the matrix against the actual CTAB declaration.

Hardware regressions use a preserved original instancing shader with four
instances, a selected object in row 3, an earlier two-instance group containing
only a stale copy of that row, and a different object's transform. Both native
D3D9 and DXVK must capture exactly the two correct eye records. Optional local
fixtures use `G3DX11_NATIVE_SHADER_DIR`; original CSOs remain local assets.
One bounded observation summary records empty-name, viewport, depth-only,
shader, and matrix refusals so an empty future capture is diagnosable.
The game preview and all native rendering remain unchanged.

### Observed native stone equations (offline only)

The 0.23.42 live trial successfully captured the selected stairs object's
native material in both eyes: `G3_Nature_Wall_Stone_01_F.xshmat`, vertex
program `7a13cd383a26eb4b`, pixel program `26cb84265f99bd7d`. The bound shader
uses SH/directional/rim lighting, DXT5 normal mapping, lightmap, two projected
PCF shadow maps and fog. All six declared sampler slots have sRGB disabled;
the earlier generic sRGB diffuse preview is not evidence of material parity.

`tools/prepare_native_stone_profile.py` checks exact bytecode identity and
exports a bounded SM5 equation reference for this one observed pixel program.
Unsupported programs/instructions are refused. Immediate shader definitions
override stale application registers. It preserves projected depth comparison,
VPOS pixel centers, partial register assignments and both native MRT outputs.
Raw CSO and stereo constants remain local fixtures.

`G3DX11NativeStoneParityProbe` runs that reference against the original native
PS on owned hidden D3D9/DXVK devices and matching D3D11 hardware. Twelve cases
use the captured two-eye lighting constants with controlled texture/interpolator
fixtures covering shadow comparison, normals, both rim branches, alpha and fog.
Both MRTs use float32 targets, so clamping cannot hide errors. Nonzero HDR
output and separate shadow/rim case changes are required. Current measured
maximum RGBA differences are 3.8147e-6 (native) and 7.62939e-6 (DXVK), with
exact auxiliary MRT agreement. A deliberately halved DX11 color is rejected.
These are **equation tests**, not a real-scene texture/geometry equivalence
claim, a headset test, an FPS gain or permission to suppress native draws.

Configure the optional test with `G3DX11_STONE_PROFILE_DIR` pointing to a local
export folder. `tools/export_observed_stone_assets.py` separately preserves
the real mesh's original streams, including tangents and extra UVs, and the
actual diffuse/normal DDS mip chains. `parse_mesh(..., preserve_streams=True)`
retains these bytes without changing the existing compact preview/export.
The following sections describe the decoded additional streams and entity XLMP.
Live atlas placement and sampler-to-asset identity remain explicit unproven
inputs; pointer addresses are never interpreted as resource names.
Production plugin/preview are unchanged by this offline equation work.

### Original vertex/lightmap inputs (offline)

The observed rigid vertex program is now ported with its three native streams,
centimeter transforms, tangent handedness, baked incident direction, two shadow
projections, fog and D3D9 raster-center correction. Packed D3DCOLOR inputs are
swizzled from BGRA. SM3 POW takes the absolute base; the equation generator
preserves this behavior. The otherwise unused TEXCOORD1 interface member must
remain: `NativeStageInterface.h` checks the actual compiled stage registers and
rejects the deliberately broken interface regression.

`import_gothic_lightmap.py` decodes the exact installed version-4 XLMP using the
native Engine Read/Write/FillVertices implementations as references. It preserves
2097 vertex colors/incident directions and reconstructs the stride-24 stream for
the selected stairs: 106 bitmap records, six nonempty bitmaps, 50 atlas-mapped
vertices. Unknown versions/AO grids, malformed counts, duplicate/invalid vertex
groups and invalid scale/atlas sizes are refused. The local shelf-packed atlas is
an offline fixture; its placement is **not** the live global atlas placement.

The scene comparison runs the original VS/PS against the port on the real mesh
and DDS mip chains in both eyes. Native D3D9 anisotropic filtering differs by at
most one UNORM8 step in 52 components with identical coverage. The DXVK comparison
with matching trilinear filtering differs by at most one step in 34 components.
DXVK versus D3D11 anisotropic filtering is still materially different with these
textures (72-step maximum in the recorded isolation). That comparison is **not**
passed, and the game filtering setting has not been changed to hide it.

### Native material GPU resource bridge (opt-in experiment)

`NativeMaterialResources` reads the observed native lightmap and two depth maps
through the existing DXVK device. Depth bytes go through a device-local transfer
buffer and shared BGRA8 transport, then a D3D11 compute shader converts actual
D24/D32 backing data to owned R32_FLOAT comparison textures. The transport can
pad tiny allocations while exposing the original logical dimensions. No texture
pixels are mapped or copied through CPU memory in this bridge. It uses a dedicated
immediate D3D11 context, bounded ownership/fence waits and allocation reuse; an
unsupported native D3D9 backend returns false so native rendering can continue.
Only the isolated probes perform diagnostic CPU readback.

The standalone resource test passes byte/depth checks, repeated updates, source
replacement, small and non-aligned sizes, dimension changes, reconnect, invalid
input refusals and native target/viewport preservation. The initial two integrated
tests intermittently failed with device loss, timeout or an access violation.
`G3DX11DebugProbe` now captures stacks/dumps from an owned isolated child, never
Gothic3.exe. Stage markers located the failure inside the native-source image
copy; a captured NVIDIA stack dereferenced a freed Vulkan image handle.

The controlled old-backend comparison passed eight processes with DXVK memory
defragmentation disabled. The fix in `g3vr-d3d9-stable-interop.patch` instead pins
only image addresses exported by D3D9 `GetVulkanImageInfo`, using the same existing
`stableGpuAddress` path as DXVK's D3D11 LockImage. This prevents the memory manager
from relocating exported handles after the caller queries them. Other eligible
resources retain normal defragmentation and the FFR ABI stays version 1.

With the corrected backend and defragmentation explicitly enabled, all16 tests
pass, including the two former integration failures and96-case ownership stress.
The12-case integrated test also passes12 consecutive processes. Separate actual
FFR tests (including no graphics pipeline library), VR runtime960p/4K FFR,5K
transfer and CAS checks pass. These are offline GPU tests; full live material
equivalence and a performance gain are still unproven.

Local Vulkan validation exposed source images expected in TRANSFER_SRC while
actually in GENERAL. The bridge now materializes the native pass without a layout
change, and records both external copy transitions around the copy itself. The
old Steam-bundled validation DLL no longer reported that source-layout error in
the preserved follow-up, but it also flags unknown newer DXVK extension structures
and can itself crash. This is partial diagnostic evidence, not a clean validation
run or proof that the integration fault is fixed. BGRA transport, padding,
explicit D3D11 fences and SRV unbinding alone did not establish stability.

### Full observed material trial (0.23.43)

The separate trial launcher now selects mode3. It composites the single observed
`g3_myrtana_ardea_stairs_02` stone material using the preserved native VS/PS
programs and actual per-eye constants. Normal launches leave this path inactive;
`-ObserveOnly` retains mode1, and `-SimplifiedPreview` retains mode2. The trial
is still bounded to600 world frames/60seconds and NEVER suppresses native draws.
This is a material comparison, not an optimization/FPS claim.

The original queue's entity/lightmap association was verified in the installed
CP1.75.14 AddInstance1db250 and ExecuteInstance1dabf0 implementations. A borrowed
SDK lightmap item is used only within that original call. After native atlas
setup, `eCLightmap::FillVertices` copies the selected2097-vertex additional stream
with the actual atlas UVs. Its second argument is the VERTEX COUNT, not stride;
the texture-lightmap implementation writes stride24 internally. Count/type,
loaded atlas, bitmap presence, dirty status and all output floats are checked.
Borrowed SDK pointers are invalidated at scope exit and excluded from evidence.
The offline shelf-packed atlas is never substituted for the live native atlas.

Native noise, live lightmap and both shadow maps are copied together on the GPU
with keyed ownership. Original local diffuse/normal DDS mip chains and native
filter/cull states are retained. No CPU image readback occurs in the material
renderer; only the trial's first per-eye comparison screenshots use readback.
Native color remains encoded UNORM, matching the original SRGBWRITE=0 state.
Unknown programs/declarations, stale eyes/frames, unsupported native state and
unavailable lightmap vertices leave the original renderer in control. Refusal
reasons and geometry counts are logged with a small fixed budget.

All17 prototype tests pass with DXVK defrag explicitly enabled. The real live
renderer/capture classes pass12 stereo cases, all three native cull modes and
four-image GPU ownership against the independently executed original D3D9
programs. Coverage is identical;15 RGB components differ by one UNORM8 step.
The exact mode3 trial class also renders/composites six eye frames through its
separate DX11 device, rejects an unrelated pass, captures both eyes and stops in
menus. These tests supply a fixture lightmap provider; the SDK provider itself
still requires the live game test. Matching trilinear filtering is used ONLY by
the fixture. The game's anisotropic filtering remains unchanged and its earlier
kernel variance is not claimed solved. No full-scene material parity is claimed.

## Format references and assets

Read-only format investigation used Baltram's public Rimy3D/mimicry readers:
[mesh reader](https://github.com/Baltram/rmtools/blob/master/mimicry/source/Mimicry/mi_xcmshreader.cpp),
[Genome wrapper](https://github.com/Baltram/rmtools/blob/master/mimicry/source/Mimicry/mi_genomefile.cpp),
[PAK reader](https://github.com/Baltram/rmtools/blob/master/mimicry/source/Mimicry/mi_genomevolume.cpp),
and the local [Gothic 3 SDK](https://github.com/georgeto/gothic3sdk).
Material/XIMG format investigation also used
[material reader](https://github.com/Baltram/rmtools/blob/master/mimicry/source/Mimicry/mi_genomematerial.cpp)
and [XIMG reader](https://github.com/Baltram/rmtools/blob/master/QImageIOPlugin/source/ximgplugin/ximghandler.cpp).
Reference sources remain in the analysis workspace and are not compiled or
bundled. The importer and renderer are new prototype code.

Original game geometry is extracted only from the user's installed copy and
must not be included in a distributable mod. `local-assets/manifest.json`
records archive paths, exact source/output SHA256 hashes and mesh counts.
