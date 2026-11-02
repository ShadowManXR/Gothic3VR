# 0.23.14 — capture world depth before the later occlusion pass

The real 0.23.13 two-eye capture at 3840x2160 successfully exported depth and
color. Visual review found a close hero head/torso in end-of-eye depth that is
absent from final color. That depth is not approved for temporal evaluation.

Offline inspection of the exact Engine.dll now establishes a useful boundary:
eCFrustumDatabase::Execute calls DoEndRenderSolid at RVA1a0d89, then begins
another Z pass and calls RenderDynamicOccludees at RVA1a0ef3. Native animation
Z rendering also directly invokes HardwareSkinner, independently of its normal
color Render path. This establishes the render ordering; the next real capture
must confirm whether this late pass is the source of the visible discrepancy.

This build captures another diagnostic depth candidate immediately BEFORE the
EndSolid callbacks and later occlusion work. The existing end-of-eye readback
is retained as a same-frame comparison. It does not disable the game's
occlusion culling, change character visibility or modify any render state.

- The phase hook requires the existing full Engine.dll hash check plus exact
  symbol/RVA, seven complete position-independent prologue bytes, verified
  native call instruction and return address. It only acts on the application
  thread inside the active stereo eye. The original callback runs once.
- Only the full-resolution offscreen texture target with matching depth and
  viewport is considered. Shadow/low-poly or non-texture targets are rejected.
  The phase snapshot is taken at most once per eye for the diagnostic frame;
  subsequent occlusion writes cannot overwrite the stored result.
- Capture waits for 60 consecutive ready world stereo frames, avoiding the
  initial save-load/camera transition. Menus or gaps restart this warmup.
- vr_temporal_depth_snapshot with phase=opaque_before_occlusion saves an
  additional *-opaque.f32 for each eye, associated with the same final eye BGR
  image. It describes the phase explicitly; missing callbacks produce an
  error record, not a silently substituted late depth. Transparency/overlays
  after this opaque boundary still need separate temporal treatment.
- Snapshot buffers are released after writing or cancellation. No request
  means no depth readback. Capture can pause a frame and is excluded from
  normal performance interpretation.

Real GPU regression simulates visible scene depth followed by an invisible
late depth write, verifies preservation of the early candidate, exact saved
values for both phases/eyes, matching colors and released allocations. Native
file tests verify the callback prologue and call ordering against the real
Engine file; the warmup test covers menus, duplicate eyes and missing frames.

In-game DLAA remains disabled. This fixes the input selection mechanism;
live content validation, previous per-entity skin palettes, GPU motion/depth
transport, temporal history and actual NGX evaluation remain required.
