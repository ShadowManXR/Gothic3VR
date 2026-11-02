# Gothic 3 VR 0.22.3 — revised controls, armed Sneak, single-step lockpick and 800 cm/s

## Controls

| Action | Controller |
|---|---|
| Sneak on/off | Press Y once; press again to exit |
| Native Sprint | Hold left grip while moving with left joystick |
| Main menu / Back | Left controller Menu button |
| Open radial | Press R3 |
| Select radial sector | Left joystick |
| Confirm / cancel | A / B |
| Radial page | Y while the radial is open |
| Camera reset | Hold L3 + R3 together for 0.5 s |

Sneak can be toggled while stationary or holding a melee weapon; it no longer needs a grip/movement combination. Releasing Y does not leave Sneak. A held button cannot toggle repeatedly or activate on startup. While Sneak is enabled it overrides Sprint. Menus/radial suspend the native Down action while preserving selected Sneak intent; Y used for radial pages cannot toggle Sneak upon return until released. World/actor/focus loss and native activity transitions safely clear input. The learned Sneak skill remains required; left grip support for 2H/crossbow is no longer a Sneak restriction.

Left grip + joystick uses existing native Sprint input, speed, learned conditions and stamina. Standing still releases Sprint. Right stick click is dedicated to opening the radial; ordinary locomotion remains native. The Touch action set binds /user/hand/left/input/menu/click for Menu. The right system button is not bound. Movies use the same Menu/Back button to skip.

## Armed Sneak and Murder

The previous live log shows logical Down14 was accepted but armed Sneak lasted only one frame, reverting to PS_Melee_Loop/AniState4. Native analysis explains why: PS_Melee_Sneak runs _AI_Parade. Its name does not mean sustained stealth with a weapon. The native IsSneak helper checks AniState3; a movement mode or a state name alone is not sufficient.

The plugin now chains through native OnAction/OnPlayerGameKeyPressed/OnPlayerSneak and replaces the registered PS_Melee_Sneak callback only for the current player with a fresh owned VR Sneak request. It invokes the native Normal_Sneak transition, preserves the armed melee loop and sets actual native Sneak stance3 instead of Parade4. It changes movement mode only upon a stance transition, matching native Sneak walk translation; no repeated forced stop or direct velocity motor is added. The original callback remains for other entities, keyboard-only/native use and unavailable VR context. Exact Script_Game hash and SDK callback addresses/ABI are verified before registration.

The owned stance is applied only in the idle native melee loop. Attack/parry/recovery/finishing/interaction states are not rewritten. On exit it restores the normal stance when still owned; changed actor or incompatible native states are left alone. The eye icon/haptic follows actual native stance3, not a misleading *_Sneak state name.

Murder/Pierce qualification now recognizes this actual native Sneak stance as well as native movement Sneak. Use the learned Murder perk, Sneak, and the existing charged coherent stab with a qualifying 1H blade behind an unaware target. Existing power charge, physical thrust qualification, native stamina, victim awareness/FOV/protection and witness/quest/death handling remain. No direct kill or perk grant is added. Armed Sneak/Murder still requires the user's live test.

## Lockpick: one neutral-separated guess

Only lockpick input uses the stricter latch. Native Lockpick routine/task/state or a locked target in PS_Interact_Chest activates it; this detected the live chest interaction which previously reported lock=0. It does not turn a non-GUI chest interaction into a GUI panel. Native lock status, sequence, skill checks, lockpick loss and rewards are untouched.

After entering a puzzle, rest both joystick axes within radius 0.25 for 100 ms. A deliberate left/right deflection beyond 0.65 emits one A/D press/release. This consumes the latch. Holding the joystick, changing direction directly, jitter around a threshold or briefly passing through centre cannot emit another guess. Rest at centre again for 100 ms before the next direction, including repeated guesses on the same side. Examples: left — centre stop — right — centre stop — left; right — centre stop — right — centre stop — left — centre stop — left.

Each accepted guess uses the existing bounded 80 ms A/D pulse, with no OS auto-repeat injection. Focus/tracking/context loss releases the key and requires neutral before rearming. Generic menus retain their previous directional behavior. The previous live log contains duplicated A/D injections 37–63 ms apart; the new test replays long holds, sign changes, micro-centre samples, off-axis motion and LRLR/RRLL sequences and counts actual key-down events. No duplicate occurs until the required centre stop.

## Velocity settings

Minimum swing speed now ranges 50–800 cm/s; Minimum stab speed ranges 40–800 cm/s. Both sliders retain 10 cm/s steps in 1H Melee Weapon, 2H Melee and Staff / Halberd. Each profile's existing values and separate distance settings are retained; Save settings applies live. The upper limit is extended in the launcher, INI loader and runtime speed gates. Tests at an 800 cm/s threshold reject 7 m/s movement and accept 9 m/s for both swing and stab.

## Validation and installation

70/70 automated tests pass, including updated native controller actions, real Windows INI round-trip/bounds, radial remapping, toggle ownership/context/skill handling, one A/D event per lockpick gesture, 800 cm/s enforcement and existing native binary/ABI/GPU/stereo/restoration checks. All 15 launcher pages pass layout validation; updated combat/Motion/Radial pages are visually inspected. Native callback/perception evidence and prior-session input events are retained.

Finalized bow/crossbow and tracked arm/grip behavior remain byte-identical in their headers. Native game DLLs and game assets are unchanged. Installation backs up binaries and preserves current user INI bytes exactly. The supplied INI is a new-install default, not a replacement for user settings. The game is not launched unattended. Live armed Sneak/Murder, lockpick sequence acceptance and controller Menu behavior still need the user's test.
