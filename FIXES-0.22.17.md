# 0.22.17 — keep the body under the headset during locomotion

The 30-degree body free-look cone previously remained active during locomotion,
leaving the neutral torso behind the head-directed walking direction. Room-scale
follow also tracked the headset's optical position without compensating the
configured forward camera distance; the rendered body remained at the actor
origin while the eye moved around it.

- Retain 30-degree free look while stationary. Native wished movement aligns
  the torso's forward heading to the head-directed frame during locomotion.
  Backward/sideways gait does not flip facing by 180/90 degrees. The input and
  render paths use the same heading rule.
- Add a shared horizontal neck/root target: physical headset displacement plus
  native-camera forward offset minus that offset rotated with the headset's
  horizontal view. A physical head turn around the neck no longer looks like a
  lateral room step. Looking vertically uses the headset right axis as fallback.
- Feed this neck target into the existing collision-checked room-scale catch-up.
  Preserve its step limits, 8cm lean margin, native ground/height checks, state
  restoration and velocity-preserving transfer. No direct joystick velocity
  controller, forced motion reset or unvalidated teleport is introduced.
- Use the same current-frame target to center the rendered pelvis/torso/legs
  under the headset while the native collider catches up. Keep the existing
  vertical ground path and shoulder/arm IK. Do not feed this rig translation
  into eye, controller, weapon or interaction transforms. The same rig snapshot
  serves both eyes, including two-handed frozen palettes.
- Keep the user-accepted audio fix and all hand/weapon calibration unchanged.
  No launcher, game INI or user VR setting is rewritten.

Validation: 84/84 CTests pass. New replay covers 1200 forward/backward/native-turn
and physical-turn frames at Gothic-scale coordinates, headset rotation around a
neck pivot with multiple forward offsets, shared body grounding, unchanged eyes
and body across accepted room transfers, recenter yaw and vertical gaze.
The existing tests cover native room transfers, collision-result bounds, hands,
weapons, settings and the absence of the unsupported FMOD getter. Headset and
terrain acceptance remains pending; offline math is not a live gameplay test.
