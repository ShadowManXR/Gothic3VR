# 0.22.19 — controller remapping

- Press R3 once to open the radial; choose a sector with the right joystick; A confirms, B cancels, Y changes page. Left joystick no longer selects radial sectors.
- After closing the radial, the right joystick must return to centre before camera turning resumes. Selection remains captured during the radial, including graphics stalls and the confirmation frame.
- Native secondary action (Use2, shield block / weapon parry) moves from left trigger to right grip. Use the same native game binding and scripts. Physical melee's parry suppression follows this new input. A pending finishing-kill chord reserves right grip + trigger rather than activating block.
- Menu mouse clicks, right trigger attacks/bow/crossbow/casting, left grip support, L3 sprint, double-tap Y sneak and L3+R3 reset retain their existing behavior.
- Launcher control hints are updated in English. Settings and native game files are preserved.

The user reports no further movement freeze with 0.22.18. This release retains its combat movement recovery and third-person knockdown/knockout behavior.

Verification: extend the existing controller and radial regression tests for the new mappings, held-grip menu transitions, right-controller availability, independent left/right joystick axes, retained selection and neutral-before-turn behavior. Run the full suite and launcher layout checks before installation. Headset input validation remains pending.
