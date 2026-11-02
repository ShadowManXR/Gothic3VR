# 0.23.16 — correct color-pass eligibility for the DLAA animation probe

The actual 0.23.15 capture (PID17436, four stereo pairs473..476 in Ardea)
completed, but recorded no animation palettes: 374 supported-shader draws were
rejected and34 used other shaders. The native entity scope did run. The old
aggregate counter did not distinguish the rejecting condition, so this is not
proof that any single condition accounted for all374 draws.

The previous filter required depth writes for every visible color draw.
That excludes legitimate color passes which test a previously populated depth
buffer. Such draws are now eligible; depth writing remains part of pass
identity. Depth-only/color-disabled passes and mismatched surfaces are still
excluded. Render/depth surface identities and depth/cull/alpha-test/color state
are included in draw correspondence to prevent mixing different passes.

A single-instance INDEXEDDATA frequency is accepted as equivalent to ordinary
frequency1; multiple instances remain excluded. Shader identity caches reset
each eye/frame. Six explicit rejection counters include bounded first-state
examples, so any remaining unsupported native layout can be identified from
one capture instead of another undifferentiated rejection count.

The real DXVK getter fixture now uses the actual native single-stream68-byte
HardRenderActor declaration (verified offline in Engine.dll at430e0..431d1),
the captured Gothic shader, depth-writing and depth-readonly color passes,
equivalent single-instance state and missing-depth rejection. Pure history
tests verify that changing target or pass state cannot reuse a previous pose.

This remains opt-in diagnostic instrumentation. No shaders, native render
states, animations, gameplay settings or user controls are changed. It does
not enable DLAA or export rasterized motion vectors. Actual animation
correspondence still requires a new live capture after installation.
