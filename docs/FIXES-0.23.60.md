# Navigation scheduling candidate 0.23.60

The completed EntityCost capture contains 265 detailed world frames: EntityAdmin median 5.0301 ms; navigation median 0.0922 ms, maximum 17.173 ms. The native round-robin script routine was only 0.0079 ms during that largest navigation frame. These inclusive timings do not identify every cause of navigation spikes.

For the exact CP 1.75.14 Game.dll, adapt the existing **simple / non-priority navigation loop** soft deadline from unsigned elapsed >15 ms to >3 ms in active VR. Keep complete work units, the original cursor and accumulated movement time. The priority ROI loop, RunDynamicNavigation, scripts, collision and player inputs are untouched. If a cursor cycle remains outstanding for 250 ms, use the native deadline until its next wrap. Disable the reduced deadline outside ready VR and during teardown. No draw distance, ROI, vegetation, cache-size or user configuration edits.

This distributes existing work; it does not eliminate its total CPU cost. Distant simple actors can update less frequently. A single expensive operation and native-budget catch-up can still exceed 4 ms. Headset improvement is **unmeasured** until a normal play test.

Installation is guarded by the whole Game.dll hash, function prefix, complete 27-byte local control-flow window and its relocated timer/cursor operands. Only the original CMP/JBE pair is redirected. The generated unsigned compare preserves registers, stack, flag-write destinations and cursor store. Unsupported profiles retain native code. The gate lives until process exit; the Engine/Game files on disk are unchanged. Separate diagnostic builds omit the optimization.

Verification: owned x86 fixture executes the exact original native branch, checks original and reduced deadlines, timer wraparound, native cursor write, original restoration and duplicate-hook refusal. A scheduler fixture checks deferred progress and accumulated travel. 109/109 applicable native/control regression tests pass. GPU code is unchanged; the .59 full GPU suite already passed and was not repeated.

## Streaming follow-up

Reviewed 710 unique existing JSONL logs, 57 with scoped timing evidence. Historical world PVS stalls include ~26–27 ms with ~4.73 MB of process reads, but these are older .23.12/.23.15 sessions. Much larger old peaks include startup/forced work and cannot be treated as current traversal disk stalls. Process read bytes are not physical disk latency; inclusive timer scopes overlap. The newest standing EntityCost capture contains no sampled geometry reads / queue waits and does not test zone traversal.

Native code already has PVS-cell prefetch jobs and per-type resource caches. eCBaseCache::CacheInResource schedules worker items for lazy behavior, but immediate loading waits with WaitUntilNonProcessing and calls the loader synchronously. The latter spins with Sleep(0). Removing that wait or globally forcing lazy behavior would allow callers to consume incomplete resources and is not a valid optimization.

Keep the existing PUP stereo-threading / shared optional-work budget fix. A useful next preloader would queue upcoming cell resources through the native asynchronous path, using travel direction, cancellation and process-memory headroom. It must preserve the original visible set and never force a second camera render, global cache increase, unrestricted GPU loading on workers, or whole-world asset retention. This version does **not** claim or install such a predictive preloader: traversal-specific resource misses still need attribution.
