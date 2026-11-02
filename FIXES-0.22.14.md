# 0.22.14 — knockout camera, native movement guards and hero speech

The reported symptoms were moving feet without player translation during a large
fight, and the hero's voice cutting/panning between ears. The user then clarified
that the movement problem may follow a nonlethal knockout (health exhausted).
The exact movement incident has not been reproduced under a debugger.

- Ground Run/Sprint preference no longer replaces native jump, fall, swimming,
  sliding, ragdoll or unknown movement modes. Idle joystick input leaves native
  stopping alone. Native scripted combat movement, recovery, mounted movement and
  activities retain authority. No movement flags, velocity or collision state are
  forcibly reset; no automatic unstuck teleport is introduced.
- Native LieKnockOut / LiePiercedKO and real death use third-person. Ordinary
  SitKnockDown / LieKnockDown remain first-person. A knockout exception persists
  through native recovery, then returns through the existing calibrated
  first-person reset. Stereo stays active and the head is visible in third-person.
  Save/world replacement clears the knockout camera ownership.
- Only the current hero SPU speech channel becomes centered 2D audio while VR is
  active. NPC voices, music, effects, volume and sound assets are untouched.
  Native playback/update functions still execute. Original mode/pan are restored
  on a live owned channel before replacement or VR/world shutdown. Old raw
  handles are never dereferenced without a current native channel owner.
- The legacy FMOD exports are resolved from the already loaded fmodex.dll,
  including PUP's forwarding wrapper; no separate audio system is created.
  Native UpdateChannel normally places speech on the actor root. This is a
  near-field self-voice correction, not a confirmed per-eye listener bug.
- Two bounded native movement records per second expose current/wished mode,
  script movement permission, combat/contact/recovery flags, wished direction,
  sampled translation, and sensor/goal offsets if the freeze recurs.

Validation: 82/82 CTests passed, including exact native Game.dll hash/prologue
checks, both x86 speech hook ABIs, FMOD adapter/state rollback fixtures, special
movement mode guards, and knockout/normal-fall/recovery/save transitions. The
100,000 steady-state voice fixture updates make no FMOD calls. These are offline
checks; headset audio, knockout recovery and large-fight movement still need a
live test. No game FPS improvement or confirmed freeze resolution is claimed.

FMOD mode semantics: https://www.fmod.com/docs/2.04/api/core-api-common.html#fmod_mode
