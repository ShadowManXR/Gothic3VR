# Reach and rotation — 0.22.8

Existing full-range positional IK is followed by visual twist redistribution. Solved joints and final tracked hands stay fixed; no extra input smoothing. 73/73 tests pass; headset acceptance pending. See FIXES-0.22.8.md.

## Previous release notes

# Full controller reach — 0.16.0

Both wrists target the calibrated controller position without being clamped to the native arm length. The tracked solver now applies in all equipment states, including empty hands and casting. Beyond normal reach, upper and lower rendered segments extend proportionally; near the shoulder they fold toward the target, including a coincident target. Controller wrist orientation remains immediate. Existing hand offsets and weapon palm contact settings remain active.

This is a temporary rendering override. Only each segment's bone axis is stretched, preserving arm thickness; the original palette is restored after each eye. Native skeleton assets, saves, NPC rigs and physical gameplay bones are unchanged. Shoulders remain at the character's anatomical origin and elbow position uses an inferred pole. Controller tracking does not measure the user's elbow or actual upper/lower anatomical lengths, so this does not claim full-body anatomical calibration or hand collision.

Local tests cover dense targets outside native reach, folded and coincident targets, exact wrist endpoints, orientation, segment/thickness transforms, native skeleton fixtures and restoration. The 50-test batch passes. The user accepted free hand movement in the 0.16.0 headset test. Additional equipment regressions to check: compare fully extended palms in empty hands, 1H/dual wield, casting, bow, 2H and staff, including after movement and recentering. An attached support hand intentionally stays on the weapon contact while grip is held.
