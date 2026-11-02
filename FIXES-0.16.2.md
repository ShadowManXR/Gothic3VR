# Gothic 3 VR 0.16.2 — ranged equipment recovery

The user reported bows and crossbows returning to the native carry pose at the feet after loading an older save and drawing a quick-slot weapon. Once triggered, both ranged weapons stayed there until restarting the game.

## Evidence and changes

The installed 0.16.1 log `diagnostic-20261012-223440-3824.jsonl` records `vr_bow_palette_prepare_fault; native_bow_preserved` (access violation) at 142149 ms, following a camera/load transition. That catch set the shared ranged renderer's fault flag permanently. It therefore stopped preparing both bows and crossbows. The preceding first-person gate skipped arm preparation, while the previous frame's bow sample remained available to the separate ranged binder.

0.16.2 invalidates render/combat samples before any early-return gate and refuses a bow sample whose player or equipped weapon no longer matches the active native session. Ranged actors, wrappers, node storage and counts are checked when binding, rendering and restoring. A native palette snapshot is never copied into a replacement actor's storage. The parsed model cache now follows the asset name and weapon type rather than a possibly reused factory address; incomplete assets are retried.

A temporary ranged fault no longer disables both weapons for the rest of the process. The same binding can retry after 500 ms; a different player, weapon or VR session can retry immediately. If restoration itself fails, stale borrowed pointers are discarded before recovery. Model/string palette failures preserve the tracked native palette instead of leaving a partially modified skinning palette. Per-eye spatial pose and temporary rendering visibility are restored afterward.

This is a rendering/lifetime correction. It does not reset native reload timers, charge/input rules, ammunition, skill progression, damage, inventory slots or hand/bow/crossbow calibration.

## Validation

52/52 automated checks pass, including bounded fault retry, weapon switching after a bow failure, replacement-player/session identities, invalid-frame re-equipping, clock reversal and palette ownership/restoration. Existing model/string/arrow geometry, crossbow native reload/progression, arm reach, physical combat and GPU stereo checks still pass. The launcher has no code/UI changes in this release. No unattended game or headset probe was launched. The reported save/quick-slot sequence still requires headset validation.

## Headset probe

1. Load the older save which reproduced the problem. Draw a bow using its quick slot/radial, without first equipping it through the inventory. Confirm it follows the left palm; fire several arrows.
2. Switch directly to the crossbow quick slot. Confirm it follows the right palm and fires bolts; switch back to the bow.
3. Holster/draw each weapon repeatedly. Load a second save within the same game process, then repeat the quick-slot sequence.
4. Check menus and a conversation followed by re-equipping. Confirm the accepted grip offsets, left support and arrow/string placement remain unchanged.

If a transient native binding error occurs, logs now record `vr_ranged_render_retry` and subsequent `vr_bow_render_pose` entries instead of a permanent `tracked_bow_disabled` state. A persistent unsupported asset can still lack its evaluated string geometry; this build does not claim headset success for every weapon model.
