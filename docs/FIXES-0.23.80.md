# 0.23.80 - Effective one-hand weight for staff/halberd

The previous handling filter bounded its angular gap to the controller. Fast
gestures could repeatedly force it to catch up, making a long unsupported
halberd feel almost weightless even with a high weight setting.

For the existing Staff/Halberd **one-hand** profile only, a nonzero
`One-hand weight` now adds an angular speed limit to the damped response. Higher
weight lowers that limit: approximately 554 degrees/s at 25%, 310 degrees/s at
110%, and 212 degrees/s at 200%. A fast swing can leave the visual hand/shaft
orientation trailing until it catches up. There is no spring overshoot.
The existing position-lag bound remains in place.

`One-hand stabilization` still damps small motion. Weight 0 disables the new
speed limit; weight and stabilization both 0 retain exact unfiltered tracking.
The existing Staff/Halberd grouping and all saved values are retained. The
current user values (110% one-hand weight, 50 ms stabilization at inspection)
are not rewritten.

The hand and locked weapon receive the same rigid transform, preserving grip
alignment. Grabbing with the second hand selects the original supported
profile: its response and two-hand settings are unchanged. Other weapon
profiles, contacts, native damage/skill/stamina rules, locomotion, bow/crossbow,
quick heal and rendering configuration are untouched.

Regression checks include capped fast swings at three weights, monotonically
stronger inertia, no movement of the grip point, exact unchanged two-hand
response, duplicate-timestamp support transitions and existing rigid assembly
tests. Headset validation remains pending. Only the plugin changes; launcher
remains 0.23.77.
