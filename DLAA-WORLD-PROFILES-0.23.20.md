# World-color temporal inputs — 0.23.20 candidate

Offline analysis of the saved game captures and the native Shader.Cache found
seven additional world-color vertex programs missing from the four-profile
collector. Their hashes occur in 20 earlier real game observations; all 20
declared native projection matrices match the recorded SDK camera matrices.

The collector recognizes only these exact additional hashes, reads their actual
registers, and normalizes the rigid/17-instance position transforms for the
existing GPU companions. Some native instance transforms live at c119..169 and
projection at c187..190, beyond the old c0..82 read. World/object/buffer identity
and material/pass eligibility remain mandatory. Unknown shaders stay invalid.

The bounded shader survey also retains full-resolution color passes that use
prefilled depth with ZWRITE=0. It records native declaration, draw, viewport,
scissor, render states, VS/PS programs and constants, and copied SDK identities.
Late passes can be surveyed after the opaque boundary without altering frozen
motion jobs/history. Reset recreates survey storage even when job capacity was
retained. This path remains limited to explicitly requested diagnostic captures.

GPU validation replays the saved original game programs for actual depth and
checks the normalized companions against it, including instance rows 0,3,16,
native history and unchanged render state/color. These are controlled geometry
fixtures using real native programs, not a new live Ardea capture.

The last real game capture (PID2768, frames948..951) belongs to installed0.23.19:
GPU transport succeeds for both eyes, but native motion accepted0 draws and skin
covers about6% of the image. It must not be reclassified as complete by these
offline tests. Candidate0.23.20 has not been installed or tested in a new game run.

Actual game DLAA remains disabled. Full scene/sky/reactive coverage, projection
jitter, linear color, NGX worker wiring and final eye output remain outstanding.
The working standalone NGX demo and all game/launcher/INI settings are unchanged.
