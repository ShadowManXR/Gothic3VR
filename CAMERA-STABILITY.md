# Native locomotion restored — 0.20.1

The user rejected direct movement. The runtime no longer generates ground velocity or redirects native rigid-body velocity. Native goals, animation, acceleration, braking, speed and floor/collision handling own joystick movement. The accepted room-scale transfer and head-direction input bridge remain. No new smoothing filter was added. See FIXES-0.20.1.md.

## Previous release notes

# Current movement and activity camera — 0.20.0

Continuous joystick direction uses the complete rendered VR camera basis, correcting folded Euler yaw reversals. Native wished direction and ground motor share one heading; neutral ownership avoids animation handoff during render delays. No new camera smoothing filter is added. Native floor/collision handling remains. Active object activities use native third-person and recenter on return. Headset comfort remains unverified. See BATCH-0.20.0.md.

## Previous release notes

# Accepted ground correction and resolved speed — 0.19.1

The user confirmed the 0.19.0 floor fix. PutToGround and direct movement remain; sprint no longer multiplies mode/speed/terrain values already included by native GetCurrentMaxSpeedOfDir. The reported 1350 units/s restores the native roughly 600 units/s result. No new camera filter or floor offset was added. See FIXES-0.19.1.md.

## Previous release notes

# Ground correction — 0.19.0

Direct VR ground movement now includes the original PutToGround(false, false) helper before generating horizontal translation. This restores native collider/pivot/sensor floor corrections omitted by the previous direct motor. Native terrain/collision/air/knockdown paths remain authoritative, and camera position filtering remains bypassed in direct mode. Verify flat ground, slopes, steps and physical/stick movement in the headset; automated validation cannot establish comfort or terrain height by itself. See BATCH-0.19.0.md.

## Earlier release notes

# Direct locomotion — 0.18.0

Motion → Direct VR locomotion replaces the human player's normal ground translation generator before native floor/physics processing. It uses native speed/terrain modifiers, stops its contribution on release and accepts new input immediately. Movement start response controls starting acceleration only (0–150 ms, default 60 ms). Camera position smoothing is bypassed entirely in this mode; stable horizon, anchored eye height and physical XR tracking remain. The legacy camera filter described below is available only with direct locomotion disabled. See FIXES-0.18.0.md for the hook scope, retained native paths, diagnostic stop trace and pending headset probe.

## Legacy camera filtering

# Native camera stabilization — 0.9.9

The camera base already used the player root and a calibrated eye height instead of the animated head bone. It nevertheless followed every native root correction directly. Version 0.9.9 stabilizes this origin at stereo render time, before adding the unfiltered OpenXR eye and hand poses.

Enable **Stable horizon** and **Smooth game movement — stabilize camera jumps** on the launcher's Camera and controls page. Both default to enabled for existing installations. Save settings to compare with smoothing disabled during play. Existing camera offsets, turn speed, resolution and panel settings are preserved.

The INI section `[Stereo]` also accepts `SmoothCamera=1`, `CameraSmoothingMs=40` and `CameraVerticalSmoothingMs=100`. The time constants are bounded to 0–250 ms; zero bypasses that axis. These are native movement time constants, not a delay applied to physical head or hand tracking. Exponential integration uses elapsed render time. Constant joystick movement can trail the native collider slightly; the default horizontal constant is deliberately short, while terrain height gets stronger damping. Large world teleports, a changed actor or scale, gaps longer than 250 ms, recentering and leaving first-person reset the filter. A correction is bounded to 50 cm horizontally and 30 cm vertically.

An accepted physical X/Z transfer immediately translates both the filter's previous root and its output by exactly the consumed distance. It must not be smoothed: doing so would add an artificial reverse head movement when the shared XR origin consumes that step. Native terrain Y is smoothed separately; physical crouching and leaning remain optical and immediate. The 0.9.8 native sensor, fixed-step and movement-history corrections remain in place.

A frame captures one camera base for both eyes, tracked grips and in-render controller focus. Between renders, focus queries reuse the current correction without advancing the filter. With Stable body enabled, the torso/legs receive the same render-only translation, while the actor and collider retain native physics positions. Existing palette and weapon restoration remains active.

Once first-person eye height is calibrated, temporary native third-person states and late automatic SetMode frames keep the player-anchored VR view. The previous first-person heading is held during those transition frames. Conversations keep their native third-person camera. Initial loading waits for a real first-person camera height. Native pitch/roll and head-bone motion remain excluded; normal thumbstick/native yaw and HMD rotation are not filtered.

38/38 automated tests pass. Regressions replay native position impulses, noisy running at 45/90/120 Hz, forward/backward physical transfers with rotated tracking origins, terrain height changes, immediate physical head/hand motion, body correction and transient camera modes. These checks validate mathematical continuity and integration invariants; they do not confirm headset comfort or cure low frame rate. No 0.9.9 headset acceptance has been reported yet.

For the next headset trial, compare sword draw/sheath, idle head movement, joystick running with physical forward/backward steps, small slopes and returning from dialogue. If a jump remains, identify the action and whether it affects position, orientation or whole-frame fluidity. Logs include `vr_camera_settings` and `vr_camera_comfort` with the applied correction and transition-heading state.
