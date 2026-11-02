# Gothic 3 VR 0.22.1 — short stabs and actual lockpick controls

## Short physical stabs

1H Melee Weapon, 2H Melee and Staff / Halberd gain an independent Stab distance minimum. Default15cm, range5–100cm, five-centimetre steps. Save settings applies live. A qualifying stab uses this distance for both damage and stamina, instead of the long damage/stamina swing distances. Your existing swing distance, minimum speed and calibration values are preserved.

Stabs require fast forward handle movement along the weapon axis. Minimum swing speed still applies to stabs; translating slowly cannot qualify. The handle must travel coherently forward, with at most30 degrees of weapon-axis drift; wrist rotation alone, sideways cuts, tremor and previous slow/sideways travel cannot create stab credit. Distance comes from tracking-space blade roots, excluding locomotion and native animation. Equipment/session/tracking resets and duplicated XR samples follow the same safety policy as swings.

The native per-stroke budget is shared: a stab and a later cut in one continuous stroke cannot consume attack stamina twice or bypass per-target deduplication. Native damage scaling, learned power attack and unsupported2H rules remain. The learned Murder/Pierce qualification retains its existing charged sneaking blade conditions and15cm coherent thrust requirement; lowering the general stab setting does not lower that skill requirement. No direct kill is added.

## Lockpick — A/D from the left joystick

The user confirmed the existing puzzle responds to keyboard A (left) and D (right), with no extra held key. The previous bridge sent arrows and only enabled A/D through a native routine100/task detection that did not cover this puzzle. Menu directional commands now always use actual keyboard A/D (SDK keyboard offsets0x1e/0x20) while a native menu/puzzle is open, independent of routine/task recognition.

Return the left stick to neutral when the puzzle opens. A left/right deflection sends an80ms key press and releases it, producing one guess per deflection. Holding the joystick does not repeat the guessed sequence; return to centre and deflect again to repeat the same direction. A direct change to the opposite direction sends that new direction. Focus or tracking loss, entering/leaving a menu, layout changes and radial capture release input. Native routine changes inside an open puzzle no longer reset the directional command. Native world movement wishes remain zero during menu navigation.

Right-hand menu pointer/trigger and Back retain their controls. Native skill checks, puzzle sequence, lockpick consumption and chest opening remain untouched. vr_menu_direction logs A/D press/release and SendInput success so a live failure can be distinguished from a missing physical command. A successful injected key event alone is not proof that the minigame accepted it; live headset testing remains pending.

## Manual and verification

The publisher's English Gothic3 manual, checked at https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/39500/manuals/G3_Manual_UK.pdf?t=1752672619, describes chests in section8.5 but does not specify this minigame's directional keys. The actual A/D mapping is based on the user's keyboard test and the local SDK keyboard enum, not guessed from Gothic1/2 controls. The installed native binary/script scan is retained as investigation evidence; no native module was modified.

67/67 automated tests pass. The new test covers fast short thrusts with a150cm swing threshold, shared native stamina debit, low-speed travel followed by a flick, sideways motion, wrist rotation, tremor, distinct stab settings, duplicate samples and tracking/equipment/speed changes. It also tests A/D emission when no lockpick routine is detected, one input per joystick deflection, native-state changes during an open puzzle and focus-loss release. Real Windows INI tests cover separate profile defaults, round-trip saving,5cm steps and bounded values. All15 launcher pages pass layout checks; updated1H/2H/Staff pages are visually inspected.

The finalized ranged headers, tracked arms/locked melee grips, native locomotion filter, native combat rules and skill qualification headers are byte-identical to0.22.0. Current user INI bytes are preserved by installation. The installer backs up the old binaries and does not launch Gothic3. Headset stab/contact and live lockpick acceptance still need the user's test.
