# Current behavior — 0.16.4

Magic grip offsets default to 4 / 2 / 0 cm and apply live without moving the tracked hand. In animal form hold X+Y for 0.5 s to return human; the saved human eye height and tracking centre are restored. See FIXES-0.16.4.md.

## Previous release history

# Controller magic — 0.16.3

Casting items now anchor their visible mesh centre in the calibrated palm with a consistent closed finger grip, independent of idle/walking carry offsets. Native transform/item/finger state is restored after each eye. Animal transitions clear the verified persistent/requested/temporary FP latches before selecting native third-person, then restore the configured human first-person policy on exit. 54/54 automated checks pass; these corrections await headset testing. See FIXES-0.16.3.md.

Select a learned spell or scroll through the native spellbook/quick slots and radial. Aim with the right controller and use its trigger for the game's normal cast input. Hold/release for spells whose native cast supports charging. Native quick, charged, targeted and self-spell behavior stays in the game; this batch introduces no gesture recognition or new spell grants.

The Magic page shares Aim stabilization and the aiming-reticle toggle with Bow/Crossbow. Save settings applies live. The right aim quaternion is filtered in tracking space; locomotion shifts the ray immediately. The native focus system keeps its spell target/range filters. Magic does not receive the close-object pickup reach limit or interaction-ray elevation bias.

The actual right-held Cast item's Item.Spell identifies the equipped spell, matching the verified native PS_Magic_Cast pipeline. Native spell/skill/item rules decide which spell can be equipped and whether a staff grants or allows it. A native left-held casting staff is attached to the tracked left palm using the accepted staff contact settings. The mod does not teach staff perks, grant spells or turn every melee staff into a caster. Select the native staff/spell setup normally; do not treat simply swinging a staff as casting.

For a fresh matching player-owned native spell projectile, the verified existing Projectile::Shoot import supplies the stabilized right-hand launch pose and direction. The original native callback still launches it. NPC projectiles, bolts/arrows, nonmatching spells and stale samples pass through. Native mana, scroll consumption, spell requirements, charge/reload, damage/effects, velocity, seeking path and target remain unchanged. Self/nonprojectile spells are left to their native cast behavior.

Previous 0.16.1 validation (superseded by the grip/camera corrections above): native right-held template/GetSpell imports, both native magic projectile calls through the shared Shoot import, native owner/SetSpell setup, filter reset, stale/focus/weapon/session guards and locomotion continuity. All 51 automated tests and 12 launcher layout checks pass. Magic page was visually inspected. The user reported basic casting working in 0.16.0, but held scrolls/runes/teleport items disappearing. 0.16.1 now preserves the native item-to-wrist attachment, moves it before frustum culling, temporarily enables rendering per eye and restores position/visibility. Native consumption and held-item replacement still belong to the game. This correction awaits headset testing. Transformation detection uses native IsTransformed and player species; transformed players keep native third-person stereo, humanoid arm/room-walking overrides are suspended, and first-person is restored automatically in human form.

Logs: vr_magic_selected, vr_magic_projectile_controller_aim and shared ranged/fault logs. See BATCH-0.16.0.md for the return-time probe. Projectile origin, aiming alignment, charged/target/self spell input and native staff setups are runtime validation items.
