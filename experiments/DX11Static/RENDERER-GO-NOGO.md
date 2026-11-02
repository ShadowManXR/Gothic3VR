# Custom renderer: bounded feasibility decision

Status: protocol prepared, no new runtime probe or replacement deployed.
The whole-API dgVoodoo/DX11 trial lost approximately 20 FPS according to the
user. Normal DXVK backend and accepted VR DLL have been restored and their
hashes verified. Do not use the wrapper as the new renderer's foundation.

## Decision to make

Does replacing a substantial, identifiable render workload improve total VR
frame time enough to justify expanding a custom renderer? API identity and
isolated mesh throughput are not success criteria. Existing synthetic resident
DX11 instancing reached roughly native draw cost; it did not establish a gain
in Ardea or savings in native scene preparation.

## Gate 1: recoverable work, before a broad port

Use the current clean, accepted runtime as baseline. Existing historical scene
profiles can locate candidate functions, but pre-0.23.57 first-person samples
must not serve as the current performance baseline: the signpost/name hot-path
bug was subsequently fixed.

Build a separate, opt-in, bounded diagnostic for one known world render scope.
Identify and measure separately: scene/visibility preparation, native render
submission, GPU execution, final VR transfer/synchronization, simulation.
Nested inclusive CPU counters must not be summed; CPU and GPU durations can
overlap and must not be added as total frame time.

A suppression-only control can omit the candidate render work while simulation
continues. This intentionally incomplete image is diagnostic only. Report
exactly which work was bypassed and what remains. Skipping late Draw calls does
not remove earlier visibility, sorting, material preparation or animation.
Removing depth/occluders can change later GPU work, so this control is not a
strict numerical upper bound unless downstream workloads are kept equivalent.
If equivalent downstream work cannot be maintained, use the result only as
screening evidence, not proof of a renderer gain or a categorical negative.

Pick the workload from actual current cost. Architecture is a feasible initial
candidate (12 imported resources, 28 sections), not automatically the dominant
cost. If that workload has little recoverable cost, stop the architectural port
for performance purposes and reassess the expensive scope before expanding it.
Do not grow material coverage merely to chase a positive result.

## Gate 2: minimal genuine replacement

Replace the selected substantial scope with resident DX11 inputs and a sealed,
versioned scene/material packet set. Omit its equivalent native render work.
Count the actual live instances/sections replaced; the offline asset inventory
is not live coverage. Preserve geometry, distance/LOD, textures, lighting,
shadows, alpha, depth, subsequent native composition and both-eye correctness.
Any unavoidable visual difference disqualifies a quality-matched gain claim.

Use a whole-pass transfer strategy; include input updates, both-eye drawing,
resource ownership and final VR composition in the measured total. No timed
CPU image readback, per-object bridge/fence loops, dual-rendered candidate scope
or hidden reduction in scene quality. Unsupported paths stay native and are
disclosed. Correctness captures run separately from timed windows.

## Comparison and stopping rule

One prepared headset session can run warmed A/B/A windows, where A is the
accepted renderer and B is the genuine replacement. Match settings, camera
route/save and simulation conditions as closely as possible. Combat changes
over time: compare baseline drift and repeatability instead of treating two
different battle moments as identical. Exclude loading/warmup and retain raw
frame-time distributions and visible correctness evidence.

Proposed continuation threshold, not a measured result: at least 20% lower
median total frame time, reproducible beyond baseline drift, with p95 no worse
by more than 5%, no new stereo/streaming faults and equal image quality. Report
headset FPS as secondary context. A smaller gain is not grounds for expanding
this project by days without reconsidering its scope and cost.

If the upper screening budget is unattractive, or the genuine replacement is
slower/within noise, stop and retain the normal runtime. Full renderer work is
not authorized by a synthetic speedup, a blank-scene FPS jump or an API swap.

No gain established yet. No new headset capture requested until the relevant
bounded diagnostic or actual replacement mode is concrete and verified.
