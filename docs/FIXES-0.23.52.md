# 0.23.52 - guarded single-object DX11 replacement

Actual0.23.51mode3capture live-20261022-190543-393 proved identical nonzero-RGB
coverage in both eyes. Real-material differences average1.012/1.048of255,max32/29,
consistent with known cross-API anisotropic sampling differences. These are not
pixel-identical images.1063eye composites completed; native scene draws remained.

Preserved BOTH actual captured shader register sets and matching view/projection
in a separate local offline profile. With matching trilinear filtering, the
native original shaders and the DX11 port reproduce those camera inputs with
identical coverage/color and depth in both eyes. Lighting textures in that
fixture are synthetic; do not misrepresent them as a complete captured scene.
The older camera fixture shows up to3D24 quanta of native SM3/SM5 depth arithmetic
difference, even with exact coverage. This is distinct from bridge packing error.

Mode4 is the FIRST conditional actual replacement of one known Ardea stairs02
stone draw. It retains exact mesh/program/declaration/native-state/sampler/
lightmap-provider gates. At first selected draw per eye, an isolated original
DIP reference is compared to DX11 color AND depth with current real GPU inputs.
Required: nonempty samples, zero RGB/depth coverage mismatch, mean RGB error<=2,
max RGB component error<=64, depth error<=4D24 quanta. This explicitly accepts a
bounded filtering/precision difference; it does not claim all materials exact.
Each original shader/resource/provider guard still runs on every later draw.
A scene/selection epoch invalidation revokes cached per-eye approval, keeping
original rendering for the rest of this bounded trial.

Only after that eye passes, write DX11 color/depth at the ORIGINAL DIP point,
then return a completed-replacement result through UiCapture. Omit only that
indexed API. Other/duplicate/unknown/rejected draws stay native. No later solid
boundary commit or broad Render suppression. Exceptions/refusals retain native
DIP. No visibility/FillSolidRenderOps/AI workload removal yet; ONEOBJECT does not
establish an FPS gain or completed renderer replacement.

NativeWriteback gains an optional RGB-only mask, preserving scene alpha exactly
for this native material. Other bridge callers retain their old RGBA behavior.
UiCapture tests prove observer reentry stays excluded, a true replacement skips
only indexed API, and normal paths/UI state/pixels/counts remain intact.

Tests: release plugin/probes build; native-material12stereo/cull/lighting cases,
6mode3comparison eyes,6mode4actual replacement eyes, unchanged native alpha,
unknown/duplicate draws kept, and deliberate magenta native-reference mismatch
refused with scene pixels unchanged. Independent native and DXVK hybrid bridge
color/depth/occlusion/state tests pass. GPU UI hook/state/reentry/skip test passes.
4relevant final test results passed (failed intermediate strict2D24 gate corrected
from measured3quanta, then targeted tests rerun). Current captured-camera profile
also passes the positive/negative replacement path. In-game mode4 still pending.

Default separate trial launcher now selects mode4. -CompareOnly retains mode3;
-ObserveOnly and-SimplifiedPreview retain modes1/2. Process-local variables only,
normal launcher/backend/settings unchanged; DLAA remains off. All arms/input/
head visibility source files remain identical to0.23.51. No headset/game launched.
