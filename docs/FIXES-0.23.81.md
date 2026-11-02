# 0.23.81 — face the native hero's direction after third-person scenes

Knockdown/get-up and dialogue exits share a one-shot return to first person.
The final upright actor's native forward axis supplies the new view direction;
the actor, enemy alignment and get-up attack remain native.

The previous activity return called SetTarget and immediately read the current
application camera. Retained live logs show this could still face almost opposite
the actor. Offline CP1.75.14 analysis confirms SetTarget rebuilds position and the
actor matrix at 26C, but leaves the first-person camera matrix at 22C and its
rotation history at 2AC unchanged. WishedRotation reads those separate caches.
Dialogue exits also lacked the activity return entirely.

After the existing hash-verified native mode/target calls, seed only the 3x3
rotations in both camera caches from the final actor heading, consume the old
filtered mouse deltas and update the application view before rendering. Recenter
tracking on the next valid stereo pose, preserving calibrated eye height. The
transition tracker persists until commit, resets with actor/camera/world changes,
and prevents VR head/body alignment and controller steering from consuming the
old camera on the last native get-up/dialogue update. It does not impose an
orientation on subsequent gameplay frames. Animal return keeps its existing path. No camera cache
translations, actor transforms, locomotion goals, animation or skill data change.
The existing static third-person dialogue framing is retained.

Validation covers exception transitions, all knockdown/KO variants, native
dialogue with FP flags already restored, both stale camera caches, arbitrary
headings/scales, the exact write set, invalid inputs, recenter, two-eye transforms
and independent head yaw after return. Headset validation remains pending.
