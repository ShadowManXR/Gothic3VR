# Historical diagnostics archive

Archived development notes, retained verbatim for context. These describe obsolete experiments and are not current release or installation instructions. See the root README and release/START-HERE.md instead.

---

0.23.19: opt-in live temporal input collector for native rigid, instanced, wind and skinned geometry. Copies native entity identities, tracks actual buffer writes, and composes both eyes on GPU before Gothic reuses its depth surface. Unknown coverage remains invalid. This build does not activate game DLAA; full scene coverage and the game NGX worker still require integration and live validation. See DLAA-LIVE-INPUTS-0.23.19.md.

0.23.8: real hardware FFR through a small DXVK 3.1.1 Win32 fork. Native 1x1 center, adjustable protected diameter,2x2 periphery and optional4x4 extreme corners. Explicit eye scopes; UI/cinematics excluded; actual GPU and stereo lifecycle tests. See FFR-0.23.8.md. Live headset/FPS validation pending.

0.23.7: NIS/CAS spatial upscaling and sharpening in the native D3D11/OpenXR bridge, with true lower-resolution D3D9/DXVK scene rendering and a Graphics launcher page. Off by default; restart required. 107 tests pass, including real GPU/5K/VDXR typeless images. Live VR/FPS comparison pending. See UPSCALING-0.23.7.md.

0.23.6: VR stabs select native Pierce only for Human/Orc victims, matching the native IsHumanoid rule. Animals and other creatures receive ordinary/power physical-contact damage from the same thrust. Independent stab distance/speed, native armor/skills, one hit per target and one stamina debit per stroke are preserved.

0.23.5: optional Windows/x86 DXVK 3.1.1 GPU bridge. Auto-detects DXVK, imports native D3D11 NT textures into Vulkan, and preserves the D3D11 OpenXR/VDXR session. 101 tests pass, including actual Runtime GPU copies at 4K/5K. Live VR/FPS comparison pending. See DXVK-0.23.5.md; tools/Disable-DXVK.ps1 restores native D3D9 while keeping VR.

0.23.4: Sneak, Block and Finish use matching compact icons at one bottom-center anchor above the HUD; Finish is a red skull. Position follows HUD size, distance and vertical offset.

0.23.3: corrected left/right joystick yaw direction; continuous integration and turn speed unchanged.

0.23.2: continuous native joystick yaw without OS mouse events or replay during PreRender. Short failed interactions no longer trigger the activity camera/recenter cycle. Real activities, knockouts and finishers retain third-person. VR testing pending.

0.23.1: controller locomotion now submits native actions together at the native movement tick, without Windows movement keys. Reverted the recent movement-heading/cache-pose changes. Campfire interaction accounts for a standing hand above the hearth. VR testing pending.

# Current release — 0.23.0

Raised-hand native block with shield feedback; native inventory quick heal on left trigger; Sprint ends on stop; X inventory / B weapon / radial B page and X cancel. Smaller hearth target, native Pierce for qualified VR stabs, stable cached first-person body pose in menus/weapon transitions, attached torch particles and native signpost labels. Live VR verification required.

## Previous release notes

# Current release — 0.22.25

Restore the native camera threading allowance for the exact supported PUP profile. The0.22.24 live probe showed the old stereo call forced all53856 maintenance calls into the synchronous path; its budget never applied. Native loading/distance fallbacks and per-eye frusta remain intact. Live25 confirms optional work admission; gameplay PVS max28.907ms versus277.4474ms in the earlier, longer24 session. Remaining whole-frame stalls need separate analysis; this is not a controlled FPS claim. See FIXES-0.22.25.md.

## Previous release notes

# Current release — 0.22.24

Coordinate PUP1.1.1 optional prefetch maintenance with one shared stereo-frame allowance and nonblocking queue admission. Preserve forced/loading/native work and the existing PUP Execute hook. Live hitch/visual verification pending. See FIXES-0.22.24.md.

## Previous release notes

# Current release — 0.22.23

Split large native-render hitches into scene-data refresh, zone prefetch processing/execution and its public job-queue drain. Diagnostic release; native resource scheduling, both-eye rendering and the accepted gameplay stay unchanged. See FIXES-0.22.23.md and DXVK-SCENE-NOTES.md.

## Previous release notes

# Current release — 0.22.22

Measure five original native render phases and original D3D9 draw CPU time to locate first-eye hitches. Split first-shot ranged timing into native script, VR preparation and native projectile launch. Diagnostic-only release; live root cause and FPS gains remain unproven. See FIXES-0.22.22.md.

## Previous release notes

# Current release — 0.22.21

Reduce empty finishing-target searches while preserving immediate execution validation. Release ranged ammo borrows without re-equipping stale stacks during native menu/equipment transitions. Add independently measured native session/focus phases and CPU spike locations. Native render hitches remain under investigation; gameplay comparison is pending. See FIXES-0.22.21.md.

## Previous release notes

# Current release — 0.22.20

Reduce finishing-target visibility work without changing target selection. Add separate native/VR update CPU timings and sparse nonblocking GPU eye-render timestamps. Existing controls, visual settings, tracking and progression are preserved. Gameplay FPS comparison remains pending. See FIXES-0.22.20.md.

## Previous release notes

# Current release — 0.22.19

R3 opens the radial, right stick selects, A confirms. Native secondary action / block moves from left trigger to right grip. Camera turn waits for stick neutral after closing the radial. The user reports no further movement lockups with 0.22.18. See FIXES-0.22.19.md.

## Previous release notes

# Current release — 0.22.18

Recover orphaned native combat translation and idle animation attacks in physical VR melee. Use third-person with a visible head for every knockdown/knockout and native get-up, returning to calibrated first-person afterward. Live combat verification remains pending. See FIXES-0.22.18.md.

## Previous release notes

# Current release —0.22.17

Center the body using a shared headset neck target for room-scale follow and the rendered rig. Align the torso during locomotion; retain 30-degree free look at rest. Preserve native grounded movement, eye/controller transforms and the accepted audio fix. 84/84 offline tests pass; headset acceptance is pending. See FIXES-0.22.17.md.

## Previous release notes

# Current release —0.22.16

Remove the unsupported FMOD pan getter that opened a warning dialog; retain hero-voice centering using the installed wrapper's supported calls. Physical head/body free look increases to 30 degrees. 83/83 tests and an actual wrapper/compiled-plugin export audit validate the update; headset acceptance remains pending. See FIXES-0.22.16.md.

## Previous release notes

# Current release —0.22.15

The body follows physical headset yaw with a 12-degree free-look cone, independently of eye/controller tracking. Native ground alignment uses a short owned direction request; script activities, knockout and menus retain native authority. Includes 0.22.14 movement/knockout/hero-voice fixes. 83/83 offline tests pass; headset acceptance is pending. See FIXES-0.22.15.md.

## Previous release notes

# Current release —0.22.14

Native Run/Sprint now yields to special movement and recovery. Nonlethal knockout uses third-person through native get-up, with calibrated first-person return; ordinary knockdown stays first-person. Hero speech is centered independently of NPC spatial audio. 82/82 offline tests pass; live headset acceptance is pending. See FIXES-0.22.14.md. Same-frame transfer optimizations from 0.22.13 are retained.

## Previous release notes

# Current release —0.22.13

Batch same-frame stereo/UI transfers behind one D3D9 fence and one D3D11 fence; poll without repeatedly flushing the driver. Fixed-storage stage timings identify native-render, UI, transfer and runtime pacing costs. Resolution, both-eye culling, gameplay and accepted hand/camera behavior are retained. See PERFORMANCE.md and FIXES-0.22.13.md. Live game performance acceptance is pending.

## Previous release notes

# Current release —0.22.12

Fix0.22.11 particle-query overhead: callbacks compare a bounded list of preselected held-weapon particle addresses, with no per-scene SDK/name/memory work. 78/78 tests pass; live performance acceptance pending. See FIXES-0.22.12.md.

## Previous release notes

# Current release —0.22.11

Player-held Flame Sword world-tail copies stop rendering/processing in tracked first-person VR, retaining blade-local fire; staffs hide temporarily during menus. 77/77 tests pass; headset acceptance pending. See FIXES-0.22.11.md.

## Previous release notes

# Current release —0.22.10

Anatomical controller-to-palm pivot removes the hidden8cm rotation lever; standard2H support contact moves under closed fingers. 75/75 tests pass; headset acceptance pending. See FIXES-0.22.10.md.

## Previous release notes

# Current release —0.22.9

Live per-profile melee weight/stabilization for1H, supported2H and staff held with one/two hands; rigid palm attachments and tracking-space history. 74/74 tests pass; headset acceptance pending. See FIXES-0.22.9.md.

