# DXVK 3.1.1 Windows/x86 interop

The optional graphics backend detects `ID3D9VkInteropDevice`. Native D3D9Ex
continues using the existing shared-texture path. Only the supplied **x32
d3d9.dll** belongs in the game directory; native D3D11/DXGI and the established
D3D11 OpenXR/VDXR session remain in use.

DXVK's legacy D3D9 shared handle fails native D3D11 `OpenSharedResource` with
`0x80070057` on this machine. The new backend creates ordinary DXVK render
textures and native D3D11 BGRA8 NT textures. It imports the latter into DXVK's
existing Vulkan device, then copies both eyes and the native UI in one batch.
There is no CPU image readback/upload in the frame pipeline.

DXVK's explicit `TransitionTextureLayout` materializes deferred framebuffer
clears; manually applying a Vulkan barrier or waiting for a D3D9 event alone
left previous-frame pixels in short/immediate/partial frames. Every batch
prepares only its queued sources, preserves a bounded D3D9 completion event,
flushes DXVK, acquires/releases the imported keyed mutexes in Vulkan and
submits under DXVK's queue lock. No D3D methods execute under that lock.
Source layouts are restored. A Vulkan fence completes the copy before native
D3D11 acquires the images; D3D11 completes its reads and returns the mutexes
before sources can be reused or XR destinations released. Cancellation uses
the same drain path. GPU timeout resources are retained instead of freeing
potentially active command buffers; normal teardown releases all references.

Tests use the actual Runtime capture/drain methods with real DXVK/Vulkan and
native D3D11 GPU work, plus an OpenXR acquisition/submission fixture. They
check changing colors in both eyes, UI alpha/background, batched and immediate
copies, menus, opaque cinema, partial-frame cancellation, failed-release retry,
status upload, repeated initialization, hook restoration and native device
reference restoration at 64x64, 3840x2160 and 5120x2880. A headset test and
Gothic 3 frame-time comparison remain necessary; isolated transfer timings
are **not game FPS** or proof of a scene-streaming performance gain.

DXVK DLL SHA256: `265888C31CA78DFFA290C39CB7E50BFB02762590E41927906E46FB32F01497FA`.
The game-native binaries, PUP settings and user INIs are preserved.

Upstream references:

- [DXVK 3.1.1 interfaces](https://github.com/doitsujin/dxvk/blob/v3.1.1/src/d3d9/d3d9_interfaces.h)
- [DXVK interop implementation](https://github.com/doitsujin/dxvk/blob/v3.1.1/src/d3d9/d3d9_interop.cpp)
- [DXVK Windows shared-resource limitation](https://github.com/doitsujin/dxvk/wiki/Feature-support#shared-resources)
- [Vulkan external Win32 memory](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_external_memory_win32.html)
- [Vulkan Win32 keyed mutex](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_win32_keyed_mutex.html)

Vendored headers: Vulkan-Headers v1.4.304 (Apache-2.0/MIT license included),
DXVK v3.1.1 COM interfaces (zlib license included; include paths adapted).
