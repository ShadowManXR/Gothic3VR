# 0.23.79 - Crossbow support through the closed palm

The left crossbow hand remained latched, but the contact used the open-hand
tracking pivot. With the native fingers closed, this looked like a wrist grip.

The crossbow frame support now resolves the accepted 2H closed-fist contact:
the native knuckle region plus the palm-pad depth. Express the pad normal in
the calibrated hand frame before calculating the wrist target. The global
controller/palm calibration and the foregrip point on the weapon do not move.

Keep the captured relative orientation, left-hand aim guidance, grip/release
behaviour and one frozen support contact for both eyes. Native reload/skills,
bolt geometry, right-hand placement, bow fingertip contact, quick-heal double
trigger, FFR and all saved calibration remain unchanged.

The support regression checks the closed-palm surface against the foregrip
independently of the tracking pivot, verifies the wrist stays away from that
point, and exercises 200 rotating/translating weapon frames with a separated
controller, both eyes and multiple palette-cache passes. Headset acceptance
is still required. Only the plugin changes; launcher remains 0.23.77.