## Previous release notes

# Current release — 0.22.8

Progressive visual rotation through wrist, forearm and upper arm, preserving exact tracked hands and weapon contacts. 73/73 tests pass; headset acceptance pending. See FIXES-0.22.8.md.

## Previous release notes

# Current release — 0.22.7

Independent per-eye visibility and positioning for static nocked arrows/bolts; owned launched projectiles explicitly enabled at native Shoot. 72/72 tests pass; headset acceptance pending. See FIXES-0.22.7.md.

## Previous release notes

# Current release — 0.22.6

2H support captures the current rendered pose without a45-degree axis realignment. Weighted one-hand pose and rigid palm contacts are retained on acquisition. 71/71 tests pass; live acceptance pending. See FIXES-0.22.6.md.

## Previous release notes

# Current release — 0.22.5

Native Sneak weapon restriction restored; no equipment suspension/auto-resume. L3 offers only Run/native Sprint, including activation from idle. Sneak remains active in radial. 71/71 tests pass; headset acceptance pending. See FIXES-0.22.5.md.

## Previous release notes

# Current release — 0.22.4

Sprint/jump opacity corrected for both VR eyes. L3 tap toggles native Sprint; double-tap Y toggles Sneak. Draw/sheath suspends and resumes selected Sneak through native equipment logic. 71/71 tests and15 launcher pages pass; live acceptance pending. See FIXES-0.22.4.md.

## Previous release notes

# Current release — 0.22.3

Y toggles Sneak, including armed melee; left grip + joystick is native Sprint; Menu button opens/back; R3 radial with left joystick/A selection. Lockpick requires a stable centre stop between single A/D guesses. Swing/stab velocity sliders reach 800 cm/s. 70/70 tests, 15 launcher pages; live acceptance pending. See FIXES-0.22.3.md.

## Previous release notes

# Current release — 0.22.2

Independent Minimum stab speed per melee profile; A/D during native non-GUI interactions; Sneak invokes logical SessionKey.Down through the native script path, independent of keyboard bindings. 69/69 tests and 15 launcher pages pass. Live game acceptance pending. See FIXES-0.22.2.md.

## Previous release notes

# Current release — 0.22.1

Separate short Stab distance minimum for each melee profile; lockpick receives actual A/D keyboard steps without routine detection. 67/67 tests and15 launcher pages pass. Current settings retained; live headset acceptance pending. See FIXES-0.22.1.md.

## Previous release notes

# Current release — 0.22.0

Physical melee now requires sufficient speed as well as travel. Lockpick/menu directional controls, a less sensitive native strafe cone, a small lower Sneak eye, red lower FINISH and third-person native finishing are included. 66/66 tests and 15 launcher pages pass; headset acceptance pending. See BATCH-0.22.0.md.

## Previous release notes

# Current release — 0.21.1

Rigid melee-to-palm binding for1H/2H/staff, coordinated right staff grip and whole-assembly unsupported2H weight. Shared both-eye poses now include1H/dual wield. 65/65 tests pass; headset confirmation pending. Launcher/settings retained. See FIXES-0.21.1.md.

## Previous release notes

# Current release — 0.21.0

Shield palm tracking/calibration and left-trigger native shield/1H/2H parry; shared both-eye 2H/staff/shield pose; native movie VR panel; native skill audit and qualified Murder thrust. 64/64 tests and 15 launcher pages; headset checks pending. See BATCH-0.21.0.md.

## Previous release notes

# Current release — 0.20.2

Finishing focus uses native interaction ROI and a grounded-body sight point. Ranged ammo ownership clears before native callbacks and shared bow/crossbow failures recover per current player/weapon/session, without replaying shots. Native locomotion remains. 64/64 tests; real headset selection/crossbow recovery pending. See FIXES-0.20.2.md.

## Previous release notes

# Current release — 0.20.1

Direct VR locomotion is removed. Native movement is the only joystick path. Run, left-grip Sneak and R3 Sprint no longer depend on a direct motor; all native movement properties remain. The direct checkbox and acceleration slider are removed. 64/64 tests, fourteen launcher pages; headset test pending. See FIXES-0.20.1.md.

## Previous release notes

# Current release — 0.20.0

Full VR view-based continuous locomotion, Run without Walk, left-grip Sneak with skill/native-state feedback, restored Y main menu, independent per-category damage/stamina swing sliders in 10 cm steps, and native third-person for object activities. 64/64 tests and fourteen launcher pages pass. Actual headset behavior remains pending. See BATCH-0.20.0.md.

## Previous release notes

# Current release — 0.19.1

Adds live radial size, direct Y sneak with native-state feedback, a 50 cm tracked weapon-path gate before melee damage/stamina, corrected native sprint speed and dedicated downed-target FINISH feedback/selection. 63/63 tests and fourteen launcher layout checks pass. Ground anchoring and power strike accepted in 0.19.0 are retained. See FIXES-0.19.1.md; new headset validation remains pending.

## Previous release notes

# Current release — 0.19.0

Restores native ground-height correction for direct VR walking, adds a smaller Gothic-style radial, native-bound sprint/sneak, melee stamina, skill-dependent trigger power strikes and deliberate native finishing kills. 62/62 automated tests pass. See BATCH-0.19.0.md for controls, policies and the pending headset probe. Accepted bow behavior and all saved calibration are retained.

## Earlier release notes

# Current release — 0.18.1

Restores and invalidates VR world bindings before native session/workspace teardown; fixes repeated accesses to old held-ammo pointers during exit. Native crash diagnosis and pending live exit validation are in FIXES-0.18.1.md. NPC callbacks no longer reset the player movement motor. Accepted bow behavior and launcher binary are retained. 61/61 automated tests pass.

## Previous release

# Current release — 0.18.0

Direct controller ground movement replaces the animation-goal translation generator for the active human player, with no camera position smoothing in that mode. Crossbow support and held-bolt palettes remain applied across native cache hits and stereo passes. The user accepted the bow as finalized in 0.17.1. 60/60 automated checks pass; new comfort/render behavior awaits headset validation. See FIXES-0.18.0.md for controls, evidence and the probe.

## Earlier release notes

# Current behavior — 0.16.4

Visible bow charge, left-guided supported crossbow, independent Magic palm contacts and X+Y native return from animal form with restored human eye height. 55/55 automated checks and 12 launcher pages pass. New headset behavior awaits validation. Existing calibration is preserved. See FIXES-0.16.4.md.

## Previous release history

# Gothic 3 VR — 0.16.3

Bow draw readiness now follows learned tiers, every arrow receives deterministic native charge and linear velocity, transformed animal view releases first-person latches, and Cast items/fingers stay anchored in the calibrated palm. 54/54 automated checks pass; new headset behavior awaits validation. Existing calibration is preserved. See FIXES-0.16.3.md.

# Previous release: 0.16.2

0.16.2 fixes stale ranged samples across save/camera transitions and replaces the shared permanent bow/crossbow render fault latch with bounded automatic recovery. Model caches follow native asset names; replacement actors never receive old palette snapshots. Calibration and native firing/progression are preserved. See FIXES-0.16.2.md. 52/52 automated tests pass; the reported older-save/quick-slot sequence awaits headset validation. Previous release notes below retain their original validation scope.

0.16.1 resolves the actual compiled hero Cock alias (2.8 seconds in this installation), fixes mixed quaternion/vector motion tracks, moves and reveals casting items before culling, keeps transformed creatures in native third-person VR, doubles unsupported 2H rotation speed, and preserves native melee progression bonuses instead of clipping them to weapon base damage. Full controller reach was accepted by the user in 0.16.0. See FIXES-0.16.1.md and PROGRESSION-AUDIT.md. New fixes await headset validation.



The user confirmed the 0.14.0 headset probe: both dual wield 1H weapons remain visible and physical combat works with both hands. This records the tested setup; it does not validate every weapon template or physical parrying.


Both drawn 1H weapons now use their own calibrated controller pose, native attachment and tracked spatial bounds before per-eye frustum collection. The left weapon has an independent swing history, contact sweep, hit set and damage cap from its own Damage property. Exact tracked wrists avoid clipping extended hands back toward the body. Matrix and rendering visibility overrides are restored after each eye and on faults. Native inventory/skill requirements remain in force.



47 automated tests pass, including independent left/right movement, simultaneous samples and hits, duplicate-eye suppression, separate weapon caps, tracking/equipment/menu resets and the existing staff/2H/bow suite. The user confirmed both weapons visible and physical combat functional in the 0.14.0 headset probe. Existing hand and weapon calibration is preserved. See DUAL-WIELD.md.





