# 0.23.9 — native scene and crowded-combat evidence

The 09:53 capture on October 17 contains a 896.8077 ms native PVS Process
inside a 919.0972 ms left-eye render (frame6092). PUP's optional-work maximum
was only22.0663 ms in that session. Another frame5973 spent181.4973 ms in
ExtractFromFrustumOC. The 09:55 capture has no equivalent large gameplay spike.
These sessions are not a controlled performance comparison. The user's main
reproduction is New Game's crowded Ardea battle, where sustained simulation
cost must be distinguished from isolated loading/visibility hitches.

This diagnostic build adds native scopes for grid rebuild, valid-cell lookup,
static cell hierarchy construction, geometry cache-in/cache-out/node reading,
and pool queue/idle waits (also outside eye rendering). It measures native module and entity updates,
character animation, physics simulation, animation physics, AI script routines
and navigation on the application thread, including outside eye rendering.
The outer module/entity timings overlap their children. Per-call animation and
AI means are not whole-frame costs: the analyzer also divides totals by the
observed native entity-admin update count. Worker-thread time is not measured
by these collectors. Update context is cached from the application callback;
a transition can label the first preceding native update with the prior context.

For long gameplay PVS/OC scopes, vr_scene_stall records same-scope wall time,
thread CPU time and process read-counter deltas, along with native threading,
queued-job and active-worker counts before/after. Thread CPU accounting has
OS granularity; process counters include asynchronous/buffered reads by other
threads and do not capture all mapped-file paging. Low CPU time plus long wall
time suggests waiting/scheduling, not automatically disk I/O. These records
are rate-limited per scope and share exact eye/frame IDs with existing spikes.

Engine and Game file hashes, exports, RVAs and complete relocatable prefixes
are checked before any hook. An existing PUP/mod detour with a different entry
is retained and its mask bit is absent. No unknown jump chain is overwritten.
The eight streaming bits are9..16 and five Engine simulation bits17..21;
Game's separate crowd mask has AI-routine bit0/navigation bit1. The original
ABI, result values and call counts are retained. Native simulation, AI,
navigation, physics, loading lists, graphics fences and quality are unchanged.

G3VR_SCENE_PROFILE=0 in the launch environment disables the added hooks and
CPU/I/O snapshots for an overhead comparison; the old profiler remains.
Fixed deferred storage is now16x8192 bytes, so the expanded native JSON report
cannot be truncated by its previous4096-byte queue. No allocations or file
flushes are added inside native eye scopes. Analyze-NativePerformance.ps1
accepts both older sessions and new records.

No FPS gain or engine hitch fix is claimed by instrumentation. With the user
away from the headset, do not launch a new game or overwrite saves. Use the
next Ardea session to select a specific native optimization, and retain the
installed plugin backup. The old FFR/DXVK configuration and hand/game settings
are not changed by this release.
