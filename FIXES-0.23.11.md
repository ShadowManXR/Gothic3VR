# 0.23.11 — controller movement and turn consistency

- Consume controller yaw at native camera rotation evaluation, rather than at
  translation previews. Nested translation queries retain the same delta;
  another camera evaluation cannot repeat it. Physical mouse input is restored.
- Commands accepted from fresh controller data remain owned for their application
  update, even if expensive native work delays the camera or movement callback.
  The next update must sample fresh input; no queued movement or catch-up turn.
- Replace the four directional timers and wished-input accumulator before each
  joystick-owned native action batch. Do not retain an earlier opposite/side
  action. Native scripts, goals, collision, speed and physics still process it.
- Temporarily select camera-relative input for controller walking if combat
  selected target-relative control. Restore the native reference afterwards;
  neutral-controller keyboard movement and NPCs retain their reference.

Validation: regression coverage includes translation previews, slow native update,
x86 matrix-return ABI and exception propagation, opposite native input residue,
target facing opposite the visible camera, input release and native reference
restoration. Live headset confirmation is still required; existing logs predate
this build and did not capture a forward/backward reversal directly.