The user confirmed both Staff/Halberd palm contacts in the 0.13.6 headset probe. Right-palm orientation and upward direction were accepted earlier. This confirms visual grip contact; the feedback does not separately validate both-end damage, every staff/halberd template, or a particular contact setting value.



The user confirmed the right palm orientation and upward staff direction, but reported the shaft on the back of the right hand and support capture at the left wrist. This version adds mirrored palm depth (+2 cm default) and applies the finger/depth contact offsets to both hands. The right weapon remains independent of its tracked hand. The support wrist is solved from the same corrected local palm contact used to acquire the shaft. Primary anchoring and support capture now share the actual pivot-centered shaft instead of different bounding-box and handle axes. Existing Grip toward fingers values are preserved; the removed reversal setting stays ignored.



46 automated tests cover asymmetric halberd bounds, mirrored palm offsets with arbitrary hand calibration, support contact through rotation/translation, unchanged accepted direction and right-hand tracking, locomotion continuity, physical shaft contacts, damage, and existing 1H/2H/bow paths. Launcher verification and corrected headset contact are tracked in the release artifacts. See STAFF.md.



0.10.2 aligns the drawn melee weapon's measured mesh axis forward in the calibrated controller frame, rotating the hand and weapon together around the grip. The Sword launcher tab adjusts pitch/yaw/roll separately from unarmed hand calibration. Accurate PhysX capsule overlaps along the blade complement ray sweeps for contacts starting inside close creature colliders. Wrist-only angular reversals now rearm strokes on pitch, yaw and roll. 40/40 tests pass, including geometry, grip transforms, all-axis gestures and rotation-only reversals. All six launcher pages passed layout checks; Sword was visually reviewed. Headset validation is pending. The user confirmed recovery and live hand calibration in 0.10.1.



0.10.1 adds separate Left hand / Right hand device calibration tabs, with three position and three rotation axes. Existing configurations start at 5 cm outward per hand; Save settings applies changes live. Native knockdown/get-up inputs and animations are retained while physical melee is suspended. Damage scaling now intercepts the final native ChangeHitPoints delta, because OnDamage recalculates damage and overwrote the 0.10.0 cap. 40/40 tests pass, including this regression and recovery/calibration cases; all five launcher pages passed layout checks. See HAND-CALIBRATION.md and PHYSICAL-COMBAT.md. Headset verification of these corrections is pending.



0.10.0 replaces click-driven VR melee with contacts from the tracked sword's swept blade. Short strokes deal less damage, broad strokes reach the weapon's maximum; native damage calculation runs before scaling. Native melee click attacks are suppressed while a sword is drawn in VR. Menus, radial selection and ranged inputs remain available. See PHYSICAL-COMBAT.md for the implementation, calibration thresholds and pending headset validation. Earlier release notes below describe their respective versions.



0.9.11 adds the Interaction launcher tab with Selection vertical aim: a signed slider and numeric field from -200 to +200 percent. Save settings applies the selection calibration live within approximately one second, including while the game is running. Positive values tilt object/NPC selection upward, negative values downward, and zero restores the original controller direction. For the reported need to bend the wrist down, try -15 or -30 percent. The existing saved calibration is preserved. The expanded range is used consistently by settings loading/saving and the native selection ray; tracked hands, menus and ranged crosshair queries keep their existing transforms.



38/38 tests pass, including signed negative calibration round-trips and the extended range in actual ray mathematics. All three launcher pages passed layout verification. Preferred headset calibration is to be determined by the user.



0.9.10 raises object/NPC selection by InteractionAimUpPercent=15 in [Controllers]. This is a world-elevation correction of atan(0.15), approximately 8.53 degrees or 15 cm rise at one metre for a level controller. It does not translate the tracked hand or camera. Candidate selection and the native focus camera use the same corrected direction; controller roll cannot turn the correction sideways. Ranged crosshair queries and menu pointing remain unadjusted. The value is reloaded live, preserved by launcher saves, bounded to -50..50, and can be set to 0 to restore unadjusted selection. Camera stabilization from 0.9.9 is retained.



38/38 automated tests pass; raised selection alignment in the headset remains pending.



0.9.9 adds render-time native camera stabilization, keeps physical head/hand poses immediate, and rebases accepted horizontal room transfers without smoothing their XR-origin cancellation. A calibrated player view also survives transient native camera switches; dialogues retain third-person. The eyes, tracked hands, controller focus and stable rendered body share the camera correction. See [CAMERA-STABILITY.md](CAMERA-STABILITY.md) for settings and integration.



38/38 automated tests pass and both launcher pages pass UI verification. Headset comfort and tandem walking acceptance for 0.9.9 are pending; the planned live trial has not been launched.



0.9.8 addresses the 0.9.7 headset report of forward/backward camera jerks during tandem walking. The CP/PUP movement frame history at +0x11c was not translated with room transfers. SaveFrameStates (0xed080) writes this previous root; CalcAniVelocity (0xe1cd0), CorrectGoalMiss (0xe4de0) and collision correction (0xe6760) read it. RoomNativeState now rebases that history by the accepted room displacement, along with sensor positions and goals. Native joystick displacement remains unchanged even during opposite physical steps. Physics interpolation synchronization and elapsed-time root bounds from 0.9.7 remain active; no head/controller pose filtering is added.



37/37 tests pass, including repeated forward joystick frames combined with forward/backward physical steps, exact unrelated-byte preservation and native fixed-step ABI/continuity checks. Ground-sword selection was confirmed by the user in 0.9.6; headset smoothness confirmation for 0.9.8 is pending.





0.9.7 addresses the 0.9.6 headset report: ground swords are now easy to pick up and physical steps no longer stop joystick walking, but tandem locomotion still stutters. The room transfer now runs after the native movement update, preserving its completed goals, height and velocity. It additionally calls SDK eCRigidBody_PS::SetFixedStepTranslation with the final absolute entity position, synchronizing both native interpolation endpoints. Head/controller tracking stays unfiltered. Root catch-up is limited to 3 m/s using elapsed player-update time, capped at 12 cm after pauses. NPC updates do not advance the player's timing clock.



37/37 tests pass, including the exact SHA-verified, isolated Engine.dll fixed-step setter bytes executed with the x86 thiscall ABI on a fixture. The new regression demonstrates the stale-endpoint camera pull after origin consumption, checks continuity across all physics blend fractions after synchronization, and verifies unrelated body bytes are untouched. Real headset smoothness remains pending in the 0.9.7 trial. Existing pickup/NPC behavior and all user settings are preserved.





0.9.6 corectează resetarea stării de mers și ridicarea armelor de pe sol. Proba 0.9.5 a păstrat viteza, dar notificarea nativă OnUpdatedWorldMatrix trecea totuși personajul viu în Standing, oprind mersul înainte când se adăuga un pas fizic. Hookul nou verificat la slotul 68 suspendă această notificare numai pentru transferul fizic al jucătorului, pe firul jocului, cu restaurare la excepții. Toate celelalte notificări, inclusiv NPC și teleportări native, rămân înlănțuite.



Obiectele de ridicat au separat PickupReachCm=180 și o toleranță mai mare pentru obiectele subțiri. Astfel, o sabie aflată pe sol poate fi țintită de la înălțimea unei mâini în picioare, fără a extinde raza personajelor (InteractionReachCm=120). Selectarea obiectului deja evidențiat are o preferință mică pentru stabilitate. Filtrele native și vizibilitatea sunt păstrate, iar țintirea arcului/magiei rămâne nativă. Cele două raze sunt limitate la 30–250 cm în INI și sunt reîncărcate live.



36/36 teste trec, inclusiv regresia în care protecția vitezei nu împiedică schimbarea în Standing, hookul virtual x86, păstrarea alergării în transfer, notificările pentru NPC/alt fir și cleanup la excepții. Testele de selecție reproduc sabia subțire pe sol, dintr-o poziție în picioare. Acceptarea vizuală a probei 0.9.6 rămâne în așteptare; setările utilizatorului se păstrează.





0.9.5 corectează camera la scoaterea sabiei, mersul fizic combinat cu joystickul și interacțiunile apropiate. Cu Stable horizon activ, baza orizontală a camerei urmărește rădăcina personajului, nu osul capului animat; deplasarea/rotația HMD și offseturile utilizatorului rămân aplicate ulterior. Transferul fizic păstrează viteza, starea de alergare și ținta joystickului, iar senzorul nativ verifică separat pasul fizic. Interacțiunea se măsoară de la controllerul drept la suprafața obiectului, cu 120 cm implicit și contact apropiat de 10 cm. Filtrele native pentru obiecte, inventar, NPC și vizibilitate se păstrează. [Controllers] InteractionReachCm permite 30–250 cm și este reîncărcat live; setarea este momentan în INI. Țintirea arcului/magiei păstrează distanța nativă.



