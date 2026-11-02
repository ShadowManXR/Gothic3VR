# PUP occlusion device compatibility — 0.23.29

The actual 0.23.28 session PID13776 reported vr_occlusion_nonblocking_refused,
not ready. User's subjective slight improvement cannot be attributed to28's
occlusion mechanism. Do not claim that it was active or measured a gain.

PUP1.1.1 constructs eCDX9DeviceEx with Engine's base constructor at unchanged
this, then installs its own vtable RVA80b70. Its occlusion slot159 uses PUP's
FF25 IAT thunk RVA3bd34, pointing to IAT674d4 which imports the original Engine
GetOcclusionTestInfo. Native-only table/entry checks incorrectly excluded this
supported device. New support accepts ONLY this exact PUP table/thunk/import
binding, after the existing full PUP hash verification and module pin. RTTI
base offset zero, constructor, imported symbol and HIGHLOW relocations are
validated offline. No generic thunk following or unknown subclass is accepted.

The vtable replacement preserves the original PUP thunk for bypass calls.
Scope, single non-flushing query poll, positive conservative sample count,
ready/error behavior, and native pool ownership from28 remain unchanged.
If the API device has not yet been created at script startup, installation is
deferred until the first native camera render; unknown profiles are refused
once with a distinct reason. No patch is injected into a running game.

Tests include actual native Resolve/GetData bodies, a PUP-style FF25 original
forwarding thunk and a real virtual-table swap/restore. Real native/PUP PE,
SDK slot159, native D3D9 and byte-identical installed DXVK GPU query reuse and
other relevant VR tests are checked. This is not live activation proof: the
next ordinary session must report ready (code1 for PUP) and nonzero counters.

No new diagnostic-only trial requested. Performance remains unmeasured and
pending geometry can cost more GPU work. Bow aiming is user accepted. Detailed
profiling stays opt-in and game DLAA stays off. See OCCLUSION-0.23.28.md for
original algorithm, limitations, Microsoft documentation and opt-out.
