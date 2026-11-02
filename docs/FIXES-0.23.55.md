# 0.23.55 — restore native visibility during VR gameplay

The Ardea screenshot session PID 19812 shows tooltip/menu rendering around
3–4 ms per eye and first-person gameplay around 14–16 ms per eye. Arm preparation
is roughly 1.2 ms; copies and transport do not account for the difference.

The plugin's pending-visible occlusion shortcut was active only during world
gameplay. It returned visibility immediately on pending queries and let native
Resolve recycle them before reading their final samples. The session reports
67,032 conservative results versus 181 ready results (99.73% pending). Thus
potentially hidden geometry was admitted without a completed visibility test.
The tooltip path retained native occlusion results. This is a strong, specific
regression candidate, not a measured post-fix FPS result.

Native query handling is now the default for both gameplay and menus: no
occlusion vtable hook is installed. The old behavior requires explicit
G3VR_ASYNC_OCCLUSION=1 for diagnostic comparisons. No cached visibility from the
opposite eye or old frame is substituted; each eye keeps native frustum work.

Distances, resolution, arms, gameplay, PUP scheduling and final eye transport
are unchanged. Current installed EDD values are high detail 20,000 units,
low-poly 200,000 and vegetation 10,000. Larger radii increase candidate geometry,
prefetch work and draws; the logs do not isolate distance arithmetic as the
dominant cost or quantify which distance radius costs most. Restore working
visibility before changing these radii or attempting a renderer replacement.

Validation: native Resolve/GetData fixture executes delayed zero-pixel results
and verifies culling/pool ownership under the default policy. Native D3D9,
installed DXVK, UI and PUP compatibility checks pass. Headset FPS acceptance
remains to be tested in the same Ardea tooltip-to-gameplay transition.