35/35 teste trec, inclusiv restaurarea stării de mers, păstrarea țintei joystickului în timpul unui pas fizic în sens opus, anularea deplasării camerei la animația armei și selecția suprafețelor de aproape. Proba 0.9.5 în cască este încă în așteptare. Reglajele existente sunt păstrate.









0.9.4 activează automat camera first-person după încărcare și după întoarcerea din dialog, prin SetMode(11) în versiunea CP verificată. Dialogul păstrează camera nativă. O întârziere de randare sau o mostră expirată folosită pentru mers nu mai anulează calibrarea stereo; recentrarea se face doar când este cerută sau se schimbă originea OpenXR.



Opțiunea **Room-scale walking — character follows physical steps** este activă implicit, cu Stable horizon. Capul păstrează urmărirea poziției și rotației; după o deplasare orizontală de peste 8 cm, corpul preia diferența prin senzorul nativ de coliziune. Transferul este limitat la 12 cm per actualizare și scade din originea comună a ochilor și mâinilor numai distanța acceptată efectiv. Coborârea/înclinarea fizică a capului rămâne optică; înălțimea terenului rămâne nativă. În meniuri, dialog, tracking expirat/incomplet, salt, ragdoll și mers comandat de SPU, corpul nu urmărește deplasarea. [Locomotion] RoomScaleWalking=0 dezactivează numai urmărirea corpului, păstrând camera 6DoF.



35/35 teste trec; ambele pagini ale launcherului trec verificarea de layout. Utilizatorul a confirmat în proba 0.9.4 activarea automată first-person și deplasarea fizică. Logul PID 17588 confirmă transferuri către corp și închiderea normală a motorului; comportamentul lângă pereți și pe pante rămâne de verificat separat. Utilizatorul a confirmat în 0.9.3 că postura corpului și mâinile sunt corecte. Damage-ul din balansul sabiei și tragerea fizică a arcului rămân etapa următoare.





0.9.3 adaugă **Stable body — neutral torso, animated walking** în Camera and controls. Cu Tracked arms activ, stabilizează vizual bazinul, trunchiul și umerii în first-person VR, eliminând deplasarea și rotirea posturii de luptă. Corpul folosește direcția de bază a camerei și poziția actorului; nu mută entitatea sau colliderul. Picioarele păstrează animația relativă la bazin, iar IK pornește de la umerii realiniați. Matricile native sunt restaurate după fiecare ochi. NPC-urile, dialogul și ragdoll-ul rămân pe traseul nativ. [Controllers] StableBody=1 este implicit pentru configurațiile existente și se poate dezactiva/salva live din launcher. 34/34 teste trec, inclusiv geometria corpului și restaurarea paletei; postura corpului și mâinile au fost confirmate de utilizator în proba 0.9.3.





0.9.2 corectează orientarea palmelor raportată în proba 0.9.1. Calibrarea folosește planul articulațiilor index/deget mic, apoi axele anatomice ale prinderii OpenXR (degetele spre -Y, direcția deget mic spre index pe -Z OpenXR / +Z Genome). Baza degetului mare nu mai este folosită ca direcție în sus. Testele verifică ambele mâini, orientarea palmelor și rotația controllerului. Orientarea palmelor este confirmată de utilizator în proba 0.9.2.



0.9.0 adaugă o probă pentru brațele și armele reale ale personajului: grip OpenXR → spațiul stereo → IK pentru umăr/cot/mână → matricile de skinning ale jucătorului. Armele păstrează atașamentul nativ, sabia urmărește dreapta, iar arcul stânga. Matricile și transformările sunt restaurate după fiecare ochi, inclusiv pe traseul de excepție. Sunt 33/33 teste trecute; alinierea și vizibilitatea pe modelul real necesită proba în cască. **Damage-ul din balansarea sabiei și lansarea săgeții prin întindere fizică nu sunt încă implementate.** Comenzile de atac pe trigger rămân cele native în această probă.



0.9.1 corectează recunoașterea oaselor Hero folosind lista reală capturată din joc: 73 de oase, cu lanțurile brațelor la 7/8/9 și 26/27/28, plus cele 30 de oase ale degetelor. Proba 0.9.0 nu recunoștea aceste nume și păstra animația nativă, confirmat de utilizator. Logul 0.9.1 confirmă calibrarea ambelor brațe, aplicarea/restaurarea paletei și maparea sabiei în dreapta; utilizatorul confirmă mișcarea brațelor, dar raportează palmele orientate în sus.



Bifa în engleză **Tracked arms and weapons (experimental)** activează proba; se poate dezactiva și salva în timpul jocului. Radialul și selecția sunt confirmate în 0.8.2, iar interfața radialului și a launcherului este în engleză din 0.8.3.



**Meniu radial VR:** după încărcarea salvării, apasă o dată joystickul stâng (**L3**), selectează cu joystickul drept și apasă **A** pentru confirmare sau **B** pentru anulare. Nu trebuie să ții L3 apăsat. Selecția rămâne memorată când joystickul revine în centru, ca să poți muta degetul la A. Prima pagină activează sloturile native **1–9 și 0**; **Y**, cât timp radialul este deschis, comută la inventar, personaj, jurnal, hartă, magie, armă, ia tot și înapoi. O nouă apăsare L3 închide fără confirmare. Radialul este disponibil și în inventar: indică mai întâi obiectul cu cursorul, apoi folosește radialul pentru a trimite tasta slotului către interfața nativă. **L3 + R3** are prioritate pentru resetarea camerei. Mersul, rotirea și atacul sunt suspendate în radial.



În proba 0.8.1, radialul se anula după redesenări de aproximativ 235 ms: mostra controllerelor expira la 100 ms. În 0.8.3 geometria este calculată o singură dată la conectarea VR; actualizările reutilizează masca. O întârziere grafică suspendă comenzile native, păstrând radialul, selecția și muchiile butoanelor până la următoarea mostră proaspătă. Pierderea reală a focusului, schimbarea meniului, ieșirea din lume și resetarea camerei anulează în continuare radialul. R3 singur nu anulează, iar A fără selecție lasă radialul deschis. Testele includ reluarea exactă a întârzierilor din proba live și compararea tuturor pixelilor geometriei. Radialul a fost confirmat în cască în proba 0.8.2.



Radialul folosește asocierile existente din `UserOptions.ini`; nu schimbă obiectele atribuite sloturilor rapide. Are propriul panou stereo de 768×768, separat de textura meniurilor Gothic 3 și de rezoluția per ochi. Cele 31 de teste includ toate sloturile, cele două pagini, anularea, prioritatea resetării, taste remapate, eliberarea apăsărilor și încărcarea efectivă a imaginii în D3D11. Radialul și selecția au fost confirmate în proba 0.8.2.



Mâinile sunt urmărite în 6DoF, iar țintirea folosește controllerul drept; proba mută brațele și armele, dar nu comandă încă damage-ul prin gest sau întinderea arcului. Acestea sunt următoarea etapă cerută: sabia în dreapta, arcul în stânga și mâna dreaptă pe coardă. Vezi [WEAPON-INTEGRATION.md](WEAPON-INTEGRATION.md) pentru punctele de integrare identificate.



În 0.7.6, resetarea din controller este aplicată numai de cadrul stereo. În 0.7.5, actualizarea mersului putea prelua cererea mai devreme, folosind rotația completă a căștii și lăsând originea poziției neschimbată; utilizatorul a raportat revenirea înclinării verticale pe pante. Cadrul stereo recentrează acum împreună poziția și yaw-ul, cu Orizont stabil activ, fără recalibrarea înălțimii personajului. Testul reproduce ordinea resetare → actualizare mers → randare stereo, cu cap înclinat și poziție deplasată.



**Reset cameră: apasă ambele joystickuri (L3 + R3) timp de 0,5 secunde**, apoi eliberează-le. Recentrarea anulează deplasarea și yaw-ul trackingului, păstrează înălțimea personajului și offseturile din launcher. Schimbările originii LOCAL anunțate de runtime-ul OpenXR sunt tratate la momentul indicat de runtime, pentru resetarea din Quest / Virtual Desktop. În meniuri, cursorul controllerului are filtrare adaptivă și stabilizare de 100 ms la apăsarea triggerului. Laserul de interacțiune a fost eliminat; selecția continuă prin controllerul drept și evidențierea nativă a numelui țintei.



