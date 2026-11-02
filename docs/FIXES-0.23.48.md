# 0.23.48 - shoulder compensation without reversing the elbow

The user accepted 0.23.47 locomotion, rotation, strafe and wrist/forearm following,
but dropping the tracked wrist could bend the elbow backwards. The previous
flexion solver moved the elbow while retaining a fixed shoulder and changing
segment lengths. This could change the elbow hinge drastically.

NaturalWrist now transports the solved elbow AND visual shoulder with the same
proper rotation about the fixed tracked wrist. It retains the 15 degree free
wrist range and soft 35 degree residual flexion limit. Both segment lengths,
elbow bend angle and hinge handedness survive the adjustment. Compensation goes
to the hidden upper-arm root rather than reversing or shortening the elbow.
The native entity, torso, camera, locomotion and physics are not moved by this
render-only adjustment. Shoulder mesh deformation still requires headset review.

NativeTrackedArms and progressive axial twist now use the compensated shoulder
as the upper-arm origin. Exact tracked palms, weapon/support contacts and
per-eye palette restoration remain in place. No hand calibration is changed.

Validation: release plugin compilation; 12/12 relevant CTest regressions pass.
Wrist flexion sweeps cover both arms, both bend directions, several world yaws,
0..120 degrees, continuous shoulder/elbow paths, preserved segment lengths,
unchanged elbow angle and transported hinge direction, exact palm and upper/
lower endpoints after progressive twist. Existing grip, rigid weapon attachment,
two-hand, crossbow stereo, native input and camera checks also pass.
Native imports and FMOD compatibility are audited separately in the build record.
Offline tests do not establish headset acceptance.

Accepted locomotion/camera/input source files remain byte-identical to installed
0.23.47. Launcher/settings, calibration, native game DLLs, PUP and graphics
backends remain unchanged. DLAA stays off. DX11 trial plugin hash/version is
updated without enabling the experiment or launching the game.
