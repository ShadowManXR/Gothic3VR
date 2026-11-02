# Third-person orbit and physical crouching

Local build 0.23.97 adds two independent render features. Following headset validation, launcher 0.23.98 keeps both enabled as fixed creator-approved behavior and removes their switches. Existing camera calibration and settings files are preserved during the local update.

## Third-person camera orbit

Use the right thumbstick horizontally to move the camera around the character during third-person conversations, activities and knockdowns. Release the stick to stop immediately. The stick must pass through neutral after entering a context or returning from a menu/focus interruption. Radial selection retains priority.

The first deliberate orbit captures the current framing. Static geometry clearance keeps the camera away from walls and refuses positions inside the hero. Actor translation remains followed. Orbiting does not write actor heading, tracking origins or native camera AI, so the existing first-person return-facing behavior remains authoritative.

The underlying diagnostic configuration key is `[Controllers] ThirdPersonOrbit`; launcher 0.23.98 keeps it enabled.

## Physical crouching

Lowering the headset progressively lowers the rendered pelvis and torso and bends both knees while preserving the native foot/toe ground contacts. Controller hand targets remain independent of the lowered shoulders. This visual option requires tracked arms, stable body and stable horizon in first-person.

There is a 4 cm standing deadzone and up to 50 cm of combined native/physical body lowering. At a combined depth above 75 cm, the body and head are hidden while tracked hands and held weapons remain visible. The body returns below 65 cm to prevent flickering near the boundary. Visual lowering is smoothed over approximately 85 ms; the camera itself is not filtered by this option.

This changes only the scoped render pose. It does not activate gameplay sneak, change the collision capsule, move the entity or recalibrate headset/controller origins. Recenter while standing normally. The underlying diagnostic configuration key is `[Controllers] PhysicalCrouching`; launcher 0.23.98 keeps it enabled.

### Physical crouch movement limit (0.23.103)

Separately from the visual pose, a deliberate physical crouch now requests native **Walk** for controller locomotion. The headset must remain at least 30 cm below the existing standing reference for 120 ms. The limit clears only after rising within 18 cm of that reference for 120 ms. Small leaning and brief head bobs do not engage it; the gap between the thresholds prevents chatter.

Entering this state cancels the L3 sprint latch. Rising returns to ordinary Run and requires a new L3 tap to sprint again. Only Sprint is cancelled; quick heal, support grip and other gestures remain independent. Native gameplay Sneak retains priority and is not activated by physical crouching. Native direction, collision, terrain, jumps, falls, recovery and special movement states keep their existing handling. Recenter, actor/session changes and inactive gameplay reset the policy; a short pose-cache miss retains the limit for at most 100 ms instead of briefly selecting Run.

Automated threshold, sprint, native Sneak, special-state and visual-crouch regression checks passed. Headset validation of the movement limit is pending.

## Validation

The local build passed 16 focused regression tests and normal/compact launcher interaction/layout checks. Dedicated tests cover orbit stopping/context changes/collision refusal, grounded leg IK, exact tracked hand preservation, hands-only stereo palette reuse, restoration and settings round trips. Bogdan subsequently confirmed both features working in the headset.

Check conversations, activities and knockdowns with right-stick orbit, then verify the usual first-person return. Crouch gradually, crouch deeply, and stand again while checking feet, hands, weapon alignment and unchanged centering.
