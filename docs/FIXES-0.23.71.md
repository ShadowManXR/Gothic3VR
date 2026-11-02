# 0.23.71 - Held-trigger Sprint / L3 quick heal trial

- Hold the **left primary/index trigger** while moving for native Sprint. Release it to return to ordinary Run. This is not a toggle, and holding it while stationary does not request Sprint. Moving again while still holding the trigger requests Sprint again.
- Tap **L3** (press/release the left thumbstick) for native quick health-potion use. Healing is resolved on release; holding L3 does not repeat it. A hold longer than 600 ms is ignored.
- **Left grip/squeeze** still attaches the support hand to 2H weapons, staves and crossbows. **L3 + R3 held for 0.5 s** still recenters and cancels a pending heal. Menu, radial, focus loss and stale input release Sprint and suppress pending heals. Release controls before reusing them after a suspended context.
- The previous L3 Sprint toggle is removed from the active controller translator. Launcher control hints are updated in English.

This trial isolates the Sprint button from thumbstick clicking. Direction quantization, native movement/action transport, sprint eligibility/stamina, actor heading, animation, camera and runtime timing are unchanged. The intermittent native backward-sprint problem is **not claimed fixed** by remapping; compare forward-left and forward-right movement with and without the trigger in the headset.

Validation covers held/released trigger, no idle sprint, restart while held, independent support grip, one-shot L3 heal, recenter/menu/radial/focus/stale-input cancellation and unchanged logical direction on forward diagonals. Headset validation remains necessary.
