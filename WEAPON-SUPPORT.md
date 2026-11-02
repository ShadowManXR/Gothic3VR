# Flame Sword performance —0.22.12

Flame layer ownership is collected once from the held weapon tree. Other scene emitters use a query-free address lookup. Menu staff hiding and accepted hand/weapon mechanics remain. 78/78 tests pass; live performance acceptance pending. See FIXES-0.22.12.md.

## Previous release notes

# Held weapon visuals —0.22.11

Flame Sword uses native blade-local fire without the two world-space ghost layers. Held Staff/Halberd is hidden only in first-person menus/radial and restored after each stereo world render. 77/77 tests pass; headset acceptance pending. See FIXES-0.22.11.md.

## Previous release notes

# Weapon support — 0.16.1

0.16.1 fixes the crossbow alias/duration lookup and casting-item visibility, adds transformed-player third-person, doubles one-hand 2H angular response and removes the melee cap that clipped native progression bonuses. Full controller reach was accepted in 0.16.0. See FIXES-0.16.1.md and PROGRESSION-AUDIT.md.


The user confirmed the 0.14.0 headset probe: both dual wield 1H weapons remain visible and physical combat works with both hands. This records the tested setup; it does not validate every weapon template or physical parrying.


The user confirmed both Staff/Halberd palm contacts in the 0.13.6 headset probe. Right-palm orientation and upward direction were accepted earlier. This confirms visual grip contact; the feedback does not separately validate both-end damage, every staff/halberd template, or a particular contact setting value.



Implementation uses local Gothic SDK use types and native equipped entities. Staff (12) and Halberd (51), including Blade Staff, use a separate weapon-only frame anchored to the unchanged right tracked palm, with no identity overrides. Equipment, skill checks and damage types remain native.



| Family | Current VR state | Next validation/work |

| --- | --- | --- |

| 1H swords, daggers, axes, maces | Generic physical 1H pose/contact/damage; short-weapon bounds added | One-handed sword accepted 0.10.2; other meshes await individual tests |

| Dual wield 1H | Independent controller poses, pre-cull spatial transforms, per-hand swept contacts/strokes and native-result damage scaling | Both weapons visible and independent physical combat accepted in the 0.14.0 headset probe; other templates await individual tests |

| 2H swords, heavy axes, hammers | 0.16.0 unsupported one-hand inertia, reduced stroke damage and hit interval; attached shared orientation with world-pose continuity and physical swing detection; accepted attached support palm; native 2H/Axe types | Tested 2H sword/axe: handle attachment accepted 0.12.1, rotation/position accepted 0.12.2, walking continuity accepted 0.12.3; other templates need individual probes |

| Staffs and halberds | 0.13.6 independent weapon orientation, unchanged right hand, mirrored palm depth and corrected support contact, attached left palm, coherent steering and both-end contacts | Right-palm orientation and upward direction accepted in 0.13.4; Both corrected palm contacts accepted in 0.13.6; direction and right-palm orientation accepted earlier; separate both-end combat verification remains pending |

| Bows | String draw, left-side arrow rest, stabilized aim, native shooting/ammo | User accepted arrow placement and shooting in 0.11.7; fine contact remains configurable |

| Crossbows | Right-hand tracked palm, stabilized reticle, trigger fire, optional left foregrip support, native Cock duration and active-perk reload speed | Original pose/fire/support accepted 0.15.0; new reload timing and runtime motion lookup await headset verification |

| Magic / casting staffs | Right-controller stabilized focus and native owned-spell projectile launch pose; native spell/scroll cast input, cost, skills and effects; tracked left casting staff | 50-test batch and native binary checks pass; projectile, charged, targeted, self spells, scrolls and staff casting need headset tests |

| Shields | Native attachment can follow tracked off-hand | Physical blocking and independent grip calibration not integrated |

| Special arrows / bolts | Existing bow pipeline delegates projectile effects to native Shoot | Special ammo selection/effects need explicit tests; native Bolt pipeline integrated, headset verification pending |



Next: validate the new magic, native reload, heavy 2H and full-reach batch; verify staff both-end damage; implement/test shields; expose ammo selection and validate native special projectile effects. Actual game item use type and nonzero Damage property determine eligibility for physical melee. A staff without native melee damage is not given invented damage. Mesh contact currently uses conservative bounds, so individual weapon shapes still need testing.











