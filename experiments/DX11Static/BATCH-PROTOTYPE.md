# DX11 pass batching — offline prototype, 2026-10-22

Implemented a bounded RenderBatch path for the known rigid stone profile.
It acquires the common lightmap, shadow maps and noise once, keeps them owned
through all DX11 draws, preserves the group's depth buffer, then returns resource
ownership once. The probe transfers the completed color/depth result once per eye.
Single-draw behavior is unchanged. No installed game call site uses RenderBatch.

## Measurements

3840x2160, 32 translated instances of the captured stairs mesh, shared material,
9 measured samples per route/eye after warmup; route order rotates each iteration.
Times include final native GPU completion; readbacks, startup and validation are
outside the timed samples. Gothic 3 was confirmed closed for the final run.

| Route | Left median ms | Right median ms |
|---|---:|---:|
| Native D3D9/DXVK | 0.156 | 0.154 |
| Previous per-object DX11 bridge | 42.28 | 41.50 |
| Batched DX11 bridge | 1.99 | 1.92 |

Exact unrounded measurements and breakdowns: final-4k/benchmark.jsonl.
For the batched route, rendering plus material feed takes about1.31-1.35ms,
writeback0.48-0.51ms and final native completion0.12ms. These wall intervals
include synchronization; they are not pure shader GPU timestamps and their
separate medians do not necessarily sum to the total median.
Resource acquisitions and result writebacks fall from32 each to1 each per group.

This proves a large reduction in the EXPERIMENT's overhead, not an FPS gain in
Ardea. Native scene preparation, AI, animation, arbitrary materials, complete
static-scene coverage and the second engine traversal are not represented.
Textures/shadows are the existing fixture inputs. Only one known mesh/profile
is repeated; actual Ardea does not have32 interchangeable captured stairs.
The remaining bridge remains slower than native in this workload.

## Correctness and scope

Both eyes tested at720p and4K, groups1/8/32, overlapping translated instances,
RGB comparison against independent original native draws (max1step, <500 changed
components), exact target alpha, mixed-eye/frame/resource refusal, bounded count,
unavailable second atlas, native scene unchanged after partial offscreen refusal,
and resource ownership recovery on a subsequent successful call.
5/5 relevant CTest regressions passed, including the original material path,
stereo-proof selection and native/DXVK depth/occlusion.

Inputs/providers must remain alive and stable for the immediate RenderBatch call.
It is NOT a deferred SDK queue: a future queue must own atlas vertex data and pin
resources, preserving their content version at the original pass. On refusal,
private output may be partial and must never be committed to the native scene.

## Next architecture decision

Do not enable this batch path in the game merely because it beats per-object
interop. The measured remaining cost is material transport plus reverse transport.
Investigate keeping whole passes resident on the DX11 device and consuming their
output without returning each group to D3D9, while preserving later native
postprocessing/depth and stereo. Alternatively, replacing engine submission must
remove its duplicated traversal; a draw interceptor alone does not do that.
This requires a defined pass boundary, durable scene/material packets and broad
material coverage. No performance gain is promised for the engine CPU bottleneck.
Installed plugin stays0.23.54, DefaultTrialMode0, DLAA Off; no new headset trial.
