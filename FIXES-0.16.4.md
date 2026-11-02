# Gothic 3 VR 0.16.4 — visible bow charge, supported crossbow and human return

## Bow readiness

The evaluated native string/arrow draw now uses the effective charge fraction, multiplied by that bow model's actual maximum string travel. It no longer uses the raw controller separation, which reached the mesh's maximum draw early. The string extends only as far as both physical draw and learned-tier readiness allow. At full physical draw, its arrival at full extension is the visual full-power cue. Relaxing the draw retracts it. The tracked real hand stays immediate; the string cannot physically resist the controller. Damage, velocity, ammunition and charge progression remain as documented in FIXES-0.16.3.md.

## Crossbow handling

Right-hand-only aim includes visible sway (up to about 4.8 degrees yaw / 3.8 degrees pitch). This intentional VR handling penalty affects the actual weapon, aiming reticle and bolt direction together; it is not hidden spread and does not change damage, velocity or reload skills. Right trigger still fires one shot per press.

Bring the left palm to the foregrip and hold left grip. Supported aim follows the direction from the right hand toward the left hand, with roll from the left controller. Right wrist rotation no longer drives the supported aim. The same existing Aim stabilization filters this orientation in tracking space; locomotion is applied afterwards to avoid delayed world-origin jitter. Support eliminates the one-hand sway. The visible left palm remains attached to the foregrip. Release/lost tracking returns to one-hand handling. Native Crossbow II/III reload progression, ammunition and damage are unchanged.

## Magic palm contact

Cast meshes receive independent palm-contact correction rather than changing hand tracking or finger orientation. The Magic page adds Grip toward fingers / Grip palm depth / Grip across palm, default 4 / 2 / 0 cm, each bounded to -15..15 cm. Save settings applies live. Defaults move scrolls/runes away from the wrist into the grip; individual meshes/calibration still need a headset check. Rotation, closed fingers, idle/walk consistency, visibility and per-eye restoration remain active. Native casting/mana/scroll consumption and rune ownership are unchanged.

## Return from animal form

While transformed and outside menus/radial, hold **X + Y on the left controller for 0.5 seconds** to invoke the native SDK Entity::EndTransform. Buttons are consumed through release so no weapon/escape action leaks into the restored human state. Focus/tracking/session/identity guards cancel invalid chords. The verified installed Script.dll exports EndTransform at RVA 0x1B230; the existing exact module profile gate is required. Returning is available without opening a desktop menu; the native operation owns restoration rather than the mod modifying species/status bits.

The human eye height is saved before entering animal form, restored against the returned human root, and the existing yaw-only tracking recenter is requested after the form transition. Configured camera offsets are preserved and applied once. The human first-person mode is explicitly requested again, including when native flags already claim FP. If a save started in animal form and no prior human calibration exists, allow the native human camera 300 ms to update before calibrating; that fallback needs a live probe. Animal third-person is unchanged.

## Validation / probe

55/55 automated checks pass, including effective-charge string extension, deterministic sway, supported aim independent of right wrist rotation, locomotion translation, vertical aiming, cast contact correction without moving the hand, X+Y timing/cancellation, and human height restoration against a different player root. Existing native reload/skill, ownership/ammunition, cast, physical melee, exact reach, 2H, dual wield, room-scale, stereo GPU and ABI checks pass. All 12 launcher pages pass the built-in layout/capture check; Magic and Crossbow were visually inspected. The user INI is preserved byte-for-byte during installation. Native assets/modules are not changed or distributed.

The new behavior awaits headset validation:

1. Draw a bow quickly, then hold full physical draw: watch the string finish extending; compare short/full readiness and existing learned tiers.
2. Aim/fire the crossbow with just right hand, then left grip held. Move the left hand to guide it and rotate only the right wrist to confirm it no longer changes supported direction. Walk with support and fire repeated bolts using native reload.
3. Hold a scroll/rune standing, walking and after stopping. Check palm contact; tune only the Magic grip fields if needed.
4. Transform, move the animal, hold X+Y, and verify human first-person height and controller alignment. Repeat a transform and a return on a slope. Existing L3+R3 camera reset remains available.
