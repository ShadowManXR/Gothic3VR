# 0.23.18 — bounded GPU skin-motion diagnostic

The actual0.23.17 capture validates20native entities and265same-eye preceding
draw palettes across four stereo frames. All265 matched draws change bones
and projection;213also change root transforms. No supported draw was rejected.
This establishes palette correspondence, not complete scene motion coverage.

A new SM3 companion pass rasterizes supported native indexed geometry against
the existing opaque depth using equality testing and no depth writes. It
uses four native weights/indices and current bone constants; previous bones
come from a bounded GPU texture atlas. RG stores previous-current pixel motion,
B supported-geometry occupancy and A valid previous history. Each eye has a
private RGBA16F target. Background/unsupported motion is not declared complete.

Real DXVK tests use the original Gothic vertex shader for reference depth,
synthetic triangles, and eight actual captured current/previous game palettes.
They cover bone/camera motion, perspective, jitter removal, occlusion, invalid
history, two atlas rows, stereo ownership and native state/resource restoration.
The observed state-block omission of vertex sampler state is handled explicitly.
The observer/history/replay/readback path is also tested inside BeginScene.

Only an armed one-shot request runs this pass. The second metadata frame is
used for both motion and matching color/early/late depth, after previous skin
history exists. Geometry COM references are bounded and released; companion
draws bypass native UI/draw observation to avoid recursive capture. Files use
RGBA16F with masks and explicit incomplete-coverage metadata. No shader,
lighting, gameplay, controls or animation of the original scene is replaced.

DLAA remains disabled. Complete scene vectors, GPU depth/worker transport,
masking and real NGX evaluation are separate remaining integration work.
