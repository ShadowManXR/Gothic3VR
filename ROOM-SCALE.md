# Room-scale and camera comfort — 0.9.9

0.9.9 adds render-time native camera stabilization, keeps physical head/hand poses immediate, and rebases accepted horizontal room transfers without smoothing their XR-origin cancellation. A calibrated player view also survives transient native camera switches; dialogues retain third-person. The eyes, tracked hands, controller focus and stable rendered body share the camera correction. See [CAMERA-STABILITY.md](CAMERA-STABILITY.md) for settings and integration.

38/38 automated tests pass and both launcher pages pass UI verification. Headset comfort and tandem walking acceptance for 0.9.9 are pending; the planned live trial has not been launched.

0.9.8 addresses the 0.9.7 headset report of forward/backward camera jerks during tandem walking. The CP/PUP movement frame history at +0x11c was not translated with room transfers. SaveFrameStates (0xed080) writes this previous root; CalcAniVelocity (0xe1cd0), CorrectGoalMiss (0xe4de0) and collision correction (0xe6760) read it. RoomNativeState now rebases that history by the accepted room displacement, along with sensor positions and goals. Native joystick displacement remains unchanged even during opposite physical steps. Physics interpolation synchronization and elapsed-time root bounds from 0.9.7 remain active; no head/controller pose filtering is added.

37/37 tests pass, including repeated forward joystick frames combined with forward/backward physical steps, exact unrelated-byte preservation and native fixed-step ABI/continuity checks. Ground-sword selection was confirmed by the user in 0.9.6; headset smoothness confirmation for 0.9.8 is pending.


0.9.7 addresses the 0.9.6 headset report: ground swords are now easy to pick up and physical steps no longer stop joystick walking, but tandem locomotion still stutters. The room transfer now runs after the native movement update, preserving its completed goals, height and velocity. It additionally calls SDK eCRigidBody_PS::SetFixedStepTranslation with the final absolute entity position, synchronizing both native interpolation endpoints. Head/controller tracking stays unfiltered. Root catch-up is limited to 3 m/s using elapsed player-update time, capped at 12 cm after pauses. NPC updates do not advance the player's timing clock.

37/37 tests pass, including the exact SHA-verified, isolated Engine.dll fixed-step setter bytes executed with the x86 thiscall ABI on a fixture. The new regression demonstrates the stale-endpoint camera pull after origin consumption, checks continuity across all physics blend fractions after synchronization, and verifies unrelated body bytes are untouched. Real headset smoothness remains pending in the 0.9.7 trial. Existing pickup/NPC behavior and all user settings are preserved.



0.9.6 native callback correction: the old interpretation of Game.dll helper 0x66c50 was incorrect. It tests NPC status Dead (9), not player identity. OnUpdatedWorldMatrix (0xe5b70, SDK/native vtable slot 68) calls StopMovement then SetMovementMode(Standing) for a living actor. The +0x264 guard suppresses only StopMovement, leaving the Standing transition and its velocity resets active. This matches the user's 0.9.5 report and the transfer logs showing mode=1.

The new hook is installed only after the complete Game.dll profile hash, SDK slot decode and named export/vtable target match. RoomMotionScope suppresses the callback for the exact transferring player movement PS on the application thread, spanning the native body/entity setters and final synchronization. The scope is cleared in finally even on faults; the previous native guard is restored before cleanup. No other actor, thread, native movement, teleport or dialogue notification is suppressed. Native collision checks and exact sensor/absolute-goal restoration stay in place. Logs now include world_callbacks and retained movement mode/speed.

Ground loot correction: 120 cm three-dimensional reach was too short from a standing controller to a ground sword, and the thin bounding box demanded excessive precision. PickupReachCm defaults to 180 cm only for Item property sets, with 15 cm contact and 6 cm plus approximately 6 degree ray tolerance. NPCs retain 120 cm, 10 cm contact and the prior tolerance. Both distances measure actual unpadded surfaces. A small score preference stabilizes the previously highlighted valid item. Native inventory/equipment filters, priorities and line of sight remain active. vr_pickup_focus records eligible candidates, geometric hits and LOS refusals for the live trial. Bow/spell crosshair aiming remains native.

