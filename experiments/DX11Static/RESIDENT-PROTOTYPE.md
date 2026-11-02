# Resident DX11 opaque pass and instancing — offline prototype

This follows the batch bridge experiment. The renderer can now initialize with
no D3D9 device and render an opaque stone pass entirely on one D3D11 device/context.
Noise, lightmap and shadow inputs are owned DX11 SRVs. Output remains DX11 color
and D32 depth. The resident route performs no native state/texture queries, native
resource acquisitions, reverse writebacks or D24 packing. One end-of-pass DX11
completion query is used by the benchmark; there are no per-object GPU waits.

Identical geometry/material/atlas groups can use one DrawIndexedInstanced call.
Only the three world rows vary per instance. Different camera/material/sampler
state or atlas vertex bytes take the ordinary per-draw resident path. The source
profile is still the one known stone shader pair; this is not general material
translation. Providers must be alive and stable for this immediate call.

## Measured scope

Local repeated captured stairs geometry, shared fixture images, translated groups
of1/8/32, both eyes,720p and4K. The benchmark also verifies rotated/scaled worlds.
Timings include GPU completion and rotate route order after warmup, with9measured
samples per route/eye. Initialization and image comparisons are outside timing.
Gothic 3 was closed. These are wall measurements, not pure GPU timestamps.

Latest4K32instance medians in ms per eye:

| Route | Left | Right |
|---|---:|---:|
| Original native D3D9/DXVK | 0.252 | 0.245 |
| Previous per-object bridge | 41.983 | 40.813 |
| Batch bridge | 1.950 | 1.924 |
| Resident DX11, separate draws | 0.835 | 0.835 |
| Resident DX11, one instanced draw | 0.258 | 0.255 |

The new path removes the bridge regression and reaches approximately native cost
in this bounded workload. It does not demonstrate an engine or Ardea FPS gain.
Measurements exclude native scene preparation, AI, animation, building live DX11
shadow maps, arbitrary materials and a complete final frame. Shared input images
were built before timing, not produced by a new runtime world renderer.
See final-4k/benchmark.jsonl and measurement-summary.json for unrounded results.

## Correctness

5/5 relevant CTest passed. Independent original native shader/geometry comparisons
for both eyes, groups1/8/32, maximum RGB difference1/255, fewer than500changed
components, exact alpha. Additional rotated/scaled world parity passed. Tests
reject mixed eyes/frames/resources, absent shadows, foreign D3D11 device SRVs,
input/output aliasing, wrong target views, unavailable atlas providers and counts
outside1..128. Different pixel constants and atlas data fall back to separate draws.
The resident renderer cannot enter the native bridge path. Partial refused output
stays private. Existing single-object bridge/stereo proof/depth tests still pass.

## Why this is not installed in VR yet

Runtime::Capture copies the final native backbuffer into the shared eye image.
Runtime::CompleteCopies subsequently consumes it in the D3D11/OpenXR path.
A raw opaque pass cannot simply be composited at that point: later native stages
have already modified the image and may have changed color, coverage and depth.
The EndSolid hook is also before further native rendering, not at final display.

Local evidence:
- src/StereoRuntime.cpp: Runtime::Capture, QueueTexture and CompleteCopies.
- REA Engine/functions/0x101a0530.c: FrustumDatabase::Execute calls EndSolid,
  then RenderDecals, later Begin/EndRenderAlpha and further render operations.
- REA Engine/functions/0x1019ba00.c and0x1019aa10.c: native postprocessing executes
  image-filter stages, with behavior depending on the active effect/resources.
REA pseudocode is analysis output; indirect calls are not all resolved. This is
sufficient to reject an unvalidated late raw-color overlay, not to claim a full
recovered render graph.

A game integration still needs owned per-pass scene/material packets, DX11 input
producers, broad opaque coverage and the downstream depth/transparency/effects
pipeline before direct final presentation. The current resident API is the base
for that work; it is NOT a completed DX11 renderer or an OpenXR integration.
Installed plugin remains0.23.54 and DefaultTrialMode0. Game settings/arms/input
remain unchanged. No new headset trial is requested.
