# Native pixel alignment at 4K — offline correction, 2026-10-23

The high-resolution failure was a coverage error, not corrupt cache contents.
Only pixel (39,823) exceeded the original 1/255 RGB limit in the failing eye:
native clear color 0xff081018 versus DX11 geometry color 0xff232f18. Remaining
differences were single-step rounding. Replaying eight separate resident draws
instead of one instanced draw reproduced it. Rendering each instance separately
isolated the failure to translated instance 7. Owned and immediate DX11 output
were identical, including the wrong edge pixel.

The previous port added `(1/width,-1/height)*clip.w` to the vertex clip-space
position to reproduce D3D9 pixel centers. Although mathematically equivalent,
this extra float operation can change fixed-point edge coverage at 4K.

NativeStoneRenderer now leaves the original clip position unchanged (the extra
raster-offset constants are zero) and sets DX11 viewport TopLeftX/TopLeftY to
0.5 pixels. The native view, projection, world transforms and target dimensions
are unchanged. This also follows the actual local DXVK D3D9 viewport path:
src/d3d9/d3d9_device.cpp BindViewportAndScissor uses x+0.5, y+height+0.5 with
negative viewport height. It does not perturb every vertex's clip coordinates.

## Validation

9/9 relevant CTests passed, including a permanent explicit 3840x2160 batch test.
The regression retains the eight-translated-instance lifetime case which failed
before the correction. No tolerance was increased, pixel masked or test skipped.

At 4K, all 36 ordinary full-image comparisons (both eyes, 1/8/32 repeated
instances, old single-object bridge, batch bridge, resident separate, resident
instanced, uncached owned and cached owned routes) have ZERO changed components.
Alpha also matches exactly. Owned/cached lifetimes, rotated/scaled worlds,
revision guards, varied-atlas fallback, immutable GPU snapshots, hybrid depth/
occlusion and GPU ownership stress continue to pass. The old 720p test passes.

The offline probe writes sparse pixel diagnostics only when parity fails. None
of these diagnostics run in the game. The normal clean plugin is built only for
link compatibility; it is not installed by this step.

## Scope

This resolves the specific 4K parity blocker documented in the cached-atlas step.
It does not claim to resolve the historic live trial which lost complete geometry
in one eye, nor prove a complete DX11 frame or Ardea FPS improvement. The tested
material/geometry remains one captured stone/stairs profile, repeated synthetically.

The cached route still avoids repeated atlas copies and matches native microbench
cost while including four same-device input-image snapshots. Native content-version
producers, generic durable geometry, broad material coverage, paired-eye commit
and downstream effects/depth/transparency remain prerequisites for real integration.
Normal runtime stays 0.23.58, DefaultTrialMode 0, draw suppression disabled. No new
headset trial or game launch is required for this correction.