Înălțimea ochilor este păstrată la intrarea/ieșirea din conversații și la recentrarea HMD. Camera nativă ridicată din tranziția de dialog nu mai poate deveni noua înălțime first-person. Calibrarea se reface la schimbarea personajului, ieșirea din lume, dezactivarea stabilizării sau închiderea sesiunii VR. Viteza de rotire la 800% a fost confirmată de utilizator în proba 0.7.2.



Viteza rotirii pe joystick poate fi reglată între 10% și 2000%, cu valoarea implicită 500%. Intervalul anterior limita reglajul la 200%, insuficient în proba utilizatorului. Logul confirmă setările aplicate și schimbările lor în timpul jocului.



Reglajele de confort includ viteza rotirii orizontale pe joystick, offsetul camerei înainte/înapoi și orizont stabil cu eliminarea balansului vertical din animația capului. Launcherul grupează reglajele în două file. Testele verifică și păstrarea înclinării reale a căștii, a săriturilor și a deplasării pe teren.



Comenzile Quest Touch și pointerul pentru meniuri sunt documentate în [CONTROLLERS.md](CONTROLLERS.md). Bogdan a confirmat țintirea și interacțiunile prin controllerul drept în 0.7.4, pe 10 octombrie 2026, apoi resetarea corectă pe pantă în 0.7.6. Cele 30 de teste automate trec. Logul probei 0.7.6 arată trei resetări din controller aplicate în următorul cadru stereo, la 4–7 ms după cerere, și eliberarea resurselor înaintea închiderii motorului. Stabilizarea cursorului și eliminarea laserului nu au încă o confirmare explicită. Rezoluțiile 4K și 5K au fost confirmate în cască, inclusiv accesul mouse-ului la inventar și conversații.



Versiunea curentă include stereo geometric, tracking 6DoF prin Virtual Desktop/VDXR, HUD și meniuri în cască și un launcher Windows separat. Instrucțiunile curente sunt în [LAUNCHER.md](LAUNCHER.md). Secțiunile de mai jos documentează etapele anterioare de diagnostic și mișcare.



Rezoluția se alege înainte de lansare și este aplicată randării native pentru fiecare ochi. Patch-ul `patches\Patch_Gothic3VRResolution.dll`, încărcat devreme de PUP, permite dimensiuni de fereastră care lipsesc din lista monitorului. Interfața nativă este sincronizată cu backbuffer-ul atât înainte de actualizarea jocului, cât și înainte de desenare, inclusiv la schimbarea paginii. Mouse-ul acoperă întreaga interfață chiar când fereastra desktop este mai mică sau parțial în afara monitorului. Bogdan a confirmat în proba live 4K că inventarul și conversațiile sunt centrate și toate opțiunile pot fi selectate. Cele 25 de teste automate trec.



Prima etapă a modului VR: observarea camerei și a ciclului de randare Genome. Proiectul este în `D:\Gothic 3 VR\gothic3vr`, alături de `gothic3sdk` și `vhvr-mod`.



Pluginul este C++17, Win32/x86. Folosește binding-urile Gothic 3 SDK și loader-ul oficial OpenXR Win32 pentru tracking. Versiunea acceptată inițial este `1.75.20108.14` pentru EXE, Engine.dll și Game.dll. El verifică versiunea și adresele callback-urilor din vtable înainte de instalarea observerilor. Dacă o verificare eșuează, scrie motivul în log și nu instalează hook-urile.



## Ce înregistrează



- Intrarea și ieșirea din `eCApplication::OnProcess`, `OnPreRender`, `OnRenderDraw`, `OnPostRender`, cu QPC, thread și contoare distincte pentru update și intrările OnPreRender.

- Camera activă: adresă, poziție, rotație și scară, FOV, aspect, clip depth, matrice view/projection.

- Stările camerei native: `first_person`, `temp_first_person`, `dialog_camera`. `null` înseamnă că nu s-a putut obține starea, de exemplu dacă nu există încă un camera AI valid în meniu.

- Device-ul D3D9: adresă, rezultat QueryInterface pentru D3D9Ex, adaptor, tip, behavior flags, viewport și descrierea render target-ului curent.

- Versiunile binarelor, Large Address Aware, prezența modulelor native PUP la inițializare, adresele și sloturile hook-urilor, numărul înregistrărilor pierdute.

- Tracking HMD: poziție în metri, quaternion wxyz, transformare device-to-standing și validitate. Metadate: model, sistem de tracking, rezoluție recomandată per ochi, origine și validitate (datele optice per ochi sunt încă null).



Valorile FOV/rotație sunt raportate în convenția SDK, fără o conversie presupusă în grade sau metri. Matricele sunt extrase prin `operator(row,column)` și scrise ca 16 valori în ordinea rândurilor. Render target-ul observat este cel activ în faza respectivă, nu neapărat backbuffer-ul final.



Observerii apelează callback-urile originale o singură dată. Controlul HMD pornește dezactivat. Când este activat explicit, modifică temporar rotația camerei și actualizează matricea view prin SDK; camera nativă este restaurată la sfârșitul cadrului. Interogările COM eliberează referințele pe care le obțin. Jocul încarcă DLL-ul prin mecanismul său ScriptInit.



## Proba controlului HMD



1. Conectează Quest 3 prin Virtual Desktop, cu VDXR, și ține casca activă pe cap. Rămâi în vederea desktop-ului.

2. Pornește Gothic 3 și încarcă o salvare, apoi intră în first-person nativ.

3. Privește drept înainte și apasă **Ctrl+Shift+F8**. Aceasta activează controlul și centrează prima mostră proaspătă disponibilă.

4. Apasă **Ctrl+Shift+F7**, apoi rotește lent capul stânga/dreapta, sus/jos și înclină-l lateral. Imaginea jocului trebuie să urmărească rotația capului; mouse-ul continuă să stabilească direcția de bază.

5. Privește drept și apasă **Ctrl+Shift+F11** pentru recentrare. Apasă **Ctrl+Shift+F6** pentru marker și snapshot.

6. Apasă **Ctrl+Shift+F8** din nou. Verifică revenirea la camera obișnuită și ieși normal din joc pentru analiza logului.



Pentru 0.4.2, verifică și mersul: ține W continuu, fără să miști mouse-ul, și întoarce lent capul aproximativ 60–90° stânga/dreapta. Traseul trebuie să urmeze privirea pe orizontală. Pornește o captură Ctrl+Shift+F7 în timpul mersului. Verifică A/D și S, apoi privește sus/jos în timp ce mergi. Recentrează cu Ctrl+Shift+F11 și verifică revenirea la mersul nativ după dezactivarea cu Ctrl+Shift+F8.



Corecția 0.4.1 intervine și în `gCCharacterControl_PS::OnAction` (slot 47), unde jocul formează vectorul dorit de mers, înainte de `ProcessMovements` (slot 85). Versiunea 0.4.0 intervenea doar în etapa ulterioară și logul probei arăta vectorul de mers aproape constant, deși privirea se rotea. Rotația orizontală este aplicată temporar în ambele etape, exclusiv pentru personajul jucătorului. OnAction păstrează referința mesajului, rezultatul original și restaurarea camerei inclusiv la excepțiile jocului. Filtrul selectează comenzile native înainte/înapoi/stânga/dreapta după identificator, indiferent de tastele configurate. Interpretarea mesajului se activează numai pentru hash-ul Game.dll verificat. Logul include contoare separate `action_calls`/`action_restores`, `session_key` și `action_direction`. Direcția finală vizibilă necesită proba în joc.



În caz de standby sau pierdere a tracking-ului, controlul se suspendă. După reconectare, privește înainte și apasă Ctrl+Shift+F11. Dacă există o nepotrivire de cameră sau o eroare SDK, controlul este dezactivat pentru acea sesiune; motivul apare în log.



Această probă verifică orientarea camerei în imaginea obișnuită de desktop. Randarea stereo și trimiterea imaginilor către ochii căștii rămân de implementat.



Hook-urile sunt instalate în ScriptInit, nu în DllMain. Intrările virtuale sunt găsite din thunk-urile generate de compilator pentru declarațiile SDK, apoi comparate cu exporturile Engine.dll încărcat. Pentru stub-urile importate din Game.dll verificăm și identitatea simbolului din tabela de importuri PE, tabela exportată gCGameApp și corespondența slotului în tabela de bază eCApplication. Un import poate fi interceptat de un mod; observerul apelează stub-ul original, păstrând ruta existentă. Codul funcțiilor nu este rescris. Dacă identitatea callback-ului nu poate fi verificată, pluginul refuză instalarea.



