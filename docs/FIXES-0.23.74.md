# 0.23.74 - Keep the fixed dialogue camera outside the hero

The first native dialogue frame can still have its camera inside the hero's head. Freezing that frame previously preserved the bad position throughout the conversation.

- At dialogue entry, preserve an existing third-person shot if its rendered eye position is at least 2 m from the hero horizontally and at a sensible height.
- Otherwise place the rendered eye centre approximately 2 m behind and 0.55 m to the side, at 1.65 m above the hero's root. Account for the current tracked head displacement when storing the camera base; do not recenter the headset.
- Check static geometry once on acquisition with five head-clearance rays. Prefer the rear shoulder view, then two nearby rear-side alternatives. In a confined room use the farthest clear position, at least 0.8 m from the hero. If none fits, defer acquisition rather than freeze inside the head or a wall.
- Once acquired, keep the copied camera base fixed across dialogue cuts and orbiting. Live headset rotation, translation and stereo eye separation remain active. There are no continuous collision queries or camera-follow adjustments during the conversation.

Only the plugin changes; the installed 0.23.73 launcher and its settings remain in place. Native camera state is restored after stereo rendering. Character/object focus, L3 Sprint, quick-heal controls, render optimizations and runtime configuration are unchanged.

Validation covers transition frames at the head, initial head translation and yaw, different world scales, preserved safe native views, obstructed/narrow rooms, invalid placement data, static dialogue bases, stereo tracking and camera-mode recovery. Headset validation: start several conversations, including near a wall, and confirm that the view starts outside the hero and returns normally on exit.
