# Separate remaining native scene costs

Actual0.23.24 PID7784 confirmed all nine previous hooks. Instance batching,
material boundaries and the regular render-operation path do not account for
the full inclusive native scene cost. A separate read-only ten-second thread
measurement saw94.4%of one CPU, including driver/spin work, not just arithmetic.
No controlled performance improvement has yet been demonstrated.

0.23.25 appends eight exact native hook entries: ShadowMapBase::RenderShadowMap,
CameraBase::RenderLowPoly, ShaderMaterialAdmin::BeginZPass/EndZPass,
animated/mesh RenderZPass, RenderOp::ExecuteZPass and module BeginRenderSolid.
The existing EndRenderSolid capture hook times its original module callbacks;
it retains the capture-before-callback boundary and installs no second detour.

All work executes unchanged, once. There is no shadow or culling reuse and no
detail reduction. Sampling remains one of32 world stereo frames, excluding
temporal capture; worker threads and unsampled calls do not collect timings.
Whole-file hash, symbol, RVA and complete prefix checks remain. Static SDK type
checks cover the adapter signatures. The earlier batch relocation fix remains.
Native legacy/tree Z-pass methods are proven immediate no-op stubs and left alone.

Old phase IDs0..39 remain stable; nine appended sampled_* phases have IDs40..48.
Expected execution-hook mask is131071 (17entries). Missing bits are unsupported
measurements, never proof of no work. Current metrics are inclusive/nested;
RenderLowPoly and shadow scopes can contain other scene/geometry scopes.
Do not add parent/child costs or infer a precise scene interval from mixed logs.

This release improves attribution only. It claims no FPS gain and does not
enable DLAA. Native files, DXVK/PUP, launcher and user settings are preserved.