Scrierea JSONL rulează pe un worker separat. Producătorii folosesc o coadă fixă de 2048 sloturi cu try-lock; în caz de congestie se pierd înregistrări, în loc să se aștepte după disc în randare. Fișierul are limită configurabilă și este golit din buffer cel mult la aproximativ 250 ms în perioadele fără evenimente. O închidere forțată/crash poate pierde finalul logului. Sampler-ele pentru cameră și device rulează numai pe thread-ul primului OnProcess observat; dacă randarea e pe alt thread, fazele sunt înregistrate, dar acele snapshot-uri sunt omise. O eroare SEH în citirea datelor oprește sampler-ul afectat, fără a înghiți excepțiile din callback-ul original.



## Build și instalare



Necesită Visual Studio 2022 C++ BuildTools, componenta CMake și Windows SDK. Checkout-ul local SDK utilizat: `90bfd344de4510dda7ac9da7461cc7f1eac911f7`.



Din PowerShell:



```powershell

& 'D:\Gothic 3 VR\gothic3vr\tools\Build-Diagnostics.ps1'

```



Pentru build + teste + instalare, cu Gothic 3 închis:



```powershell

& 'D:\Gothic 3 VR\gothic3vr\tools\Build-Diagnostics.ps1' -Install

```



Instalarea copiază DLL-ul și configurația în:



```text

D:\SteamLibrary\steamapps\common\Gothic 3\scripts\Script_Gothic3VRDiagnostics.dll

D:\SteamLibrary\steamapps\common\Gothic 3\scripts\Script_Gothic3VRDiagnostics.ini

D:\SteamLibrary\steamapps\common\Gothic 3\scripts\Gothic3VRDiagnostics\openxr_loader.dll

```



Scriptul de instalare păstrează configurația existentă și face backup DLL-ului anterior în `gothic3vr\backups`, dacă există. DLL-ul este fixat în proces după inițializare pentru a păstra valide callback-urile. Schimbarea lui necesită repornirea jocului. Dezactivare: `Enabled=0` în INI și repornire; dezinstalare: eliminarea acestui DLL după închiderea jocului.



## Prima probă în joc



1. Pornește Gothic 3 și încarcă o salvare.

2. Mergi și rotește camera aproximativ 20–30 de secunde.

3. Apasă **Ctrl+Shift+F7** pentru o captură de 120 de update-uri cu toate intrările/ieșirile. Captura automată inițială este utilă și pentru meniul de pornire.

   Compară vederea third-person și first-person din joc: fă o captură și un marker Ctrl+Shift+F6 în fiecare mod. Controlul HMD poate fi activat numai în first-person nativ; pluginul nu schimbă modul ales de jucător.

4. Deschide inventarul, apoi pauza și un dialog, dacă este disponibil. Folosește **Ctrl+Shift+F6** în fiecare stare: adaugă un marker și cere snapshot-uri.

5. Dacă vrei să verificăm o anumită stare în detaliu, pornește o nouă captură Ctrl+Shift+F7 în acea stare.

6. Ieși normal din joc.



Combinațiile sunt citite doar când Gothic 3 este în prim-plan. Dacă OnProcess nu se execută într-o anumită stare de pauză, nici hotkey-ul nu poate fi citit acolo; pornește captura înainte de a intra în acea stare. Pluginul nu consumă tastele, deci eventuale bind-uri ale altor moduri pot primi aceleași combinații.



Logurile sunt create pentru fiecare proces în:



```text

D:\SteamLibrary\steamapps\common\Gothic 3\scripts\Gothic3VRDiagnostics\logs\diagnostic-YYYYMMDD-HHMMSS-PID.jsonl

```



Configurația implicită: sampling la 1000 ms, trace de 120 update-uri, maximum 16 MiB pe sesiune. După atingerea limitei apare `log_limit` și logging-ul se oprește pentru sesiunea respectivă.



Pentru rezumatul celui mai recent log:



```powershell

$log = Get-ChildItem -LiteralPath 'D:\SteamLibrary\steamapps\common\Gothic 3\scripts\Gothic3VRDiagnostics\logs' -Filter '*.jsonl' |

    Sort-Object LastWriteTime -Descending | Select-Object -First 1

& 'D:\Gothic 3 VR\gothic3vr\tools\Summarize-Diagnostics.ps1' -LogPath $log.FullName

```



Se generează un rezumat Markdown și unul JSON. Un log util trebuie să conțină patru înregistrări `hook`, evenimente `phase`, snapshot-uri `camera` și, dacă PostRender rulează pe thread-ul observat, `device`. Raportul evidențiază fazele fără pereche, linii incomplete și înregistrări pierdute. Timpii nu trebuie adunați între callback-uri care se includ unul în altul.



## Limitele acestei etape



Acesta nu este încă un mod VR și nu desenează stereo. Observăm callback-urile aplicației, nu toate draw-call-urile și nici fiecare invocare directă a `eCCameraBase::Render`. Testele native verifică decoderul thunk-urilor, convențiile thiscall/fastcall, apelarea originalului și restaurarea fără suprascrierea unui hook instalat ulterior. Compatibilitatea cu jocul/PUP și ordinea reală a fazelor se confirmă numai prin logurile unei sesiuni Gothic 3.



Licență: GPL-3.0-or-later, în concordanță cu Gothic 3 SDK. Vezi LICENSE. Nu au fost preluate asset-uri sau biblioteci Unity din VHVR.



## Corecția 0.1.1



Prima probă în joc a confirmat încărcarea DLL-ului 0.1.0, versiunile acceptate și prezența PUP, dar hook-urile au fost respinse înainte de instalare. Limita internă de 128 de sloturi era prea mică pentru eCApplication, care moștenește o ierarhie mare de ferestre. Limita de verificare este acum 512; fiecare intrare rămâne verificată separat în memoria procesului și față de exportul Engine.dll. Nu presupunem că tabela are 512 intrări.



Un test suplimentar compilează și decodează cele patru thunk-uri din declarațiile SDK cu aceleași opțiuni Release /O2 /GL ca pluginul. Confirmarea instalării în joc și a datelor camerei necesită o nouă probă cu 0.1.1. Camera first-person nativă este baza planificată pentru tracking-ul VR; randarea stereo rămâne de implementat.



## Corecția 0.1.2



A doua probă, cu 0.1.1, decodează toate sloturile SDK, dar respinge identitatea OnPreRender înainte de instalare. Inspecția fișierelor Engine.dll și Game.dll confirmă sloturile și importurile corecte ale celor patru callback-uri. Compararea adreselor finale nu este suficientă pentru o rută de import interceptată.



0.1.2 verifică simbolic importul FF25, identitatea tabelei exportate gCGameApp și slotul din tabela eCApplication. Păstrează implementarea deja apelată de joc prin importul respectiv. Logul include acum câte un `hook_validation` pentru toate cele patru callback-uri, cu adrese, module, bytes și metoda validării, inclusiv în caz de refuz. Testele verifică importuri intercalate, simbol/DLL greșit, limite de memorie și ținte neexecutabile. Validarea în joc a versiunii 0.1.2 rămâne necesară.



Tastele sunt acum Ctrl+Shift+F6 pentru marker/snapshot și Ctrl+Shift+F7 pentru trace, pentru a evita F9, care declanșează quickload în joc. Worker-ul scrie și contoarele la fiecare 5 secunde, independent de callback-urile jocului. Aceste înregistrări confirmă că logger-ul este activ chiar dacă un callback nu este apelat sau inițializarea hook-urilor este respinsă; nu reprezintă frame-uri de randare.



## Corecția 0.1.3: aplicația nativă aCProtoTypeApp



A treia probă identifică implementări native OnPreRender și OnPostRender în Gothic3.exe. Aplicația reală este aCProtoTypeApp, derivată din gCGameApp, apoi eCApplication. Declarațiile SDK descriu baza, fără aceste override-uri din executabil. Inspecția offline confirmă o singură tabelă cu toate cele patru adrese observate și ierarhia RTTI, cu baza eCApplication la offset zero.



0.1.3 acceptă această tabelă numai pentru executabilul cu SHA-256 `8D47A07E1158AFF941E7AF28215FC0605EF954956004BE937595CF8AFA5B98B1`. Profilul verifică RVA-ul tabelei, locatorul și numele RTTI, cele patru sloturi și adresele callback-urilor, plus corespondența cu tabela de bază Engine.dll. Adresele sunt relative la baza încărcată a EXE-ului. Aceste RVA-uri se aplică numai binarului verificat prin hash; sloturile continuă să fie derivate din SDK.



