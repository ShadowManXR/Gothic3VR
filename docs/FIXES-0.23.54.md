# 0.23.54 — suspend unsafe DX11 replacement and repair stereo/depth validation

The .53 live trial approved only the right eye: left proof failed (coverage and
color mismatch), but 472 right-eye draws were omitted. The right-eye scene
capture changed zero pixels after writeback. This was a rendering regression,
not evidence of a renderer performance improvement.

- Default experimental launch mode is now 0 (native VR). No extra experiment
  device, per-object resource transport, comparison readbacks or native omissions.
  Explicit diagnostic modes remain available; no game graphics settings change.
- An eye's isolated proof cannot independently authorize omission. Both proofs
  must pass; activation begins in the following frame. A failed eye or epoch
  invalidation revokes the pair for the session and stops mode4 material work.
- Reference validation now also composites onto an actual native depth prepass
  in private targets. Color equivalence on cleared depth was insufficient.
- Correct D24 packing: floor(q+0.5) loses integer precision at far depth.
  Correct decoding: recover bytes first and normalize with split power-of-two
  arithmetic, preserving prepass equality without a depth bias or ALWAYS test.

The new offline native-prepass regression failed before the rounding correction
and passes after it. Tests include stereo-pair activation, asymmetric refusal,
native fallback, original alpha, and previously validated material/occlusion cases.

Limitations: initial paired proof is not a transactional guarantee against a
later failure after the first eye commits. Single-object transfers and fences
still cost time; this is not a performance-ready renderer. Default replacement
stays disabled pending broader depth/scene and batching work. No new headset
test is requested for this mitigation. Normal arm/input/camera settings unchanged.
