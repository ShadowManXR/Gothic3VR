# VR frame pipeline and measurement — 0.22.13

The first performance pass changes the shared-texture transfer schedule, not
game quality, render resolution, locomotion, arm IK or combat behavior.

Each eye still renders its complete geometric view and rebuilds its own frustum.
The left eye, right eye and UI each retain their separate D3D9Ex shared source.
Their D3D11 copies are submitted together after ONE D3D9 completion fence, then
ONE D3D11 completion fence completes all reads before releasing XR images.
An updated radial/status image joins the same D3D11 completion fence.
No texture is reused while a read is outstanding. No work or acquired image is
intentionally carried into another XR frame. Cancellation uses the same drain.

D3D9 event polling submits once and only requests another flush after 2 ms if
needed. D3D11 explicitly flushes its event once and polls with DONOTFLUSH. The
existing two-second failure bounds remain. These fences must not be removed:
D3D9Ex sharing alone provides no cross-device synchronization.

## Timings

`stereo-game-*.jsonl` now contains `vr_frame_performance` every two seconds.
Collection uses fixed storage and QPC; there are no extra GPU timestamp queries,
per-frame formatting, per-frame diagnostic allocations, or performance overlay.

Reported CPU wall-time means and maxima cover:

- `xr_wait`: intentional runtime frame pacing as well as any runtime stall.
- `tracking`: begin-frame, pose location and controller action synchronization.
- `arms_prepare`: preparation before the eyes, plus per-eye preparation.
- `left_render` / `right_render`: native render calls, including hooked palette work.
- `ui_draw`: native UI phase including its GPU-command replay on the CPU.
- `d3d9_wait`: waiting for completion of shared-source writes.
- `swapchain_wait`: waiting for acquired XR destinations to become writable.
- `d3d11_wait`: waiting for all shared-source reads to complete.
- `xr_end`: xrEndFrame wall time.
- `frame_work`: elapsed time from located/synchronized poses through xrEndFrame.

These are NOT independent GPU durations. `frame_work` includes several other
fields and must not be added to them. Frame intervals include work outside these
callbacks and pacing. Interval p95 uses at most the most recent 256 intervals in
the reporting window; mean/max cover the whole window. Standby and cancelled
frames reset the reporting window and interval baseline.

Counters report copy count, UI draw count, polling calls and actual flush
requests. A normal three-layer gameplay frame uses one D3D9 and one D3D11
completion fence instead of three of each. UI-only frames use one of each.

`tools/Analyze-Performance.ps1` reads the most recent session by default and
summarizes the last 60 reporting windows. It keeps immediate/batched modes
separate, weights means by sample/frame count, and reports the worst per-window
p95 without misrepresenting it as an exact combined percentile.

For a controlled comparison only, set `G3VR_COPY_BATCH=0` in the launching
process environment to complete each transfer immediately. This retains the
same timing collection and lightweight polling and does not edit user settings.
Unset the variable to restore the default same-frame batch schedule.

## Verification and interpretation

`DiagnosticFramePipelineTests` covers bounded ownership, duplicate rejection,
partial frames, fence failures, release retry and submission order.
`DiagnosticFramePerformanceTests` covers timing aggregates, p95, reporting bounds
and pause reset. `DiagnosticBridgeGpuProbe` is an explicit hidden-window native
D3D9Ex/D3D11 test: changing 4K eye/UI patterns are copied and read back solely in
the test process to check pixels and compare immediate/batched fence counts.
Its transfer timings are NOT Gothic 3 FPS or VDXR headset acceptance.

On the development RTX 4080, the standalone 4K test completed 1080 pixel checks.
Immediate mode used 108 D3D9 and 108 D3D11 fences for 36 frames; batching used
36 of each. Transfer medians were 1.055 ms and 0.635 ms respectively in that
single run, including the new polling policy in both modes. This isolated
transfer test does not measure world rendering or headset FPS. All 80 CTest
checks also passed. The CPU collector benchmark took 43.916 ms for 100000
collections; it does not include game work or every external timing call.

Compare the same save, location, resolution and headset refresh rate. Use the
timings to decide whether the next change should target native scene submission,
GPU rendering, UI replay, tracking or resource streaming. No game FPS gain is
claimed without that controlled live comparison.
