# NIS / CAS — 0.23.7

Launcher → Graphics selects Off, NVIDIA Image Scaling or AMD CAS. Start with
75% internal render scale and 25% sharpness. Select an explicit output resolution
in the top list, save and restart the game. At 3840×2160 output, 75% renders each
eye at 2880×1620 (56.25% as many scene pixels). 100% applies sharpening only.
Off renders at the full selected output size. No headset streaming resolution,
native world distance, texture, animation or progression setting is changed.

The native resolution change happens before engine initialization, through the
existing atomic UserOptions writer and updated early context patch. The native
D3D9/DXVK renderer, depth, postprocessing, GUI and mouse all use the same internal
resolution. The selected resolution becomes the OpenXR projection output size.
UI/HUD/cinematics keep the separate native-resolution quad, bypassing sharpening;
this version does not independently render the GUI at the larger output size.
"Use game settings" leaves UserOptions intact and applies sharpening only,
preventing repeated scale reductions across launches. Direct game launches with
unmatched dimensions fall back to same-size sharpening and log restart_required.

Full-image SM5 compute shaders are compiled offline from the MIT NIS 1.0.3 and
CAS headers in the user's VRPERFKIT source checkout. There is no VRPERFKIT DLL
injection. The postprocess runs on the existing native D3D11/OpenXR device, after
the D3D9 shared-image / DXVK Vulkan import fence and before the final D3D11 fence
and XR release. A reusable RGBA scratch UAV feeds an sRGB blit into the BGRA XR
image. Typeless VDXR images use explicit sRGB RTVs. Input SRV fallback performs a
GPU copy only. Off keeps direct CopyResource and allocates no shader/scratch.
No extra CPU readback, cross-frame queue or XR session is introduced.

107/107 automated tests pass, including real GPU NIS/CAS at 50/75/100%, odd image
edges, exact Off copies, color/gamma, input-SRV fallback, independent eyes, 5K
DXVK projection, native UI alpha/cinema, cancellation, release retry and shutdown.
Launcher all-tab layout and FMOD native compatibility checks pass. Live headset
image quality and game FPS comparison remain unmeasured. Reduced pixels help a
GPU bottleneck; CPU scene updates / streaming stalls need separate work.

DLSS feasibility: the current official Windows SDK libraries target x86_64 (and
ARM64), whereas the game process is x86. A separate 64-bit rendering host would
not alone supply DLSS's required depth, motion vectors, camera jitter and temporal
history per eye. Our current bridge transfers final color only. That would be a
larger engine-rendering integration, rather than replacing this spatial filter.
Official references:
- https://github.com/NVIDIA/DLSS/tree/main/lib
- https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS.md

Third-party license notices accompany the installed plugin in
scripts/Gothic3VRDiagnostics/UPSCALER-LICENSES.txt.
