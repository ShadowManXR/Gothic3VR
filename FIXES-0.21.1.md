# Gothic 3 VR 0.21.1 — rigid melee grip

The user reports large swings rotate swords/axes in the palms, also affecting 1H and staff. Native item-to-hand attachment was being recalculated from animation every frame. Unsupported 2H weight rotated the mesh alone. Supported staff followed the coordinated weapon frame while the right palm kept its independent controller rotation.

Drawn melee now captures one validated native attachment per weapon, hand and rig, after the native skinning update. 1H and 2H use that fixed attachment through every swing. Staff/halberd additionally lock their accepted calibrated mesh-to-palm profile, retaining the existing upright orientation and grip settings. Native idle/draw/carry pose changes cannot recalibrate a held grip. Drawing a different weapon, holstering, changing the rig/world or applying hand/weapon calibration resets the appropriate binding. Dual wield retains independent bindings.

Unsupported 2H inertia now transforms the whole visible hand/weapon assembly about its palm contact. One-hand heaviness and existing combat restrictions remain; the weapon cannot independently turn inside the hand. Two-hand support continues using the common frame. Staff resolves its right-hand grip from that same supported mesh, keeping both palms fixed to their captured contacts/orientations. Releasing the left grip returns to the independent tracked hand.

All held melee prepares before both eye frusta and uses the same frozen palette and mesh poses, including 1H and dual wield. Native matrices/rendering flags are still borrowed only during render and restored. No inventory changes or damage/stamina/skill policy changes were added. The five finalized ranged headers and the native combat rule/gesture headers are byte-identical to0.21.0.

65/65 tests pass. The new test checks both palms for1H/2H/axe/staff/halberd across240 wide sweeps with native attachment animation and world translation, weighted2H palm contact and skin/mesh equality, coordinated staff with opposing controller rotation, secondary palm attachment, dual-wield independence and reset/invalid matrices. These establish transform invariants, not headset acceptance. Fifteen launcher pages are checked; there are no new controls.

The launcher source, headers and target options are unchanged. Installation reuses the verified0.21.0 launcher binary, allowing its open window to remain and retaining current settings. The plugin is0.21.1. Installer backs up binaries, preserves the current user INI and does not launch Gothic3.

Test a1H weapon, sword/axe2H with one and two hands, and Blade Staff/halberd with one and two hands. Make wide horizontal/vertical sweeps and roll both wrists together or oppositely, then walk/turn while holding. Watch the handle against the visible palm rather than the world: its relative contact and orientation should stay fixed. Check release/re-grab, dual wield, live calibration, sheath/re-equip and both eyes. Exact real-game behavior needs user confirmation.

Shield/native parry, movies, native skills and0.20.2 FINISH/ranged recovery remain included; see BATCH-0.21.0.md for those pending probes.