36/36 tests pass, including SDK slot 68, the actual virtual hook ABI with the native standing-reset contract, scope cleanup, NPC/thread passthrough, and standing downward selection of thin loot. Launcher pages pass layout verification. User confirmation for simultaneous physical/joystick walking and picking up a ground sword is pending.

Historical 0.9.5 implementation and evidence follow; its claim that preserving the velocity guard alone prevented interruption was disproved by the live test.


Profile: CP 1.75/PUP 1.1.1. The existing movement hook validates the complete native Game.dll SHA256 4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00 and the Engine.dll hash before touching this profile. Native CameraAI SetMode RVA 0x2c4b0 / slot 88 selects persistent first-person with mode 11. Dialogues retain the native camera.

0.9.4's body following replaced the character movement goal and called MoveWorld without the native transfer guard. OnUpdatedWorldMatrix (RVA 0xe5b70) calls ResetMotion (0xeb660), which clears the current, animation and rigid-body velocities. A physical step therefore interrupted joystick running.

0.9.5 queries the native CharacterSensor ProcessPlayerMovements (RVA 0xeec80) with a separate temporary goal. RoomNativeState snapshots and restores only the sensor's verified scalar payload +0x14..0x5f: current, unsmoothed and previous positions; flags; borrowed movement pointer; goal; quaternion. The property-set header remains untouched. The sensor must be active (+0x38) and bound to this exact movement PS (+0x3c). Requests write only the goal vector +0x44 and goal-changed flag +0x40, avoiding SetGoalPosition's immediate position overwrite. Restoration also runs on faults.

The accepted room step uses the same scoped byte +0x264 guard as native controlled movement (0xe699a..0xe6a1b), so the world-matrix callback does not call ResetMotion. Both body and entity positions move through SDK methods. The saved sensor positions, movement goal +0x2c0 and latest goal +0x360 are translated by the actual accepted displacement; flags and velocity remain native. The rigid body's position and linear velocity are synchronized after the entity notification, with the original guard restored in the finally path. No physics object layout is written. Per-frame `vr_room_scale` includes mode, saved velocity and `tandem`.

Room transfer remains bounded to 12 cm per update, after an 8 cm optical leaning allowance. Origin jumps beyond 1.5 m are refused. Only accepted horizontal movement is consumed from the shared head/hand tracking origin; physical crouching stays optical, terrain height stays native. Menus, dialogue, stale tracking, jump/ragdoll, mounted and SPU combat movement do not transfer the root. RoomScaleWalking=0 disables root following independently of optical 6DoF.

With StableHorizon, the camera's horizontal base uses the player root; native animated head offsets are ignored. Eye height retains the existing session calibration. User height/forward offsets and full headset/controller tracking are applied afterwards, shared by both eyes, tracked arms and controller interaction.

Controller surface focus hooks native FocusCheck (RVA 0x6d7c0, verified six-byte prologue), keeping FindFocusEntity's inventory/equipment/NPC eligibility and priorities, and native IsFreeLOS. Reach is 120 cm from the controller to the object's world-node boundary, with 10 cm contact and modest ray padding for small items. There is no player-to-object-centre minimum distance. The native range-rating preference is suspended only during interaction and restored afterwards. Shape focus uses the candidate path during interaction only; crosshair bow/spell aim keeps the native path and range. InteractionReachCm is bounded to 30..250 and preserved by launcher saves.

Validation: 35/35 tests, including camera animation rejection with physical translation, exact sensor restoration, independent forward joystick/backward room goals, retained movement bytes, short surface reach/contact, and the four-argument native FocusCheck x86 ABI. Both launcher pages pass UI self-check. Live 0.9.5 headset acceptance is pending.

Live checklist: draw/sheath sword repeatedly; walk/run using the left stick while stepping physically forward/back/laterally; approach an NPC and pick up a ground object at close hand distance; check a wall and slope; reset L3+R3 and return from a dialogue. Existing settings are preserved. Physical sword swing damage and physical bow drawing remain pending.


