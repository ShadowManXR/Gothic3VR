# Owned opaque pass packets — offline prototype

Implemented OwnedStonePass and OwnedStoneImages for the single verified stairs /
stone material profile. The renderer already owns the imported geometry buffers;
packets own copied entity metadata, native constants/declaration/sampler metadata
and the additional atlas vertex bytes. The borrowed SDK callback is invoked only
by Append, before the original scope exits. Deferred views reference stable owned
packet storage and remain correct after moving the pass object.

OwnedStoneImages snapshots the four DX11 input images on the same immediate
context. Holding an SRV alone would not freeze its pixel contents. GPU copy
commands are ordered before later input updates and renders, without a CPU
readback, explicit flush or wait inside snapshot capture.

OwnedStoneImagePool reuses a bounded set of snapshot images after all packet
owners release them. Pinned snapshots are never overwritten; exhaustion refuses
rather than blocking or corrupting an earlier pass. Matching slots are reused
without new GPU allocations. Each slot is bounded to8M source pixels; default
pool size4, maximum8slots. Passes have a128draw maximum and8MiB charged packet
payload budget. Container/allocator overhead is not process working-set accounting.

The pass lifecycle is collecting -> sealed -> consumed, with sticky rejection.
An unknown shader/mesh, unexpected geometry, mismatched entity/world transform,
duplicate entity, source failure or count/byte overflow invalidates the whole pass.
Render requires exact frame, eye, epoch and camera stamp. Sealed or stale packets
cannot be appended to or replayed into another pass. Private partial output must
never be published after failure.

## Validation

6/6 relevant CTest passed. Both-eye lifetime tests destroy scoped SDK providers,
mutate AND release all original input images, move the pass object, then compare
its image with an independent native reference. Native alpha is exact; RGB
maximum1/255 and fewer than500changed components. Tests cover stale stamps,
wrong mesh/world/program, duplicate entities, bounded CPU/count, empty/unsealed
passes, source NaN/Infinity (with packed color excluded), pool exhaustion, pinned
ownership, slot reuse and release after rejection. Existing material, native and
DXVK depth/occlusion tests still pass. Full test and captures under this directory.

The x86 compiler emitted __fdclass calls for the per-atlas-float finite checks.
Checking copied IEEE754 exponent bits preserves NaN/Inf refusal and removes
hundreds of thousands of CRT classification calls in the32packet test.

## Cost, including preparation

The earlier ~0.25ms resident-instanced result measured rendering PREPARED packets.
A new route includes image snapshot commands, packet copying/validation, sealing,
rendering and final GPU completion. It performs four same-device image copies per
pass and zero native resource captures/reverse writebacks. The test asserts zero
new GPU snapshot allocations after warmup.

4K,32copies of the captured mesh, medians per eye in the optimized run:
- Native drawing: about0.25ms.
- Resident instanced drawing of prepared data: about0.25ms.
- Complete owned packet preparation plus resident rendering: about0.85ms.

These are bounded synthetic microbenchmarks, not Ardea FPS or a complete engine
renderer. Source image production, visibility, AI, animation and downstream
postprocessing remain outside the workload. The first measurements were affected
by another game at99%GPU usage and are excluded from performance conclusions.
User confirmed that external load; the later optimized run started at5%GPU usage
(the preceding unoptimized run started at0%). See gpu-before-optimized.txt and
optimized-4k/benchmark.jsonl for the actual environment and unrounded values.

## Integration limits

No live engine hook uses this queue, no native draw is omitted, and no new trial
is enabled. Only the cached known geometry/profile is supported; live native
geometry-content parity still needs proof. Inputs are frozen from the DX11 images
provided by the caller, not recovered from arbitrary native material producers.
A future collector must bind every packet to the correct source-content version,
camera/pass boundary and full material/geometry coverage. Native texture pointer
identity alone is not a content-version guarantee. Cross-eye transactional
publication and downstream depth/transparency/effects remain required before VR
integration. Installed plugin0.23.54, DefaultTrialMode0 and all game settings stay
unchanged. No new headset test is required for this offline stage.