Observerii apelează implementările native originale; nu le înlocuiesc cu metodele clasei de bază. Un EXE diferit sau o tabelă modificată nu este acceptată de profil. Testele verifică și hash-ul SHA-256 cu un vector cunoscut, identitatea clasei, tabela, sloturile, adresele și protecția executabilă. Înregistrările esențiale de inițializare sunt puse în coadă fără pierderi cauzate de try-lock; callback-urile jocului păstrează logging-ul fără a aștepta lock-ul.



Tastele rămân Ctrl+Shift+F6 și Ctrl+Shift+F7. Proba de mai jos confirmă funcționarea versiunii 0.1.3.



## Proba în joc confirmată pentru 0.1.3



Sesiunea diagnostic-20261009-204343-14752 confirmă toate cele 4 hook-uri, 296 de snapshot-uri de cameră și 74 de device, fără pierderi sau erori ale sampler-elor. Ambele taste funcționează; first-person este detectat. Camera se actualizează între OnProcess și OnPreRender, iar snapshot-urile ulterioare păstrează aceleași matrice. Ordinea capturată este OnProcess → OnPreRender → OnPostRender, cu OnRenderDraw inclus în OnPostRender. Device-ul observat este D3D9Ex, cu target 2560×1440.



## 0.2.1: Quest 3 prin Virtual Desktop și VDXR



Ruta activă este Quest 3 → Virtual Desktop → VDXR → OpenXR Win32 → plugin. Aceasta înlocuiește backend-ul OpenVR/SteamVR din 0.2.0 în DLL-ul instalat. Nu trebuie pornit SteamVR pentru proba actuală.



Loader-ul oficial Khronos 1.1.63 este fixat în third_party/openxr; proveniența, hash-ul și licența sunt incluse. Biblioteca VDXR rămâne cea din instalarea Virtual Desktop a utilizatorului. Pluginul selectează manifestul Win32 virtualdesktop-openxr-32.json prin XR_RUNTIME_JSON numai în proces, pe durata inițializării, apoi restaurează valoarea anterioară. Nu modifică registrul OpenXR sau configurația OpenComposite. Calea este construită din ProgramW6432/ProgramFiles și instalarea standard Virtual Desktop Streamer/OpenXR; o instalare mutată în alt director necesită adaptarea descoperirii.



Sesiunea necesită XR_MND_headless și XR_KHR_win32_convert_performance_counter_time. Nu are graphics binding, swapchain-uri sau straturi stereo. Worker-ul poll-ează evenimentele și începe/oprește sesiunea după stările OpenXR. Creează spații VIEW și STAGE, cu fallback LOCAL când STAGE nu este suportat. Citește xrLocateSpace la timpul QPC convertit în XrTime, cu predicție solicitată zero. Poziția și orientarea trebuie să aibă flag-urile VALID; transformările sunt verificate pentru valori finite și rotații valide. Quaternionul OpenXR xyzw este convertit în reprezentarea diagnostică wxyz. Flag-urile OpenXR sunt păstrate în tracking_result, iar originea efectivă este scrisă în log.



Apelurile OpenXR rulează numai pe worker-ul de tracking, implicit la 11 ms. Snapshot-urile worker-ului sunt logate la 100 ms. OnPreRender.exit citește o copie cu try-lock și îi înregistrează vârsta, pentru corelarea cu camera. O mostră invalidă are poziție, quaternion și matrice null. Datele optice per ochi nu sunt încă citite și sunt null. Dezactivarea separată: Enabled=0 în [Tracking] și repornire.



Proba cu Quest 3:



1. Pornește Virtual Desktop Streamer, cu VDXR selectat, și conectează Quest 3 prin Virtual Desktop din cască.

2. Pornește Gothic 3, încarcă salvarea și activează vederea first-person nativă. Adu jocul în prim-plan pe PC.

3. Apasă Ctrl+Shift+F7, apoi Ctrl+Shift+F6. Rotește și deplasează capul puțin timp de 15–20 secunde. Ieși normal din joc.



Imaginea rămâne forma obișnuită a jocului pe desktop-ul Virtual Desktop. Versiunea 0.3.0 adaugă orientarea camerei la cerere; poziția căștii în metri nu este încă aplicată camerei și nu se trimite stereo. Logul trebuie să conțină vr_info pentru VirtualDesktopXR, vr_pose valide și mostre valide cu source=OnPreRender.exit. Vârsta mostrei nu reprezintă latența totală motion-to-photon.



Probă opțională fără Gothic 3:



```powershell

& 'D:\Gothic 3 VR\gothic3vr\tools\Test-Tracking.ps1' -Seconds 15

```



Scriptul rulează Gothic3VRVdxrProbe.exe și scrie logs/tracking-*.jsonl. Codul 0 confirmă cel puțin o mostră validă; codul 2 înseamnă lipsa tracking-ului valid.



Validare 0.2.1: build Release x86 și 6/6 teste CTest trecute, inclusiv conversia xyzw→wxyz, absența datelor inventate, loader-ul OpenXR Win32 și verificările existente pentru hook-uri/SDK/EXE. Sonda a identificat VirtualDesktopXR și suportul headless în runtime-ul instalat. Fără cască conectată, xrGetSystem a returnat XR_ERROR_FORM_FACTOR_UNAVAILABLE (-35), cod final 2 și zero mostre valide. Tracking-ul real și noul DLL în joc necesită proba de mai sus.



