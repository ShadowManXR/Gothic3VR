# Conservative nonblocking VR occlusion â€” 0.23.28

Concrete native render change, not an additional diagnostic build. Native
Engine GetOcclusionTestInfo (RVA aa670) uses a tight GetData(FLUSH) retry loop
when ResolveOCQuery requests wait=true. ExtractFromFrustumOC requests that
blocking resolution at RVA19eb5b and compares returned samples unsigned to its
visibility threshold at19eb6a. This path is a demonstrated WAIT CAPABILITY,
not proof of the fraction it contributed to the previous Ardea capture.

The verified DX9 device virtual slot159 is replaced. Only the original native
ResolveOCQuery call site (return RVA1a4403), during a VR world eye on the render
thread and wait=true, uses a single GetData(flags=0). Ready results (including
zero) are unchanged. Pending results return INT32_MAX, a conservative visible
count distinct from native unresolved sentinel UINT_MAX. Native Resolve then
returns the handle to its own pool exactly once. No query is retained, freed,
allocated, AddRef'ed or manually recycled by the plugin. HRESULT errors remain
errors and retain original native ownership. Unknown objects/types/callers,
nonblocking queries, menus, desktop, workers and active temporal capture use
the original route. Existing D3D9/D3D11/OpenXR image-copy fences are untouched.

This can trade extra GPU work for removal of CPU/GPU query waits: pending
occluded geometry may be drawn. It does not intentionally omit visible geometry
or reduce visual quality. No claim is made that this alone fixes sustained
Ardea CPU load, nor that it improves FPS on every route/GPU. Next user trial
should evaluate this actual change, not collect another diagnostic-only probe.

Profile checks: full native Engine hash (existing stereo installer), exported
method/RVA, native entry bytes, exact static vtable at42aafc, SDK Release thunk
slot159, and Resolve's vcall return bytes. Unknown hooks are not overwritten.
G3VR_ASYNC_OCCLUSION=0 before launch restores native waiting behavior. Default
is enabled for eligible VR calls. Startup ready/refused and small 2-second
counters indicate actual use without restoring detailed profiling. 0.23.27
scene/per-draw profiling remains opt-in; bow aiming is user accepted; DLAA off.

Offline tests execute actual copied native method and Resolve bodies with a
fixture admin and a pre-reserved native pool. They verify native wait loop
flags/call count, pending fallback, ready zero/nonzero, failure ownership,
wrong caller, desktop/nonblocking/type bypass, and no query refcount changes.
No game DLL is initialized. GPU tests on native D3D9 and supplied DXVK each
reissue one pending query 256 times, then verify real4096 visible pixels, real0
occluded pixels, and reuse after zero. Test-only event fences are NOT added to
the runtime optimization. SDK slot and existing relevant VR tests also run.

Microsoft's recommended pending-occlusion fallback follows the same principle:
https://learn.microsoft.com/en-us/windows/win32/direct3d9/queries

No actual in-headset gain measured yet. Keep 0.23.27 backup for comparison.
