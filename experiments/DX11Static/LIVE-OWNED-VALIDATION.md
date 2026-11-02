# Mode5: actual native inputs into an owned DX11 pass

This is a bounded read-only validation, not a replacement renderer or FPS trial.
It captures the known Ardea stairs material at its real native draw, freezes its
noise/lightmap/shadow images while interop ownership is held, and consumes the
sealed packet at the matching solid boundary AFTER the SDK material scope ended.
Native world draws are never skipped and private output is never composited.

NativeStonePacketProducer reads the actual atlas bytes for each observed draw.
Exact byte equality permits immutable content-version reuse across eyes. Changed
bytes at the same source address publish a new version; unavailable data and old
epochs refuse. This intentionally still pays the live SDK read/comparison cost.
Native pooled base/minimum/index offsets stay on the original-DIP reference;
the privately imported geometry uses its own local indices. A regression repacks
all three native streams and the index buffer with nonzero offsets and validates
both eyes. Native buffer-content fingerprints are still not claimed.
It does not claim that native mutation hooks, generic geometry or complete texture
producers have been implemented. Native image contents are snapshotted per capture.

A same-frame two-eye pair must pass native color/coverage/depth checks. At most
four comparisons are attempted and validation stops after success or refusal.
Fixture tests cover deferred provider destruction, exact-content reuse, same-
address mutation, source failure, stale epochs, wrong projection, unchanged native
scene output, and a passing left eye followed by a failed right eye refusing the
entire pair. Original 4K and resource-lifetime tests remain enabled.

The first real trial was REFUSED: the captured right-eye geometry and depth were
exact, but anisotropic color sampling differed (up to 44 UNORM steps). An offline
control using the same shaders/geometry/textures passes with Windows D3D9Ex versus
DX11 (max1 step), whereas DXVK/Vulkan versus DX11 with anisotropy differs (max72
steps). This is a sampling-API discrepancy; it does not by itself establish the
cause of every live color difference. Thresholds have not been relaxed.

Mode5 now also records a separate sampler-control reference: only material slots
4/5 min/mag filtering become trilinear for the private native replay and the owned
control render. Native sampler state is restored before returning to the game;
the actual native world keeps its original anisotropy. The immutable packet is
not changed or consumed by the control. Both control and actual paths retain the
same strict RGB/coverage/depth thresholds. Discovery warms up for one frame so a
late first observation cannot start validation with only the right eye.

PASS still requires actual sampling parity in BOTH eyes. SAMPLER_CONTROL_ONLY
requires exact geometry/depth in the actual pair AND strict control material
parity in both eyes of one frame, and explicitly means actual color parity is
unproven. It never enables replacement or proves FPS improvement. The analyzer
keeps this qualified result separate and rejects missing/cross-frame/depth-failed
evidence. Tests cover both classifications, sampler restoration and a deliberately
corrupted shader that must refuse even the control.

The separate G3VR_DX11_VALIDATION build keeps G3VR_CLEAN_RUNTIME and disables the
general CPU/GPU/temporal/camera/arm probes. Only mode5 is accepted; native hooks
are restricted to the solid boundary, rigid mesh context and instance execution.
The ordinary build has this option OFF and retains its direct native draw path.

Start-OwnedDX11Validation.ps1 verifies the clean/validation binaries, engine,
backend and local asset hashes. It temporarily atomically exchanges only the
script DLL, stores the clean copy outside the game, launches with child-only
mode5 variables, waits for exit and restores the original exact DLL. No INI or
launcher setting is changed. A journal supports recovery after an interrupted
helper; external plugin changes are never overwritten. The game directory never
contains duplicate script resources. Normal launcher use remains unchanged.

For the live check: Quest active, start the separate desktop validation shortcut,
enter Ardea/New Game, close the Tip, look toward the central stairs/house for
20-30 seconds, then exit. Early diagnostic readbacks can pause frames. The result
and both isolated eye images are in results/owned-live-*/. This test verifies real
input ownership and deferred rendering, not improved headset framerate.