Referințe: [VDXR: headless și 32-bit](https://github.com/mbucchia/VirtualDesktop-OpenXR/wiki/Developers), [XR_MND_headless](https://registry.khronos.org/OpenXR/specs/1.0/man/html/XR_MND_headless.html), [loader Khronos 1.1.63](https://github.com/KhronosGroup/OpenXR-SDK/releases/tag/release-1.1.63).



## 0.2.2: ciclul headless necesar pentru tracking VDXR



Proba în joc cu 0.2.1 (diagnostic-20261009-212523-8436) confirmă 4 hook-uri, 196 snapshot-uri de cameră, 49 de device, first-person și identificarea Meta Quest 3 / VirtualDesktopXR. Totuși, toate cele 683 de mostre HMD sunt identitate, deși runtime-ul setează VALID/TRACKED. Utilizatorul confirmă casca purtată și mișcată în desktop-ul Virtual Desktop. Acea probă nu confirmă tracking-ul mișcării.



Comparația cu casca reconectată izolează ciclul de frame-uri: sonda fără xrWaitFrame/xrBeginFrame/xrEndFrame returnează 29 de mostre identice; sonda cu acest ciclu returnează 147 de mostre distincte, cu intervale de poziție 0,163 / 0,123 / 0,123 metri pe X/Y/Z și variație de orientare de până la 86,266 grade față de prima mostră. Tracking-ul live este confirmat prin sonda separată, în configurația actuală VDXR.



Clientul 0.2.2 execută acum ciclul headless înainte de citirea poziției. xrEndFrame are mereu zero straturi și folosește timestamp-ul din xrWaitFrame. Interogarea poziției rămâne la timpul QPC curent, fără predicție solicitată. Ciclul rulează exclusiv pe worker-ul VR, fără să blocheze callback-urile jocului. În caz de eroare a ciclului, sesiunea este distrusă și se încearcă reconectarea; mesajul include operația și eroarea. Nu se trimit texturi Gothic 3. VDXR își gestionează intern resursele grafice necesare sesiunii headless.



Analizorul raportează acum și numărul transformărilor distincte, intervalul poziției, variația orientării, head_motion_observed și identity_pose_only. Nu interpretează pose_valid ca dovadă suficientă a mișcării. Testele analizorului includ o captură doar cu rotație de 90 de grade.



0.2.2: 7/7 teste CTest trecute. Testul nou verifică ordinea Wait/Begin/End, timestamp-ul, zero straturi chiar când shouldRender este true și oprirea corectă după erori. Pluginul corectat este instalat; este necesară o probă în joc pentru corelarea tracking-ului live cu camera.



Proba rămâne: casca pe cap și conectată prin Virtual Desktop/VDXR, Gothic 3 în first-person, Ctrl+Shift+F7 și Ctrl+Shift+F6, mișcări ale capului 15–20 secunde, apoi ieșire normală. Casca trebuie să rămână activă pe durata probei; standby-ul o deconectează în configurația utilizatorului. Nu trebuie pornit SteamVR.



## Proba în joc confirmată pentru 0.2.2



Sesiunea diagnostic-20261009-213836-18164 confirmă 4 hook-uri, 260 de snapshot-uri de cameră, 65 de device și 899 de mostre HMD valide, fără înregistrări pierdute, faze fără pereche sau erori de probe. Există 731 de transformări HMD distincte, cu variații de poziție 0,194 / 0,157 / 0,194 metri și orientare până la 59,593 grade față de prima mostră. Runtime-ul este Meta Quest 3 / VirtualDesktopXR, originea stage, target-ul recomandat 2660×2884 per ochi.



281 de mostre HMD sunt înregistrate la OnPreRender.exit; 64 de update-uri au și snapshot de cameră în aceeași fază, dintre care 32 în first-person. În intervalul de după trace-ul manual, vârsta copiei HMD la callback are medie 7,242 ms, P95 14,575 ms, maxim 19,690 ms. Maximul sesiunii, 109,085 ms, apare la inițializare; mostrele de peste 50 ms sunt toate în primele 0,575 secunde. Aceste valori nu sunt latența totală a căștii.



În proba cu 0.2.2, diagnosticul și tracking-ul sunt confirmate simultan în joc; acea versiune nu modifică încă camera.



## 0.3.0: orientarea relativă a camerei



Orientarea relativă este inverse(center) * head, în quaternion, apoi schimbată în convenția +Z forward a camerei Genome prin S*R*S, S=diag(1,1,-1). Se compune peste orientarea nativă din acel cadru; recentrarea folosește toate cele trei axe. Poziția, camera AI și ținta privirii personajului nu sunt modificate.



Inspecția offline a Engine.dll confirmă că DoRender actualizează camera, apelează OnPreRender, apoi desenează scena cu eCCameraBase::Render, care recalculează view din rotație. Prin urmare, aplicarea are loc după callback-ul original OnPreRender, cu SetRotation și UpdateViewport(true). Conversia Euler este verificată împotriva matricelor înregistrate în joc, inclusiv o captură first-person reală, iar testele acoperă yaw la ±90° și rotații dincolo de ±90°.



Rotația nativă este salvată înaintea aplicării și restaurată după callback-ul original OnPostRender, după draw-ul inclus în el. Există restaurare de rezervă înainte de OnProcess/OnPreRender dacă un cadru anterior nu a apelat PostRender. Restaurarea funcționează și când logger-ul se oprește. Dacă jocul a schimbat camera sau rotația în interval, pluginul refuză să suprascrie acea schimbare și dezactivează controlul.



Aplicarea necesită joc activ, fereastră în prim-plan, first-person nativ fără mod temporar/dialog, cameră perspective cu scară unitară, mostră VALID și ORIENTATION_TRACKED, quaternion finit și vârstă de cel mult 50 ms. [HeadControl] Allowed=0 dezactivează această funcție; MaxPoseAgeMs acceptă 20..200 ms. Controlul pornește OFF la fiecare lansare.



Evenimentele head_control înregistrează activarea, recentrarea, suspendarea, numărul cadrelor aplicate/restaurate și, la snapshot/trace, rotația nativă, rotația solicitată, matricea view citită după aplicare/restaurare și eroarea rotației față de cea calculată. Verificarea internă acceptă cel mult 0,001 pe elementul matricii de rotație. O matrice corectă în log nu demonstrează singură efectul vizual; acesta necesită proba în joc.



0.3.0: build Win32 Release și 8/8 teste CTest trecute. Controlul camerei necesită prima probă în joc cu această versiune. Verifică separat direcțiile rotației, absența acumulării, recentrarea și revenirea la mouse după dezactivare.



## 0.3.1: recentrare pe Ctrl+Shift+F11



Recentrarea este mutată de pe F5, care declanșează quicksave în Gothic 3, pe Ctrl+Shift+F11. Instalarea actualizează și comentariul configurației existente. Ctrl+Shift+F8 rămâne pentru activare/dezactivare; F6 pentru marker și F7 pentru trace, în combinație cu Ctrl+Shift.



## Proba 0.3.1 confirmată în joc



Utilizatorul confirmă rotația vizibilă a camerei. Sesiunea diagnostic-20261009-220423-1888 conține 17.099 de aplicări și 17.099 de restaurări, 228 de snapshot-uri aplicate/restaurate și eroare maximă a rotației view de 1,79e-7. Direcția de mers a rămas cea nativă: integrarea anterioară modifică doar camera la randare, după procesarea mișcării.



## 0.4.0: mers după privirea HMD



Inspecția Game.dll confirmă că gCCharacterControl_PS::OnPreProcess apelează virtual ProcessMovements, iar această metodă citește inversa matricii view pentru cadrul de referință Camera. Noua integrare folosește acel callback numai pentru proprietatea CharacterControl a jucătorului activ, pe thread-ul aplicației, cu tracking valid și camera first-person. NPC-urile continuă prin original fără modificări.



În timpul ProcessMovements, camera primește temporar direcția orizontală a privirii compuse din mouse și HMD, fără pitch sau roll. Jocul calculează în continuare tastele, mersul lateral/înapoi, vitezele și mișcarea. Camera de bază este restaurată imediat după consumarea direcției, inclusiv când originalul propagă o excepție. Poziția și transformarea entității nu sunt scrise direct. În timpul randării se aplică în continuare rotația completă HMD.



Privirea aproape verticală păstrează ultima direcție orizontală validă. Pauza, modul de control Target și privirea nativă înapoi suspendă această integrare. [Locomotion] HeadDirected=0 păstrează mersul nativ cu privire VR independentă. Controlul folosește aceeași activare Ctrl+Shift+F8 și recentrare Ctrl+Shift+F11.



Hook-ul de mișcare este validat prin hash-ul exact al Game.dll `4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00`, tabela exportată gCCharacterControl_PS, exportul ProcessMovements și slotul 85 derivat din declarația SDK. O nepotrivire refuză integrarea de mișcare și păstrează tracking-ul camerei.



Logul include movement_applied, movement_direction și movement_restored, contoare separate pentru mișcare și direcția dorită world din getterul SDK GetWishedDirection. Aceste date descriu comenzile de mers, nu deplasarea efectivă după coliziuni.



Build Win32 Release: 9/9 teste CTest trecute, inclusiv 11 verificări pentru mers și restaurare, plus validarea celui de-al cincilea thunk SDK. 19 importuri în DLL-urile jocului sunt prezente. Comportamentul mersului cu 0.4.0 necesită proba în joc.



## 0.4.2: orientarea directă a vitezei orizontale



Proba 0.4.1 confirmă 6.316 comenzi OnAction și 9.046 procesări ProcessMovements, cu restaurări pereche. Vectorul cerut și direcția camerei randate coincid pentru comenzile înainte, dar utilizatorul raportează că navigarea rămâne deficitară. Analiza nativei ProcessControledTranslation arată că jocul poate interpola între ținta nouă și ținta precedentă înainte de aplicarea vitezei.



0.4.2 adaugă un context limitat la OnProcess al proprietății CharacterMovement a jucătorului (slot 73). În acel context, SetLinearVelocity al rigid body-ului jucătorului (slot 92) orientează viteza orizontală nativă către vectorul de mers obținut din ProcessMovements. Magnitudinea orizontală și componenta Y se păstrează; viteza zero rămâne zero. Contextul este curățat inclusiv la excepțiile originalului. Fizica primește viteza prin callback-ul original, păstrând procesarea coliziunilor. Nu se scrie poziția entității.



Integrarea se activează numai pentru comanda nenulă din update-ul curent, HMD proaspăt și first-person nativ. Este limitată la sneak/walk/run/sprint/wade și se suspendă pentru mișcare montată, mișcare de luptă din SPU, salt, cădere, înot și ragdoll. Configurația [Locomotion] DirectVelocity=0 revine la comportamentul 0.4.1. HeadDirected=0 păstrează mersul nativ complet.



Hook-urile suplimentare sunt validate prin thunk-urile SDK, exporturile și vtable-urile reale. Profilul Game.dll rămâne același; Engine.dll trebuie să aibă hash 991CF674A9CF1BC60CDDC204BE0993E8251336F4D3471543A37846CC6840BF5A. Dacă verificarea sau instalarea eșuează, toate cele patru hook-uri de mers sunt refuzate/restaurate.



Evenimentele locomotion_velocity înregistrează viteza nativă, viteza solicitată și direcția dorită în world space. Acestea confirmă cererea către fizică, nu deplasarea după coliziuni. Session key este acum separat de flag-ul de readback al camerei; în 0.4.1 flag-ul suprascria identificatorul din log, fără să afecteze filtrarea comenzilor din cod.



Build-ul Win32 Release trece 9/9 teste CTest, inclusiv 23 verificări de locomotion și 8 thunk-uri SDK. Cele 22 importuri native sunt prezente. Efectul vizibil și comportamentul lângă obstacole necesită proba în joc cu 0.4.2.

















