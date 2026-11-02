# 0.23.67 — optional SMAA 1x Medium

Graphics now offers one anti-aliasing selector: Off, FXAA — Low cost, or SMAA
1x — Medium. Existing FXAA settings migrate unchanged; SMAA is disabled until
selected. Save settings and restart the game to change the scene filter.
Conflicting manually edited flags resolve to SMAA alone, never both filters.

The upstream iryoku/smaa algorithm is pinned to
71c806a838bdd7d517df19192a20f0c61b3ca29d. Medium uses luma edge detection,
threshold0.1, eight search steps, no corner or diagonal detection. Three
offline-compiled GPU passes filter each completed world-eye image before its
OpenXR submission. HUD, menu and radial layers bypass the filter. No temporal
history, jitter, depth estimation, CPU image copies or new game hooks are used.
VDXR, SteamVR and Meta share this filtering point; the Meta64 host is unchanged.

Two RGBA8 intermediates are cleared before each eye, with no cross-eye history.
A typeless GPU color copy exposes encoded data to edge detection and linear
data to neighborhood blending; final output is encoded once through the sRGB
OpenXR target. Alpha is preserved per pixel. NIS/CAS can feed SMAA at output
resolution. Off retains the exact-copy path and allocates no SMAA resources.
The only upstream edits explicitly bind the two samplers; provenance records
both original and locally patched hashes. License is appended to the existing
UPSCALER-LICENSES file, which the supported installer already deploys.

Validation: real hardware tests cover Off/FXAA/SMAA with native/NIS/CAS,100/75/50%
scale, odd dimensions, typed/typeless output, SRV/render-target-only input,
two alternating input textures, changing patterns including edge-to-flat,
gamma/channel order, diagonal smoothing, gradients and non-opaque scene alpha.
D3D11 debug validation reports no warnings/errors. Seven relevant CTests passed.
All18 launcher pages pass interaction and layout checks at normal, compact and
150% DPI; idle slider paint count remains0. Actual headset AA quality and cost
still require a game test. Offline synthetic GPU timings are not headset FPS.

Locomotion stays at the user's accepted0.23.66 behavior. No new movement trace
request is armed. Hand calibration, mods, progression, render distances and the
working Meta frame-wait scheduling are preserved.

## Subsequent DLSS5 investigation

The user's requested candidate route is Advanced/Custom, ReShade DXGI x86,
explicit OpenXR integration, dgVoodoo disabled. The local manager1.3.8 payload
does include an x86 feeder and a separate x64 host. Existing read-only analysis
is in work/dlss5-manager-analysis-20261025/FINDINGS.md (outside this project).
This is not yet a working OpenXR neural-rendering integration. A desktop
Present hook alone will not process our headset eye textures; per-eye callback
routing and the Meta64 process split must be validated before enabling it.
No ReShade/DLSS5 payload or dgVoodoo files are deployed in this SMAA build.
