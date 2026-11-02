# Sparse native scene execution profiling

The 0.23.22 live capture (PID4212) records sustained expensive native rendering.
After the diagnostic capture, 39 selected slow windows contain1905 frames with
41.43ms mean interval and29.01ms combined eye render wall time. These are selected
windows, not a controlled before/after benchmark. The1.14second spike in frame912
belongs to diagnostic GPU readback and must not be treated as an engine hitch.

0.23.23 times nine deeper native operations on one of32 world stereo frames:
render operations, instance batches, material begin/end, effect execution,
texture execution, visual submission, dynamic occludees and decals. This is
instrumentation, not an FPS optimization. No native operation is skipped or
cached; both eye frusta, materials, shadows and gameplay remain native.

The exact Engine.dll hash, exported symbol, RVA and full instruction prefix are
checked before each detour. Unsupported or already modified entries are left
alone. Original thiscall/stdcall, parameters, return values and exception flow
are preserved. TLS disables timing on other threads and unsampled frames.
Temporal diagnostic frames are excluded. G3VR_SCENE_PROFILE=0 disables these
extra hooks, as it does the existing scene profiler.

Startup native_scene_execution_profiler_mask reports nine supported entries.
Existing phase IDs0..29 are unchanged. Nine appended sampled_* metrics report
inclusive means per measured call; their counts are sparse. Never divide their
totals by all entity updates to infer full-frame costs, add parent/child costs,
or compare maxima from separate aggregation windows as one frame.

The scene analyzer marks these phases as sparse and estimates no total savings.
Actual next-session measurements are needed to choose a safe engine change.
