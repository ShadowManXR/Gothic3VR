# Versioned immutable atlas reuse — offline, 2026-10-23

`OwnedStoneAtlas` freezes and validates the known stone mesh's 2097-vertex,
24-byte additional stream. `OwnedStoneAtlasCache` publishes explicit
epoch/source/allocation-generation/content-revision versions. Lookup returns an
immutable shared owner without invoking the SDK provider, copying its data or
reclassifying every float. This changes no native mutation hook: a real producer
which observes every allocation and content write is still required.

The cache is single render-thread only, bounded to 128 source records and 8 MiB
of live atlas payload. Retired versions pinned by a pass count against the same
budget, even across reset. New publications need room for old and new versions;
exhaustion refuses. Invalidation retains a tombstone so the old revision cannot
be republished accidentally. Reset requires an increasing epoch. A source
address is never used as evidence of unmodified contents.

`OwnedStonePass::AppendCached` requires the exact expected version and matching
pass epoch. Packets retain the immutable owner after the producer and cache die.
The renderer recognizes only the private owned-source callback, compares shared
immutable storage directly when grouping instances, and still compares bytes
when different owners are supplied. Borrowed sources retain the original copy
path. Different atlas contents fall back to separate draws.

## Validation and cost

8/8 relevant CTests passed, including original native GPU resource transport,
hybrid depth/occlusion, GPU ownership stress, batch parity, cache revisions and
packet lifetimes. CPU tests exercise 10,000 lookups with one provider invocation,
NaN/Infinity versus packed-color bits, epoch/allocation reuse, tombstones, pinned
budget exhaustion/recovery and source/cache destruction. GPU tests check both
eyes, producer destruction and replacement, exact version refusal, and distinct
atlas contents disabling instancing. Original native parity thresholds are
unchanged: exact alpha, maximum RGB error 1/255, fewer than 500 changed components.

Latest complete 1280x720 benchmark, 32 repeated instances of ONE captured mesh,
9 measured samples after warmup, milliseconds per eye:

| Route | Left | Right |
|---|---:|---:|
| Original native D3D9/DXVK | 0.2453 | 0.2360 |
| Full uncached owned preparation + rendering | 0.8947 | 0.8767 |
| Full cached owned preparation + rendering | 0.2234 | 0.2169 |

Cached timing includes lookup, entity/constants packet copying and validation,
all FOUR same-device input image snapshots, sealing, rendering and final GPU
completion. It excludes the initial atlas publication (one 50,328-byte fixture,
measured separately at 0.0081 ms) and native source-version production. A real
changed atlas pays publication again. Steady-state snapshot GPU allocation count
is unchanged. Zero native captures/reverse writebacks on either owned route.
Instancing is possible here because repeated fixture instances share atlas bytes;
this does not imply arbitrary real scene entities have identical lightmaps.

This is a reduction of experimental preparation overhead, NOT a demonstrated
Ardea/game FPS gain. The small difference versus native is not evidence of a
reliable renderer speedup. Dynamic scene preparation, AI, animation, actual
texture/shadow production and downstream effects are outside this workload.

## Earlier 4K failure — resolved in the following raster alignment step

The 3840x2160 run refuses the old eight-translated-instance lifetime case in eye
1: 125 changed components, maximum error 31/255 versus the native reference.
Rebuilding the pre-change files reproduces the failure. The owned result is
pixel-identical to the immediate DX11 result (zero differing pixels), so cache
ownership is not its cause. IEEE-strict shader compilation and an equal D32F
native depth reference did not remove the discrepancy; both experiments were
reverted. The underlying material/raster discrepancy is not yet identified.

No tolerance was relaxed and failing 4K output was not accepted at this step.
The following raster alignment step identified a half-pixel clip-space rounding
error and fixed it with a fractional viewport, preserving the original projection.
All 36 full 4K route/count/eye image comparisons now match the native reference
exactly. See RASTER-ALIGNMENT.md for the evidence and current remaining limits.

Normal installed runtime remains 0.23.58, DefaultTrialMode 0, no draw suppression.
Only the experimental source/probes are staged. No game launch or headset test
is required for this step. Native source-version hooks, generic durable geometry,
broad materials, paired-eye publication and downstream rendering remain required.
