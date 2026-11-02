# Exact native position programs — 0.23.21 candidate

Extends the previous seven world-color profiles using the original local native
Shader.Cache. The inventory contains584vertex programs. For the532programs
declaring rigid-world or17-instance transforms, a bounded symbolic interpreter
proves the complete POSITION expression from fresh D3DDisassemble output of the
hash-verified bytecode. CTAB names alone never authorize a profile.

462programs have exactly the supported transform.70remain unsupported, including
68with additional blend/flex transform dependencies and two special position
programs. Unknown instructions/operands taint position; control flow is rejected.
Five mutated position/semantic/control-flow programs test conservative rejection.
The generated452additional exact profiles join the previous10rigid/instance
profiles and one existing wind companion, for463native position profiles total.

Every added original program must additionally pass actual DXVK GPU depth parity
and production collector history/identity tests, including non-diagonal world
transforms, perspective projection and instance rows0,3,16. The runtime only
matches exact hashes; material/pass, entity identity and buffer-write history
checks remain independent and mandatory. This does not classify an entire scene
as static and does not permit unsupported alpha/blended/deformed passes.

The dedicated native sky shader67ecae639f0848ac was also identified offline. It
uses inverse projection/view, scrolling cloud coordinates and apparent-depth
logic. Cleared depth alone is insufficient evidence of sky coverage, and clouds
cannot be treated as a static world surface. No guessed sky motion is enabled.
The diagnostic survey now reaches DrawPrimitive, DrawPrimitiveUP and
DrawIndexedPrimitiveUP, including depth-disabled sky/color passes. UP caller
memory is never retained or mislabeled as a stale bound VB/IB. Real hook tests
use the original sky shader and verify no motion jobs are invented for it.

The native shader cache is bounded to1024entries (the local native inventory
contains584VS), avoiding the former64-entry recognition limit in varied scenes.
Explicit rejected-pass counts distinguish viewport/depth, colorless occlusion,
blend/alpha-test, stencil, clip planes and partial scissor, with other input
refusals separate. These checks remain strict; diagnostic counters do not relax
material eligibility. All additional work stays inside requested capture scopes.

Candidate only: no new live headset capture and no installation. Last actual
game coverage remains the0.23.19PID2768capture. Game DLAA remains disabled until
actual scene/sky/reactivity, jitter, linear color and NGX eye output are validated.
