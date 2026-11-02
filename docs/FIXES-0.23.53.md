# 0.23.53 - activate native integration for DX11 replacement mode

User reported an immediate FPS gain in0.23.52mode4. Actual capture
live-20261022-193532-062 has ZERO candidates,scopes,materialdraws or replacements
in either eye. The user observation cannot establish a DX11 performance benefit:
this session rendered through the existing native path. Runtime accepted mode4,
but Diagnostics::InstallTemporalPhaseCapture accepted only modes1..3. The
experiment therefore had no native scene/material integration. This was missed
by the isolated prototype tests, which do not execute game-native hook startup.

New NativeTrialMode.h parses the process opt-in for BOTH Diagnostics and Runtime,
accepts exactly modes1..4 and rejects missing/malformed/truncated values. Remove
both duplicated mode whitelists. Native startup logs vr_dx11_native_hooks_opt_in
mode=N. The actual game hook bodies/prefix/version checks remain unchanged.
The mode-binding test covers1..4,invalid5/0/extra characters,null and truncation.

Before actual selected material work, suspend FFR for the isolated reference and
native color/depth compositor, then rearm the same eye's world FFR state. On FFR
failure disable it safely, retaining the completed-replacement result if one
already committed. A cheap NeedsMaterialWork guard excludes unrelated native
DIPs and completed material work from this callback. It does not perform extra
sampler/program queries or FFR rebinding on unrelated geometry. The prototype
checks scoped work is requested before replacement and cleared after it.

Validation: release plugin/probes compile;2/2 relevant CTest checks pass (mode/
camera/resource binding and12materialcases+6comparison+6replacementeyes+negative
mismatch fallback). Real GPU Runtime test passed directly with verified custom
backend at960x540:FFRsettings lifecycle,stereo/colors,UI,copies,menus,cinema,
cancellation,release retry and restoration. Its CTest entry was absent because
G3VR_FFR_D3D9 was empty; do not report the empty CTest invocation as a pass.
Actual in-game mode4 replacement still requires a new live run.

Keep0.23.52replacement policy/tolerances and opt-in mode4 launcher. Normal
launcher/settings/backend/arms/locomotion are unchanged. DLAA off. No game/headset
launched by assistant. No FPS gain claimed from the zero-replacement capture.
