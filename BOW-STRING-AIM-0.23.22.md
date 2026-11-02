# Bounded string-hand bow guidance — 0.23.22

The left hand remains the primary bow direction. During a valid physical draw,
the right string-hand position adds a damped angular correction towards the
string-to-bow line. Influence defaults to20%, adjustable0..40% in the launcher
Bow page (String-hand aim influence), with a hard6degree combined angular limit.
Zero restores left-hand-only aiming. No steering near a zero-length grab line,
from an untracked string hand, or before an actual draw. Crossbows keep their
existing separate guidance.

The correction is smoothed in the bow's relative frame, leaving locomotion and
native body turns immediate. Bow grip position is fixed. Weapon/session changes,
tracking loss and time discontinuities reset the correction. Rendered bow/arrow,
reticle, native target and released projectile use the same evaluated geometry.
The physical draw basis is retained separately, so the small aim correction
cannot create draw distance or change the native tier-dependent power/timing.
Release continues using the last held direction to avoid trigger-release twitch.

This build includes the already validated0.23.21opt-in temporal input work.
Game DLAA stays disabled and no diagnostic capture request is armed by this change.
Headset feel and default influence require the user's next physical bow trial.
