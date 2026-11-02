# 0.23.83 — summoned Flaming Sword render binding

The retained 0.23.82 log confirms native Spl_Flamesword creates and holds
It_1H_MagicFlameSword_01 as a right-hand type2 melee item. The native script's
AssureItems/HoldStacks path is preserved: no replacement sword is spawned and
spell mana, skills, inventory lifetime and damage are unchanged.

The summoned item now has its own bounded, reversible visibility/alpha scope
before both stereo frusta, independently of whether the new item has already
joined the hero's child tree. Its existing mesh and low-cost flame layers follow
the accepted tracked 1H pose. Each eye restores the original root/child flags and
opacity, including unequal child visibility. Stowed items and other weapons are
not revealed by this policy. No particle optimization or flame suppression rule
from the accepted 0.23.82 build is reverted.

A newly created inventory entity can precede its first native attachment update.
Previously LockedMeleeAttachment could retain that initial world-origin offset
for the item's entire held lifetime. Only the summoned sword now defers initial
capture if its pivot is over75cm from the native wrist. It retries the native
pose on subsequent frames; after capture the established grip remains locked
during native animation. This race is covered by regression fixtures; the old
capture lacks per-item offset telemetry, so it is not claimed as a confirmed
live cause. Once-per-item wait/ready logs include offset, visibility and alpha.

Checks cover new-item late attachment, coordinate scale, rigid grip retention,
per-eye flag/alpha restoration, independent item trees and ordinary melee/ranged
regressions. No engine, renderer, launcher, settings, templates or mod archives
are changed. Headset validation remains pending.
