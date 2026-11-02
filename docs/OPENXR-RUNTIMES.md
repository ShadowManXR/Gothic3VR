# OpenXR runtimes

Select Camera & controls > OpenXR runtime, save, and restart Gothic 3.

| Setting | INI value | Connection |
| --- | --- | --- |
| Virtual Desktop / VDXR | 0 (default) | Connect in Virtual Desktop. Existing direct x86 path. |
| Meta Quest Link / Air Link (64-bit host) | 1 | Enter Quest Link/Air Link before launching. |
| SteamVR (OpenXR) | 2 | Start SteamVR with the headset connected. Direct x86 path. |
| System default OpenXR (32-bit) | 3 | Reads inherited XR_RUNTIME_JSON, otherwise the registered x86 runtime. A standard Meta manifest uses the x64 host. |

No Windows default runtime is changed. Old INIs retain VDXR. Runtime discovery
validates manifest and DLL architecture. Explicit choices never silently switch
provider. Existing camera, controls, stereo, HUD, FXAA and FFR are retained.
Bindings cover Quest/Touch, not every SteamVR controller model.

## Meta bridge, version 0.23.63

On this installation Meta's RuntimeIPCServiceClient_32.dll crashes during
xrCreateSession. The same minimal D3D11/OpenXR program built for x64 creates
and destroys a session successfully. A service restart did not resolve x86.
OpenComposite translates OpenVR to OpenXR and cannot bypass this direct
OpenXR failure.

The x86 game now launches its own x64 Meta compositor process. Required files
under scripts/Gothic3VRDiagnostics/meta64:

- Gothic3VRMetaHost64.exe
- openxr_loader64.dll (official Khronos loader; renamed to avoid a duplicate
  resource basename with the existing x86 loader)
- OPENXR-META64-LICENSE.txt (unique resource basename)

The host uses the installed Meta x64 manifest. Its modern Windows manifest and
clean compatibility-layer environment avoid obsolete Windows-version overrides.
It requires neither OpenComposite nor SteamVR. VDXR and SteamVR do not launch
this host. Meta files and global runtime registrations remain untouched.

A versioned local pipe carries timing, events, poses, layer descriptions and
actions. Handles are fixed uint64 values; pointer-bearing structs are serialized
field by field. Controller states and LOCAL/VIEW hand poses are batched once per
action sync. Projection/UI quad layers, haptics and recenter events are forwarded.

Images stay on the GPU. One named NT shared keyed-mutex texture per swapchain
is reused across native XR slots. The host copies into the acquired Meta image,
completes the GPU batch and releases it before submission. There is no production
pixel readback or extra queued frame. The copy and synchronous IPC have a cost;
equal performance across runtimes is not promised. A kill-on-close job owns
the host so it cannot remain running after the game exits.

Build the game with the existing Win32 project. Build the host separately:

```powershell
cmake -S tools/MetaBridgeHost -B build-meta-host64 -A x64
cmake --build build-meta-host64 --config Release
```

Copy the three host output files into the directory above. Keep
G3VR_META_GPU_FIXTURE=OFF in every installed build. The fixture enables CPU pixel
verification for offline tests only and must never be distributed.

## Validation

Final host: native Meta session, swapchains/actions, 32 GPU transfers,
keyed-mutex reuse and shutdown passed. A separate fixture verified actual XR
destination pixels and UI transparency. x86/x64 wire records match byte-for-byte,
including high-bit handles and projection/quad layers. Runtime selection,
environment restoration, settings preservation, stereo, UI submission and GPU
upscale regressions pass. Tests and fixtures stay outside the game directory.

Visible headset frames and physical controller input still require Quest Link
validation: both eyes, HUD/radial, both controllers, movement, weapons and
recentering. Runtime-specific FPS is unmeasured.

OpenXR loader: https://registry.khronos.org/OpenXR/specs/1.1/loader.html

## Meta frame pacing correction

The first timed Link trial (stereo-game-31681921-24156) used an 80 Hz runtime.
49 selected active-scene windows, excluding initial full-rate and long-loading
windows, had median app submission FPS40.772, blocking xrWaitFrame6.548ms,
application/IPC9.911ms, GPU-copy synchronization0.337ms and xrEndFrame0.051ms.
These are application submission timings, not headset/encoder FPS or a matched
VDXR benchmark. The uninstrumented interval between submissions and the next
wait was approximately6.2ms. Serial runtime waiting is therefore an actionable
cost, not evidence that the image copy or controller IK is the bottleneck.

The Meta host now uses one persistent wait-only worker. After xrEndFrame it
starts the next xrWaitFrame, allowing that wait to overlap Gothic's following
native update. Exactly one wait result can be outstanding. Poses/actions are
still located at render time using that result's unchanged predicted display
time. No images are queued and no game simulation is moved to another thread.
Expired results after loading/suspension are completed as empty frames before
a fresh wait. The worker drains before session destruction/restart. A runtime
wait stuck beyond the shutdown bound exits only the owned host; the game-side
job remains the final lifetime guard. VDXR, SteamVR and gameplay paths are unchanged.

A one-shot meta-frame-timing.request beside the host enables bounded CPU-only
reports for120seconds. It is consumed at startup and is not enabled by default.
There is no per-frame disk log, GPU query, pixel readback or permanent HUD.
Reports distinguish caller-blocked wait, native wait and the part overlapped
with native update. Synthetic concurrency and measurement tests pass. Actual
headset movement improvement still requires validation; no FPS gain is claimed.

Frame wait threading/call-order reference:
https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/sources/chapters/rendering.adoc
