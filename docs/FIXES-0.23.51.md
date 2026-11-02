# 0.23.51 - isolated live native/DX11 material comparison

Actual0.23.50mode3 trial live-20261022-185327-167 succeeded in feeding the real
SDK atlas vertex stream and live GPU noise/lightmap/shadows to the DX11 stone
renderer in both eyes (415left/380right composites). Both automatic first-eye
before/after images changed zero pixels, although the isolated DX11 geometry
covered20319/7620sample pixels and differed from scene pixels in its footprint.
This is NOT evidence of material parity: scene occlusion/depth can reject the
entire composite. No FPS improvement or completed replacement is established.

The next trial captures the selected actual original DrawIndexedPrimitive into
private same-size native color/depth targets, retaining current native streams,
programs, constants, samplers and draw arguments. It compares that isolated
image against the DX11 image from the SAME draw/frame/eye. Once per eye only;
no readback in normal launches. Temporary targets and original MRT/depth/viewport/
scissor are restored on success or exception. No scene target is cleared and
the actual original scene draw still executes. Coverage metrics based on nonzero
RGB are explicitly named RGB coverage, not a full native depth comparison.

UiCapture now guards recursive world-observer calls so the diagnostic replay
cannot recursively trigger material capture or count as a gameplay draw in its
profiling/temporal observer. The GPU UI test performs a nested draw from the
observer and verifies original counts, native pixels and state remain correct.

Original-DIP and DX11 isolated images and numerical comparison JSON are saved
automatically along with existing scene before/after evidence. Parity flags and
native draw suppression remain false. Do not use zero scene delta as proof.

Validation: release plugin and standalone live probe compilation; real GPU UI
restoration/reentry test passes; native-material probe passes12stereo/lighting/
cull cases and6mode3eye frames. New isolated fixture metrics enforce nonempty
coverage, zero RGB coverage differences and at most1UNORM step difference.
The measured fixture had3changed components at1step for eye0 and0for eye1.
This fixture uses synthetic lighting inputs; actual game material parity still
requires the new automatic live isolated images. No game/headset launched.

Existing0.23.50arm behavior and all input/settings/graphics/backends preserved.
DX11 experiment remains opt-in via Start DX11 Trial.cmd; DLAA stays off.
