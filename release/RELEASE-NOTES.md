# Gothic 3 VR 1.0.9

- Lectern reading retains the calibrated first-person VR viewpoint with live headset tracking, including the return from reading.
- Native reading animations, text, learning rewards, interaction timing and scripts remain unchanged.

## Previous release: 1.0.8

- Chest interactions keep the calibrated first-person VR viewpoint instead of switching to the native interaction camera or orbit.
- Keep that viewpoint through chest closing until the native first-person camera has settled, and hide stowed back weapons in both eyes throughout the interaction.
- Suppressed the displayed player's chest animation by retaining the last gameplay skin pose for both eyes. Cold entry without a cached pose temporarily hides the avatar instead. All visual changes are restored after rendering.
- Native chest scripts, alignment, animation events, chest lid motion, locks, lockpicking, loot generation, ownership and quest events continue unchanged.

## Previous release: 1.0.7

- Dialogue now retains the VR player's first-person viewpoint instead of moving to a side/third-person shot. Native speaker cuts and dialogue camera orbit no longer reposition the rendered VR camera.
- Headset look, room-scale movement and stereo depth remain active. Native dialogue scripts, camera AI, subtitles, choices, voice and quest logic continue normally.
- Reuses the calibrated gameplay view; dialogue opened without a prior gameplay frame uses the player's facing and calibrated eye height.

## Previous release: 1.0.6

- Removed the VR player's native 500 ms pickup wait and the animations immediately before and after world-item inventory transfer.
- The original script still handles inventory insertion, stack amounts, quick-slot assignment and pickup rules. NPC and non-VR pickups, other interactions and instruction cancellation retain their native behavior.
- Applies only to the supported native binary profile; incompatible pickup hooks fall back to the original behavior.

## Previous release: 1.0.5

- Updated both hand HUD defaults to the user-calibrated positions and rotations.
- Added separate slot-bar and status-block scale controls (25�200%, default 50%) on the Hand HUD page.

- Added independent hand HUD displays attached to the hero's lower-arm bones: quick slots, item icons and stack quantities on the left; health/stamina/mana on the right.
- Uses the installed game's original HUD background artwork and status colors. No game artwork is distributed with the mod.
- Added a Hand HUD launcher page with an enabled-by-default toggle and separate position/rotation adjustments for each arm. Changes apply live.
- Native HUD widgets keep their original positions, independent visibility options and inventory behavior.

## Previous release: 1.0.4

- Fixed leftover quick-slot and player-vital frames/backgrounds when their gameplay HUD options are disabled.
- Inventory always shows the complete quick-slot bar and health/mana/stamina block; closing inventory restores the saved options.

- Added independent launcher controls for the native compass, slot bar, and health/mana/stamina panel.
- All three HUD displays are disabled in new configurations and when restoring Defaults. Saved explicit preferences remain intact.
- HUD visibility changes apply live in VR; item actions, status values, and the radial menu keep working while the corresponding native widgets are hidden.

## Previous release: 1.0.3

- Added native quick-slot item icons and live stack quantities to the VR radial item wheel.
- Corrected the radial icon lookup to use native slot digits (1–9, then 0), matching the quick-slot key sent on confirmation.
- Added the launcher HUD option to hide the compass.
- Added the application version beside PC VR / OpenXR in the launcher.

## Previous release: 1.0.1

Performance hotfix — 2 November 2026.

- Fixed repeated menu/lockpick checks and native font validation for every world-label candidate, including names the game does not display. Eligibility and font results are now reused within each update/render frame.
- Retains adjustable world-label sizes, nearby/distant label behavior and native name filtering.
- Confirmed in a Virtual Desktop headset test to resolve the reported stuttering regression after the square-VR updates.
- Existing settings and controller/hand calibration survive the update. The approved 1.0 fresh-install and Defaults preset is unchanged.

Close Gothic 3, the VR launcher and Union+ before running the 1.0.1 installer over the existing VR installation. A new game is not required. This installer also supports fresh installs with the documented native patches.

---

# Gothic 3 VR 1.0

First public release — 1 November 2026.

- Full stereoscopic VR with 6DoF head/hand tracking, room-scale movement and physical crouching.
- Motion-based one-handed, dual-wield, two-handed and staff combat, with independent swing/stab thresholds, weapon weight, stabilization, power strikes and native finishers.
- Physical bow drawing, two-hand crossbow support and controller-aimed magic; native skills, ammunition, stamina and progression remain in effect.
- Shield and weapon blocking, sneaking, sprint-until-stop, quick healing, controller-directed interaction and Quick Loot.
- Radial quick slots, Quicksave, Main menu and configurable controller bindings with double taps, modifiers and live VR input capture.
- Context-aware third-person views, optional Hands only mode, and adjustable grip-loss distance for supported weapons.
- Square VR scene resolutions with landscape menu layout, customizable HUD/world labels and the native Gothic reticle artwork.
- FXAA and SMAA Medium/High/Ultra, optional NIS/CAS spatial upscaling, oval fixed foveation with transition control, and optimized fire/magic effects.
- Native INI tweaks, adjustable atmospheric fog intensity and protected optional Union+ 1.7.3 integration.
- G3 VR startup splash, main-menu artwork/music, and skipped startup advertising logos while retaining story cinematics.

Steam, GOG and retail editions were tested with the supported native patch profile. Quest 3 / Touch remains the physical reference configuration. Other declared controller profiles are compatibility candidates, not hardware certification. See START-HERE.html for requirements and limitations.

The creator's saved settings from 1 November 2026 are the fresh-install and Defaults preset: 2560 × 2560 per eye, VDXR, FXAA, Performance VR effects, FFR mode 2 (60% diameter / 3% transition), 50% world labels and Toggle support grip. Existing user settings and calibration survive updates.

The release contains no active development capture/profiling modes or DX11 renderer experiments, and no DLAA/DLSS runtime. Native patches, Union+ and content packs remain separate downloads. The installer is unsigned.
