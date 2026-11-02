# Controller mapping

OpenXR reports logical inputs such as thumbstick click. L3/R3 are familiar labels, not hardware strings that the game searches for. The existing interaction-profile declarations select available inputs; remapping cannot assign a System/Home button reserved by a runtime or invent a button absent from the controller. Untested hardware is not certified by accepting a binding declaration.

1. Start Gothic 3 in VR, preferably in a paused menu.
2. Open the launcher through your headset's desktop view and choose **Controller mapping**.
3. Choose a **Game action** and **Activation**. You can choose the button/modifier manually or click **Listen in VR...**.
4. For listening, return focus to the VR game within 20 seconds. Release all buttons, then press one button or hold two together and release. The selected activation is retained; choose **Double tap** when you want a double-tap binding, including a held modifier plus two taps.
5. Return to the launcher and click **Save settings**. The game reloads bindings live; release buttons/sticks before resuming.

The current binding and reported buttons are shown on the page. **Clear action** disables one action; **Restore controller defaults** resets only the mappings. The launcher keeps at least Main menu or Open radial assigned. Shared bindings in overlapping contexts are reported; some defaults deliberately share buttons in different contexts. A chord takes priority over its single-button actions, while a held support grip remains attached when used as a modifier.

Use **Hold** for the bow string, charged magic/power attacks, blocking and held support. Support-grip Single press works with the existing **Toggle support grip** option. Sprint remains sprint-until-stop, and sneak remains a toggle. Locomotion/turn axes, hand poses, native skill restrictions and the radial's right-stick selection are unchanged.

Listening pauses controller actions, not the game simulation. It does not create another XR session, change the active runtime, or start the game automatically. Cancel, leaving the page, closing the launcher or a missing heartbeat releases listening; inputs require neutral before rearming. Capture receives buttons from the existing VR game and needs the headset/runtime to report them as active. Use manual selection when offline.

## World labels

**HUD & menus → World label size** scales native world focus labels for NPCs, enemies, items and signs. 100% is the previous size; try 50–70% for square VR resolutions. It does not resize inventory/dialogue/menu text or health-bar geometry. Save settings applies live.
