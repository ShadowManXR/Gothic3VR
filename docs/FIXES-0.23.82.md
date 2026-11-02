# 0.23.82 — VR fire and magic particle rendering budgets

Graphics now offers Fire & magic effects: Original, Balanced VR (default) and
Performance VR. Balanced renders 65% of a dense emitter, up to 768 particles;
Performance renders 40%, up to 384. Small effects retain at least 12 particles.
Both optimized profiles suppress only explicitly named heat-haze particle layers,
including their native refracted classification. Original bypasses this new policy.
Save settings applies the profile live, on the next native particle buffer build.

The retained native analysis confirms simulation and vertex builds already have
application-frame stamps. No redundant simulation/cache scheme is added. Native
UpdateVertices sorts complete 180-byte particle records and builds CPU quad
vertices before drawing transparent geometry. A fixed 135 KiB application-thread
scratch buffer supplies an evenly sampled array only inside that native builder.
The original array pointer and count are restored even on failure/SEH; capacity,
particles, lifetimes, velocities, collision, spells, mana and damage stay native.
The native once-per-frame buffer cache remains shared across both eyes/instances.
There is no per-frame heap allocation or world scan and no new runtime profiler.

Targeting uses bounded native VFX_Magic / fire / torch / smoke names and texture
prefixes. Weather, blood and unrelated particles keep their existing behavior.
Heat-haze suppression leaves native simulation/auto-kill running, avoiding orphaned
magic effects or broken expiration. Flames, glow, lights, sound and existing VR
hand/torch/flame-sword attachments are retained. Distances and renderer are unchanged.

The source audit found 330 relevant particle templates in the installed native
Templates.pak (not a reconstruction of mod/patch mount precedence):
328 explicitly use 25 Hz, so there is no speculative tick-rate reduction. Examples
include campfire emission 500/s and separate heat-haze emitters at 150–300/s. This
is asset evidence, not a measurement of current live FPS or proof of the dominant
bottleneck. The optimization reduces render preparation and transparent overlap;
native particle simulation and dynamic-light cost are not claimed eliminated.

The exact Engine.dll hash, exported entry RVAs and complete prologue instructions
gate installation. Tests cover scoped array restoration, untouched simulation,
capacity, sample coverage, original bypass, native frame-cache layout, profiles,
settings and launcher layout. Headset visual/FPS comparison remains pending.
