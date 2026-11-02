# FXAA

Graphics > **FXAA anti-aliasing (low cost)** controls `[Stereo] FxaaEnabled`.
Save settings and restart Gothic 3. Existing installations default to Off;
the local 0.23.61 trial explicitly enables it while retaining other settings.

NVIDIA FXAA 3.11, quality preset 12, subpixel 0.25, edge threshold 0.166,
minimum edge threshold 0.0625. The green channel supplies luminance without
another prepass. This spatial filter smooths visible edges but cannot eliminate
temporal shimmer on moving foliage. It may soften fine scene detail.

The compiled D3D11 pixel shader filters each completed eye image before OpenXR
submission. Native HUD, menus, radial menu and status quads bypass it. No motion
vectors, history buffers, CPU readback, runtime shader compiler or external
injector are needed. Resolution, foveation and gameplay settings are independent.
With NIS/CAS, FXAA uses the existing final output pass. At native resolution it
adds one fullscreen draw; a GPU-only input copy is used if the shared scene image
cannot have an SRV. Encoded SDR input is filtered before decoding for the sRGB
output RTV, and source alpha is preserved.

`DiagnosticUpscaleGpuTests` checks real GPU output for native/NIS/CAS, 100/75/50%
scales, direct SRV and copy fallback, typeless XR output, colors/gamma, coverage,
changing frames and diagonal edge smoothing. Settings roundtrip, launcher
interaction/layout/idle painting, and actual world/UI XR submission also pass.

The optional test executable `--benchmark` mode is offline only, never included
in the plugin. Local RTX 4080 timestamps for two 3840x2160 eyes measured about
0.25 ms total FXAA GPU time on a dense checker image. This synthetic number is
not a headset FPS result or a performance guarantee; flat-image/copy timings
are particularly sensitive to caching and GPU clock state. Validate image
quality and frame time in the headset with the same scene and settings.

Source: <https://github.com/GameTechDev/CMAA2/blob/master/Projects/CMAA2/FXAA/Fxaa3_11.h>
NVIDIA license retained in `third_party/upscalers/fxaa/LICENSE.txt`; include it
as `scripts/Gothic3VRDiagnostics/FXAA-LICENSE.txt` with binary distributions.
