# Live temporal inputs — 0.23.19

This is an opt-in game input validation build. It does **not** activate DLAA or
claim complete scene motion. The standalone NVIDIA NGX prototype is unchanged.

The native renderer now supplies bounded skinned, rigid, instanced and wind
motion to the D3D11 temporal compositor for each eye. Engine context and batch
hooks copy property IDs/world matrices; native model rows match those IDs
independently of sort order. Shared geometry drawn for distinct entity groups
keeps separate histories. Actual VB/IB Lock/Unlock writes and ProcessVertices
invalidate content history. Retained resources prevent address reuse across
the two-frame window. Shared resource/device hooks restore before release;
already dispatched callbacks still forward to their original methods.

The skin history limit is 256 draws, native history 1024. Overflow, unsupported
shaders/passes, ambiguous identities, missing native phases and modified buffers
do not receive valid motion. Source inputs freeze at the native opaque boundary.
Both eyes export their depth and motion before the shared Gothic depth is reused.
Private combined motion/depth stay on GPU; a zero known-static mask preserves
unknown coverage instead of inventing camera vectors for unidentified surfaces.

`scripts/Gothic3VRDiagnostics/dlaa-capture.request`, read at VR startup, enables
four stereo diagnostic frames after 60 ready world frames. Metadata is logged
for all four; the second saves skin/native/composed RGBA16F motion plus matching
opaque/late depth and final color. The request is consumed afterward. Diagnostic
readbacks occur only in this explicit capture; normal gameplay does not run the
new collector or compositor. Frame cancellation, reference changes and shutdown
drop temporal resources before the game's graphics objects are destroyed.

Automated validation uses actual captured Gothic vertex programs, actual DXVK
draw/getter/resource methods and real shared GPU copies. Reference geometry and
the CPU camera fixtures are controlled tests, not live Ardea coverage evidence.
The new hook entries are checked against the exact local Engine.dll hash,
export names, RVAs and complete relocatable instructions without executing it.

Still required: validate actual scene coverage and identities from a game run;
handle uncovered shaders/materials, sky and transparency/reactivity; then wire
projection jitter, complete histories and color conversion into the game NGX
worker and final eye output. Never set `AllInputs` from partial scene vectors.
