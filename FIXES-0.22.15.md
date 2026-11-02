# 0.22.15 — body follows physical headset yaw

The stabilized body previously used only the native camera base, so a physical
turn in the headset left the visible torso facing the old direction.

- Add headset yaw relative to the recentered tracking origin to the body heading.
  Keep a 12-degree free-look cone: small head movements leave the body still;
  beyond the cone, the body follows while retaining up to 12 degrees of head turn.
- Apply native/joystick yaw once, outside the physical free-look cone. Exclude head
  pitch and roll from body rotation, and retain horizontal heading while looking
  straight up/down. Recenter, runtime-session changes and world/actor changes
  clear the physical-body heading state.
- Feed the new heading to the stabilized pelvis, torso, clavicles and arm poles.
  Eye, controller, weapon and interaction transforms retain their original shared
  tracking base; the head rotation is not added to them a second time.
- After native character input, request this body direction with the native
  SetAlignmentLookAt API when ordinary player-controlled ground movement permits
  it. Restore the previous native direction after the native physics update.
  Skip native targets, script-owned rotation, recovery/knockout, third-person
  activities, dialogues, animals, pause, menus and cinematics. NPC input cannot
  release the player's alignment lease. Do not write entity/camera transforms,
  velocity, position, movement mode or collision flags for body following.
- Retain the 0.22.14 knockout third-person, native locomotion guards and centered
  hero speech fixes, and the 0.22.13 same-frame GPU transfer improvements.

Validation: 83/83 CTests passed. The additional test covers the free-look cone,
repeated updates without drift, combined joystick/head turning, three full
physical revolutions, vertical look, recenter/session/actor changes, stale and
invalid tracking rejection, unchanged eye transforms, and native alignment lease
ownership. Actual headset body movement and the earlier audio/knockout fixes
still require user testing; no live acceptance or freeze-resolution claim.
