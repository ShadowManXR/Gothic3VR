# 0.23.49 - stowed weapon/shield visibility in first-person

User reports that sheathing while running hides the body but leaves the weapon
and shield on the back passing in front of the headset camera.

The existing head/body and menu-staff scopes did not cover stowed inventory
entities. Add a separate bounded visibility snapshot for the actual native
BackLeft, BackRight, Bow and Crossbow slots (5,6,7,8). Capture weapon/shield/staff
entities and their children once per outer stereo render, after attachment
preparation. Hide after per-eye arm/bow preparation for BOTH eyes. Restore exact
original parent/child visibility in the outer finally block, or abandon the
snapshot if world teardown has invalidated it. No references survive the frame.

Native Game.dll decompilation confirms GetStackFromSlot selects the current
linked slot (not a quick-slot assignment). GetBone maps5 to ShieldBack or
MeleeBackLeft,6 to AxeBack/StaffBack/MeleeBackRight and7 to Bow/CrossBow.
The code also accepts SDK slot8 for crossbow compatibility. Explicitly exclude
current right/left hand entities even if equipment references overlap during
weapon draw/sheath. Restrict to equipment weapon types; do not hide ammo,
scrolls, armor, NPCs or world items. No scene or full-inventory scan.

The scope applies to VR first-person at rest, running and in first-person menus.
Dialogue, activity, animal, knockout and other third-person views retain native
equipment visibility. Drawn weapons stay controlled by the existing arm/bow
paths. Do not change native animation, movement, opacity or equipment ownership.

Tests exercise the real generic collection/visibility scope with native-style
recursive setters: both eyes, exact mixed descendant flags, draw/sheath overlap,
fresh frame after sheathing, equipment categories, third-person exclusion and
invalid collection rollback. Existing head visibility, arm/weapon/crossbow and
native movement/camera checks are retained. Headset acceptance remains pending.

No launcher/settings/calibration or graphics changes. DLAA remains off.
