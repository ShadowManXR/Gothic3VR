# Gothic 3 VR 0.22.4 — sprint/jump visibility and revised controls

| Action | Controller |
|---|---|
| Native Sprint on/off | Tap L3 once; tap again for normal movement |
| Sneak on/off | Double-tap Y, with presses within 450 ms |
| Draw / sheathe weapon | X; also the existing native radial weapon command |
| Main menu / Back | Left Menu button |
| Radial | R3 opens; left joystick selects; A confirms; B cancels; Y changes page |
| Camera reset | L3 + R3 held together for 0.5 s |

Left grip is free for 2H/staff/crossbow support. Sprint is toggled on L3 release rather than requiring a hold. It persists across joystick movement, but native Sprint input releases at neutral so idle stamina is not consumed. Native speed, stamina, skills and movement remain authoritative. A second tap returns to the ordinary native Run movement (not Walk). Menus/radial suspend input and discard pending taps. A camera-reset chord cannot also toggle Sprint. Focus/world loss resets the toggle. Sneak has priority over Sprint.

Y now requires two distinct presses within 450 ms; a single press, a held button or startup hold cannot toggle Sneak. A 30 ms release is required between button edges, preventing tracking/button chatter from counting as a second tap. Menus/radial discard a pending first tap while preserving selected Sneak intent. Actual Sneak stance still controls its eye icon and haptic; learned skill checks remain native.

## Sprint and jump transparency

The verified native Game.dll camera OnProcess makes the player and item hierarchy fully transparent (alpha0) in movement modes5 (Sprint),6 (JumpSteer) and13 (FallSteer), explaining invisible weapons that still produce tracked damage. Enabling rendering flags alone did not remove this opacity.

During one first-person stereo render the plugin snapshots the alpha of the live human player and all bounded children before mutation, sets alpha1 for both eyes, then restores each original value in the render finally block. Native head hiding is preserved and third-person/animal/activity/finishing rendering stays native. No physics, stamina, damage or animation data changes. Each eye also re-enables only the active tracked weapon using the existing saved rendering flag. This fixes a native re-hide between preparations without changing its attachment or calibration. Logs record native alpha, sprint/jump state and scoped restoration.

## Drawing and sheathing while Sneak is selected

The native OnPlayerSelectWeapon callback rejects AniState3 (Sneak) and4 before inventory selection. A hash/address-verified SDK-verified function-entry wrapper now handles only the live VR player's owned Sneak in an idle weapon loop. It releases the owned native Down input and temporarily uses Stand, then calls the original selection script exactly once. The original inventory selection, learned restrictions and equip/sheath tasks remain native. Selected Sneak intent remains on, and logical Down resumes after the native task returns to a settled idle loop for120 ms. No forced fixed equip duration, event replay, skill grant or direct inventory mutation is introduced. Actor/context/intent loss clears the suspension.

## Validation

71/71 automated tests pass. New checks exercise two-eye opacity, exact per-entity restoration including a render exception, collection capacity, hidden-head preservation, double-tap timing/hold/skill/context behavior, persistent L3 Sprint, recenter isolation and Sneak suspension across long native equip tasks. Existing lockpick single steps, separate swing/stab thresholds, frozen 2H palettes, ranged progression, native ABI/GPU and lifecycle tests pass. Fifteen launcher pages pass layout checks; Motion and Radial hints are updated.

Native DLLs and game assets remain unchanged. Existing calibration, weapon settings and user INI bytes are preserved. Accepted bow/crossbow handling and locked grip math are unchanged; NativeTrackedArms changes only its active-weapon rendering re-enable guard. The package includes the built binaries, source and verification evidence. New live sprint/jump visibility and drawing/sheathing in Sneak still require the user's headset test; no unattended game launch.
