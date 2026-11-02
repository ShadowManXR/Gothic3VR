# 0.23.12 — bounded temporal-input discovery for DLAA

The 2026-10-17 19:33 session (PID 9544, 0.23.11) provided actual 3840 × 2160 eye
projections, IPD 0.06261 m and sparse camera/device metadata. In the selected slow
render windows, the two eye renders cost about 22 ms CPU wall time together;
input averaged below 1 ms. The separate 90-second process sample showed about
22% average whole-GPU utilization and about 1.2 GiB process private memory. These
are overlapping/uncontrolled session observations, not a measured FPS gain.
They point to native scene CPU work as a priority, not memory capacity. The
snapshot/evidence is in the workspace ardea-live-20261017-1933 folder.

Sparse diagnostic matrices cannot stand in for a temporal renderer. In
particular the sampled eye projection used a near plane of about 25 game units,
whereas the restored native camera reported 13. DLAA must use the exact matching
eye projection and depth surface, not the restored desktop camera.

This build adds an opt-in discovery capture, not in-game DLAA:

- Place dlaa-capture.request in scripts/Gothic3VRDiagnostics before launch.
  It captures four successful world stereo frames, skips menu-only frames and
  removes that request after completion. With no request, capture is disabled.
- The existing draw hooks observe the first and every 32nd native draw, capped
  at 128 observations per eye. No new draw/device detours are installed.
- Records include matching per-eye SDK view/projection matrices, sampled render
  target/depth surface descriptors and texture-container identities, viewport,
  depth-write/test states and bound shader identities. The first full-resolution
  depth-writing geometry VS/PS bytecode and VS constants c0–c255 are saved.
- Device getters retain no COM resources after observation. No GPU readback,
  rendering-state setters, projection jitter, substitute shaders or waits are
  added. Formatting and shader-file output happen after the XR submission, only
  for those four frames. Their timing is diagnostic and must be excluded from
  a clean benchmark. Cancellation does not publish a stale stereo pair.
- The real GPU UI/hook regression test verifies unchanged native pixels/state,
  actual draw-hook observation, hard query budget, matching stereo serials,
  exact bound-shader bytes/constants, no accidental UI loss of world metadata,
  cancellation and resource release.

Depth pixels and object/animation motion vectors are still not exported. Candidate
surface formats and shader constants require validation before they can feed the
verified standalone NGX worker. No fake zero-motion DLAA is enabled. All existing
0.23.11 movement, combat, native/PUP, visual quality and launcher settings remain.
