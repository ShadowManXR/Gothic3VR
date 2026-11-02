# Palm-based calibration —0.22.10

The controller grip position is the palm pivot. Start position offsets at0cm; adjust angular offsets around that palm. Existing saved settings are preserved; Save settings applies live. 75/75 tests pass; headset acceptance pending. See FIXES-0.22.10.md.

## Previous release notes

# Progressive arm rotation — 0.22.8

Small rolls stay at the wrist; larger turns recruit the forearm and then the upper arm through soft limits. Existing calibration and full reach remain. 73/73 tests pass; headset acceptance pending. See FIXES-0.22.8.md.

## Previous release notes

# Hand calibration and sword grip — 0.10.2

The user confirmed recovery and hand calibration in 0.10.1. In 0.10.2 the **Sword** tab adds weapon-only pitch/yaw/roll adjustment. The actual mesh axis is aligned forward automatically in the calibrated grip frame, then the additional angles are applied. Both the sword hand and weapon rotate together; the wrist target is recomputed around the same grip centre. Zero means this forward grip rather than the original upright sword posture. Positive pitch raises the blade; negative pitch lowers it. Saved device positions/angles remain active; if the saved hand pitch is +20 degrees and you want a flatter sword, try Sword pitch -20 degrees. Unarmed hands, bows and the left hand retain their device calibration. Settings are reloaded live, and a changed calibration cancels the current stroke.

The launcher has separate **Left hand** and **Right hand** tabs. Save settings applies changes within about one second while playing. Each hand has outward/inward, up/down and forward/back offsets (-20 to +20 cm), plus pitch/yaw/roll (-90 to +90 degrees). Positive outward moves the left grip left and the right grip right in the controller's local frame. Offsets follow controller rotation; they calibrate the device grip rather than shifting the camera or tracking origin. Zero returns to the previous uncalibrated pose. Existing configurations default to 5 cm outward on each hand; other new axes default to zero.

Start with controllers pointing ahead and hands a comfortable distance apart. Adjust outward until the virtual and real hands match, then adjust height and forward position. Change one orientation axis at a time only if necessary. In 0.16.0, the rendered upper/lower arm segments extend or fold to place the wrist at the calibrated controller target in every equipment state, including empty hands and magic. Native bone length no longer clamps that target. Shoulders remain on the character and elbows are inferred; this is not a measurement of the player's anatomical segment lengths. Bone-axis scaling preserves thickness and is restored after each eye. See FULL-CONTROLLER-REACH.md. New headset reach validation is pending. There is no hand-to-hand collision solver.

Keys are in [Stereo]: LeftHandOutwardCm, LeftHandUpCm, LeftHandForwardCm, LeftHandPitchDeg, LeftHandYawDeg, LeftHandRollDeg, and the corresponding RightHand keys. The normal INI save preserves unrelated settings. Bones, equipped weapons and physical melee traces use the same calibrated grip. Saving a changed calibration cancels the current stroke, so a settings change cannot count as a swing. Selection-ray elevation remains independently adjustable in Interaction; menu pointer stabilization remains unchanged.

During native knockdown/get-up states the render overrides and physical sword combat are suspended. Trigger inputs pass to the native game for recovery. No combat/animation state is forcefully reset.

Automated validation: mirrored offsets, rotated-device offsets, world scaling, orientation without duplicate translation, zero offsets, independent signed settings round-trips and bounds. All five launcher pages passed layout checks; both hand pages were visually reviewed. Headset alignment and recovery acceptance are pending the 0.10.1 trial.
