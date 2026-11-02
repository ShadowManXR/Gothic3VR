# 0.23.77 - Bow string-hand contact and double-trigger quick heal

- During an active bow draw, the rendered right palm follows the evaluated
  string/arrow nock. This uses the current native model geometry, including
  visible skill-dependent charge speed, before the hero palette is frozen for
  both eyes. The calibrated hand orientation remains responsive; the wrist
  target keeps the palm on the nock rather than letting it drift with the raw
  controller position.
- The constraint is visual only. Raw controller positions remain available to
  physical draw, charge, aim influence and release logic. Releasing the right
  trigger, canceling the draw or losing tracking/geometry restores the free
  hand. Crossbow support remains governed by its separate foregrip attachment.
- **Double-tap the left primary/index trigger** for native quick heal. The
  support grip/squeeze is independent and can stay held on a 2H weapon. A
  single press or hold does not heal; each potion needs a new tap pair.
  The pair window is 450 ms with a minimum 80 ms between presses, a stable
  30 ms release and analog press/release hysteresis. Menus, radial, recenter,
  focus loss and stale samples cancel pending gestures.

Native potion selection, healing eligibility/animation and inventory effects
remain in charge. L3 Sprint, double-Y Sneak, all weapon progression, saved
calibration, the accepted crossbow contact and landscape FFR are retained.
Launcher instructions use the new quick-heal binding in English.

Tests cover moving string geometry versus a separated/rotated raw controller,
both frozen eye poses, cancellation and no feedback into the physical sample;
input checks cover held/bounced/expired/paired triggers, preserved support grip,
Sprint, recenter and menu/focus isolation. Headset validation is still required
for visible bow-hand contact and the new quick-heal gesture.
