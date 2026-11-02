# Renderer DLAA client — 2026-10-20

`RendererDlaaClient` implements the missing client side of the x86 game / x64
NGX GPU exchange. It is built independently; the live VR renderer still uses its
original eye images. No DLAA setting has been enabled and no new capture armed.

The client wraps the renderer's existing NVIDIA D3D11 immediate context, obtains
its actual adapter LUID and exports eight keyed NT textures. Input packing is
GPU-only: explicitly encoded BGRA/RGBA8 is decoded to linear RGBA16F, native
RGBA16F motion is reduced to RG16F pixel vectors, and R32F device depth is copied.
The original context's complete state is isolated/restored with D3D11.1 context
states. Producer and consumer copies complete before shared ownership changes.

Both independent NGX features return private linear RGBA16F images. `Present`
copies a requested latest frame/eye into an XR-compatible BGRA sRGB/typeless
render target with exactly one gamma encoding. HUD composition belongs after
this pass. It rejects stale frame IDs, wrong eyes, dimensions and formats.

Submission requires matching per-eye frame/epoch/time and explicit readiness
for native applied projection jitter, full scene motion, world color without
HUD, standard depth, and reactive material handling. Existing game capture
does not satisfy these conditions. Do not set these flags based on transport
availability or infer sky/static history from cleared depth. All missing input
cases reject before submitting; rejected frames expose no stale output. Frame
gaps, scene epochs and long intervals reset both histories. Explicit individual
eye resets are preserved. A transport failure disables the session and reaps
only its owned worker process.

Common GPU/pipe/protocol headers now live in `src/dlaa`; the standalone prototype
uses those same files, avoiding an x86/x64 wire-contract fork. The official NGX
runtime remains in the standalone worker project and is signature checked there.
No root renderer DLLs, game settings, launcher or installer were changed.

Validation from `work/dlaa-prototype`:

- x86 -> x64 and x64 -> x64 real NGX: 75 stereo frames each at 321x181,
  31 negative cases each, 64 caller pipeline-state restores each.
- Both sRGB and typeless XR targets compared at every pixel against the actual
  returned linear eye image. BGRA8 and RGBA8 encoded fixture inputs preserve
  channels and gamma in both eyes.
- Analytic renderer fixture: error against a 16-sample reference fell from
  [0.00925714, 0.011272] to [0.000923192, 0.000968986]. This is a fixture result,
  not a measurement of Gothic 3 image quality, FPS or headset latency.
- Native system D3D9 -> NVIDIA shared D3D11 -> actual x64 NGX -> XR format:
  8 stereo frames at 97x65; reusing/overwriting the same original native
  surfaces between eyes preserves both outputs; native device refs return zero.

Remaining live integration: native projection jitter, complete motion and late
sky/reactivity composition, then invoking Evaluate/Present before HUD. The
actual sky in PID8536 is VS9cd6b9e1f1627a6f + PSe3a7e3dc1f1c8437 after EndSolid,
with read-only LESS stencil. ShaderSurvey now prioritizes it as sky evidence;
this label does not grant motion eligibility. Its two differently scaled cloud
layers need reactive handling, not one fictitious exact motion vector.

`TemporalProjectionJitter.h` defines the SDK row-vector pixel-displacement
convention without changing clip depth/w or XR compositor FOV. The actual
native camera hook is not using it yet. `TemporalSkyMotion.h` implements the
specific PID8536 inverse-projection/inverse-view ray and rotation-only
reprojection; player translation cannot move the infinite sky. The older
VS67ec program includes an extra environment rotation and is explicitly
rejected by this model, despite being tagged as sky evidence by the survey.

The original unmodified VS9cd6 program was executed on the native GPU with
preserved per-eye constants: 72 direction comparisons, maximum error
1.08753e-7; 24 asymmetric projection/jitter checks and 82 sky model checks passed.
Four shader mutations are rejected by the separate bytecode proof. These
checks prove the ray/rotation math, not live sky coverage or cloud history.

Geometry composition now distinguishes already de-jittered native companions
from future raw vectors via `geometryIncludesRasterJitter` (default false).
Raw layers opt in to subtract previous-current raster displacement; existing
producers keep their original contract. Hardware tests check cancellation,
remaining 3,-4 pixel object movement, overlap, invalid history, and the existing
pre-de-jittered convention. No jitter is enabled in normal gameplay.
