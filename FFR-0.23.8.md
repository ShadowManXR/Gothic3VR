# Hardware fixed foveated rendering — 0.23.8

## Feathered pixel blur — local 0.23.102

The tile-interleaving experiment from 0.23.101 was rejected in the headset and
has been removed. The hardware rate map is restored to its original contiguous
oval zones.

Launcher → Foveated rendering → **Transition width** now controls actual
per-pixel filtering after scene AA and upscaling. Range: 0–30%, default 10%.
Zero restores the original image path with no additional filtering pass. Width
uses normalized oval-diameter units and retains the landscape 3:2 shape. A
smoothstep blend starts one quarter of the band inside the rate boundary and
reaches full filtering three quarters outside. The inner center stays unchanged;
the outer edge of the full-detail oval participates in the visual blend.

A linear-color nine-tap blur softens the periphery (1.5 output pixels in
Quality/Balanced; 2.5 in Performance). It has no tile checkerboard, frame history
or temporal randomization. Each eye uses its own projection center. Only eyes
actually drawn with FFR are filtered; UI, HUD and cinematic-only contexts bypass
it. The hardware DXVK backend and palette are unchanged.

One reusable full-size BGRA scene target is shared sequentially by both eyes.
The existing AA/upscaler writes there; the new pass filters directly into the XR
target. At 4K this target occupies about 32 MiB of GPU memory. There are no CPU
pixel readbacks in the runtime. Resources are created lazily when filtering is
first used and retained until session shutdown. Any filter initialization error
logs a one-time fallback to the original image path for that session.

The GPU fixture passed Off/NIS/CAS with every spatial AA profile, inner-center
color/alpha preservation, repeatable stereo results, changing frames and zero
width restoration. A separate 4K fixture measured approximately 0.074 ms extra
GPU time per eye on this machine; this is not an in-game FPS guarantee. The real
DXVK runtime fixture also passed stereo/UI/cinema/cancellation/reinitialization
checks. Bogdan subsequently confirmed the visual result in the headset.

## Original backend notes

Launcher → Foveated rendering. Quality starts with a 75% full-detail diameter,
Balanced with 60%, Performance with 45%. Diameter can be adjusted from40 to95%.
Quality/Balanced shade2×2 pixels at a time outside the protected ellipse.
Performance additionally permits4×4 shading at the extreme corners. Tiles
touching the protected ellipse always remain1×1. There is no guessed lens mask
or visibility culling. The ellipse follows each eye's projection center.

Choosing a preset turns the spatial upscaler Off to retain native clarity.
Save settings applies FFR parameters live; installing the backend requires a
restart. The output resolution stays selected in the top list.
Manual NIS/CAS combinations remain possible, but initial testing should use
native resolution. UI/HUD/cinematics bypass FFR. This is fixed foveation, not eye
tracking. Live headset image quality and game FPS have not been measured.

This requires the included Gothic3VR DXVK fork and NVIDIA hardware support for
VK_NV_shading_rate_image plus the corresponding EXT_extended_dynamic_state3
enable state. RTX4080 hardware was verified. Unsupported backends gracefully
keep native rendering; they do not emulate FFR by blurring a completed image.

The fork is based on exact upstream v3.1.1 commit
b1a1c99ab52b687cf950d62c88bc2fa316b41663. Source patch, ABI header, submodule
versions and MIT license are in third_party/dxvk-ffr/ and third_party/dxvk/.
The modified Win32 d3d9.dll is supplied in third_party/dxvk-ffr/x32/.
For a fresh installation use tools/Install-Diagnostics.ps1 -FfrBackend, or
tools/Install-FfrBackend.ps1 after installing the plugin.
No local dxgi.dll/d3d11.dll or OpenXR runtime replacement is installed.

Two small R8_UINT rate images are uploaded on the existing DXVK Vulkan device
and reused for live parameter changes. The private versioned COM interface
enqueues state changes in D3D9 command order.
The DXVK context applies the mask only to single-sample, full-scene-size color
and depth targets with depth testing enabled during an explicit stereo world
scope. Shadow maps at other sizes and depth-disabled postprocessing remain1×1.
Both graphics pipeline library and optimized monolithic pipeline paths declare
the NV state dynamic. Internal DXVK blit/clear pipelines retain static1×1.
Dynamic state is cached per command buffer, including secondary command buffers.
Off transitions, cancellation and teardown drain native GPU work before freeing
rate images. A timeout quarantines live GPU resources instead of freeing them.

GPU tests render a pixel-frequency shader. The tested center remains bit-exact
while the periphery shows actual coarse shader evaluation. Both eyes, UI bypass,
Off restoration, repeated teardown and stock-DXVK unsupported fallback are tested.
Actual Runtime tests also cover the Vulkan→nativeD3D11→XR copy path,4K lifecycle,
UI alpha/cinema, partial cancellation and release retry. Native FMOD checks are
required before installation. Shader-work savings do not measure game FPS or
solve CPU scene-streaming stalls.

Use tools/Disable-DXVK.ps1 to revert to native Direct3D9 while retaining VR.
It recognizes only the exact tested stock/FFR binary hashes and keeps the DLL
as a recoverable renamed file. To disable only foveation, select Off in launcher.

Rebuild: clone the exact upstream commit recursively, apply
third_party/dxvk-ffr/g3vr-ffr-v1.patch (includes the ABI header), install local
Meson/Ninja and glslang, then build enable_d3d9=true with other APIs disabled.
The verified build used MSVC19.44, x86 target, static Release CRT, WindowsSDK
10.0.26100.0. work/build-dxvk-ffr.cmd in this task records exact local commands.
