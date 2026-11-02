# 0.23.15 — animated geometry correspondence for the DLAA input probe

The 0.23.14 live capture (PID 18512, XR frame 1058) has matching scene/color
and early depth in both eyes. Late depth is also clean in this capture; the
earlier invisible hero cannot be attributed conclusively to late occlusion.

The opt-in four-frame diagnostic now associates supported animated indexed
draws with the native entity property ID (GUID and count), visual/context,
geometry bindings and the exact observed shader. It copies the actual 64-bone
palette and projection registers and retains the preceding frame separately
for each eye. SDK objects and GPU buffers are not retained. Reordered draws,
conflicting repeated draws, missing objects, gaps, reference changes and root
teleports invalidate correspondence instead of silently reusing another pose.

The native animation hook is guarded by the existing whole Engine.dll hash,
the exact exported symbol/RVA and complete relocatable instruction prefix.
It always calls the native renderer; no animation, visibility, camera, render
state or shader is changed. No armed capture means no palette/device queries.
Draw collection is bounded to 256 queried draws and 64 recorded draws per eye.
Only three disassembled skinning shader hashes and single-stream indexed
geometry are supported. This is partial coverage, explicitly labeled as such.

Tests exercise four-weight affine skinning/projection and pixel motion sign,
entity/geometry correspondence, independent stereo histories, ambiguity and
resets; a real DXVK device and captured Gothic shader validate palette reads,
scope masking, pass rejection and preservation of native device state/references.

This build does not enable DLAA. These are diagnostic palette histories, not
rasterized scene motion-vector textures. GPU input transport, complete moving
geometry coverage, jitter, masking and the game-side NGX connection remain.
