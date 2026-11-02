# Play mode 0.23.27

Detailed CPU scene/crowd profiling and per-draw D3D9 timing now require explicit
process-local opt-in: G3VR_SCENE_PROFILE=1 and G3VR_DRAW_PROFILE=1 respectively.
Normal play installs none of the pure native scene/crowd/session timing detours.
Unlike the old SCENE_PROFILE=0 mode, it also omits legacy renderer timing hooks.
Per-draw UI hooks remain for UI capture, but no ordinary world timing scope is
opened, so normal draws forward without two QPC calls and timing collection.
Basic frame/update metrics and sparse asynchronous GPU measurements remain.

PUP compatibility and its shared stereo work budget still run independently,
including required budget clocks, original forced/loading work, threading and
queue locking. Camera/arms/UI/input/ammunition logic is unchanged. A separately
armed temporal request still opens world observation even if draw profiling is
off. No temporal request is armed by this release. Game DLAA remains off.

Offline evidence excludes a blind low-poly optimization: session 17340 sampled
304 root calls at 5.811707 ms/call versus 304 nested low-poly calls at 0.109428
ms/call. They overlap; do not add them. Engine UpdateRenderMatrices already
caches by frame (Engine RVA 3aba0, comparison at 3abc7), and DXVK already drops
unchanged shader constants. No duplicate cache or cross-eye visibility reuse
is introduced on these assumptions.

This release removes accumulated diagnostic overhead. It is NOT evidence of
an FPS improvement in Ardea and does not claim the remaining engine bottleneck
is solved. Existing captures are retained for offline analysis. No additional
diagnostic-only trial is requested.
