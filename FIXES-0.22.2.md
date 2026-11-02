# Gothic 3 VR 0.22.2 — independent stab speed and native action controls

## Stab speed

1H Melee Weapon, 2H Melee and Staff / Halberd each gain Minimum stab speed, independent of Minimum swing speed. Default 50 cm/s, range 40–400 cm/s, steps of 10 cm/s. Existing Stab distance minimum remains separate: default 15 cm, range 5–100 cm, steps of 5 cm. Save settings applies live. Increasing swing speed does not raise the stab speed requirement.

A stab requires coherent forward movement of the actual tracked weapon handle along the blade, at its own speed and distance thresholds. Slow travel, wrist rotation alone, tremor, previous sideways travel and stale samples do not qualify. Both damage and stamina share the qualified stab gate; existing swing distance/velocity gates remain independent. Native per-stroke costs, damage scaling, power attack, learned Murder/Pierce qualification and target deduplication remain. Lowering general stab settings does not lower the separately qualified skill requirement.

## Lockpick without a GUI page

The previous bridge sent A/D only while a GUI/menu or a specifically detected Lockpick routine was active. Native object activities can lack both flags; their world movement was suppressed and no directional key was delivered. The bridge now also routes the left joystick during native object interactions classified from their action/animation/task/state. It excludes native finishing attacks and animal form. This broadens input delivery without directly opening a chest or altering the puzzle sequence, skill checks or lockpick consumption.

Centre the left joystick when the interaction starts. Deflect left/right for an 80 ms A/D pulse; centre to repeat the same direction. Holding the stick does not repeatedly guess the sequence. Opposite-direction changes generate a new pulse. Gameplay locomotion and turning remain suppressed in these interactions; the right trigger retains the native interaction action. No menu pointer is created for a non-GUI activity. Focus/tracking/context loss releases input.

The A/D mapping comes from the user's actual keyboard test. vr_menu_direction records sending A/D; vr_native_control_received records the original native OnAction handler receiving logical StrafeLeft/StrafeRight. These are distinct evidence: successful SendInput alone is not proof that the puzzle accepted a guess. The live minigame still needs confirmation.

## Sneak uses the logical native action

Hold left grip while starting to move; after the existing 120 ms grip qualification the plugin dispatches SessionKey.Down (logical action 14), which is the action normally bound to Ctrl. It does not simulate a Ctrl press. Changing the keyboard binding, including to a different key, does not change VR Sneak.

The dispatch invokes the hash-verified original gCCharacterControl_PS::OnAction on the application thread, preserving the native pressed/before/duration properties and OnPlayerGameKeyPressed script path. Local Script_Game analysis shows OnPlayerSneak reads the pressed property, updates the native Sneak input flag, and requests action 77 for a human. Previously selecting movement mode 2 alone did not execute that path. Native perception, NPC reactions and skill restrictions remain in the game.

While a fresh controller grip owns Sneak, keyboard Down polling cannot cancel that owned hold. Releasing grip, menus, recovery, native activities, animal form, focus/tracking loss, camera reset, movie entry, VR shutdown and world teardown release ownership. A new player identity is never dispatched through a stale actor pointer. No Sneak movement-mode override is applied; the native handler selects the state. Outside owned VR Sneak, keyboard action handling remains native. Left grip used to support a two-handed weapon is excluded; the learned Sneak skill is still required.

The lower eye icon and haptic transition now follow the native Sneak action/animation/state, rather than merely an authored movement mode. vr_native_sneak_action records logical input dispatch; vr_native_control_received and vr_input_context record native receipt/state. No NPC comment is forced by the plugin. Actual native perception/NPC response still requires the user's live test.

## Validation and installation

69/69 automated tests pass. Tests cover separate fast stab/slow swing thresholds, rejection of slow stabs, Windows INI defaults/round-trip/bounds, non-GUI interaction A/D and trigger handling, focus release, native Sneak without physical keys, sustained holds, skill/support gates, native x86 thiscall/reference/int-return ABI, actor changes, stale time and callback re-entry. All 15 launcher pages pass layout checks; the 1H, 2H and Staff pages have been visually inspected. Staff settings use two columns to keep all values readable.

The finalized bow/crossbow headers, tracked arm and rigid melee attachments, native combat/skill rules, existing joystick direction filter, activity camera and finishing rules are byte-identical to 0.22.1. Native game DLLs are unchanged. The new native action message layout is derived from the verified Game.dll OnAction implementation and matches the existing parry bridge. Investigation disassembly is retained as evidence; no native binary bytes are patched for this logical input bridge.

Installation backs up binaries and preserves the current user INI byte-for-byte. Missing stab speed keys load 50 cm/s until the next launcher save. The game is not launched by the installer. Headset contact, live lockpick acceptance and native Sneak/NPC response remain pending user confirmation.
