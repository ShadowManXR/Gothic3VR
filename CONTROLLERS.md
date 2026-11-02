# Controls — 0.23.0

- X: inventory; B: draw/sheathe; left Menu: main menu/back.
- R3: radial, right stick: select, A: confirm, B: page, X: cancel.
- Left trigger: use a native health potion from inventory, once per press.
- L3: Sprint until movement stops; double Y: Sneak.
- Right grip: native block with shield/weapon raised to chest level. Blue shield icon and a short vibration confirm native block.
- Left grip: support weapon; L3+R3 held: recenter.

# Native Sneak and Run/Sprint — 0.22.5

L3 tap toggles Run/Sprint. Double-tap Y toggles Sneak. Draw/sheath in Sneak follows native refusal. R3 radial, left stick/A selection, Menu and left-grip support remain. 71/71 tests pass; headset acceptance pending. See FIXES-0.22.5.md.

## Previous release notes

# Revised controls — 0.22.4

L3 tap: Sprint on/off. Double-tap Y: Sneak on/off. Left grip: weapon support. Left Menu: menu/back. R3: radial; left joystick selects; A confirms; B cancels; Y changes page. L3+R3 held0.5s: camera reset. 71/71 tests and15 launcher pages pass; live acceptance pending. See FIXES-0.22.4.md.

## Previous release notes

# Revised controls — 0.22.3

Y: Sneak toggle. Left grip + joystick: native Sprint. Left Menu: menu/back. R3: radial; left joystick selects, A confirms, B cancels, Y changes page. L3+R3 held 0.5 s: camera reset. Lockpick: one guess per deflection, both axes at centre for 100 ms before the next. See FIXES-0.22.3.md.

## Previous release notes

# Native Sneak and interaction controls — 0.22.2

Left grip + movement invokes logical native Sneak/Down, independent of keyboard bindings; grip release exits. Skill/support restrictions remain. The eye icon follows native Sneak state. Non-GUI interactions also receive left-stick A/D: centre on entry, deflect for one step, centre to repeat. See FIXES-0.22.2.md.

## Previous release notes

# Lockpick directional steps — 0.22.1

Left joystick sends A/D while a menu/puzzle is open, independently of native lockpick routine detection. Centre once on entry; deflect left/right for one step, centre to repeat. Holding does not repeat guesses. Right pointer/trigger and Back remain. See FIXES-0.22.1.md.

## Previous release notes

# Current controls — 0.22.0

Left joystick has a 35-degree forward/back cone with 25-degree strafe exit. In menus it sends left/right arrows; in native lockpick action/task it also sends the configured game left/right bindings. Return the stick to neutral on entry. Finisher: drawn melee + eligible downed target, right grip and trigger held 0.8 seconds; native third-person during execution, calibrated first-person return. Sneak: left grip + movement with a free hand; small eye low in view. See BATCH-0.22.0.md.

## Previous release notes

# Defence controls — 0.21.0

Hold left trigger: native shield block or 1H/2H parry outside dual wield. Dual wield keeps left-trigger power. Shield page saves palm/angle calibration live. Y skips native movies after a neutral sample. Native locomotion unchanged. See BATCH-0.21.0.md.

## Previous release notes

# Current native controls — 0.20.1

Left stick: native Run. R3 + forward: native Sprint. Left grip + movement: skilled native Sneak with a free left hand; weapon support wins. Y: main menu/Back. Direct locomotion dependencies are removed; SNEAK badge/vibration remain. See FIXES-0.20.1.md.

## Previous release notes

# Current controls — 0.20.0

Y: main menu / Back. Left grip held + movement: Sneak with a free left hand and native skill; release exits. Native weapon support takes priority. R3 + forward: Sprint. Normal joystick movement uses Run, with no Walk. Radial: L3, right stick, A/B, Y page. Camera reset: L3 + R3 for 0.5 seconds. Animal return: X + Y. SNEAK badge and 160 ms native-state vibration requests; headset delivery pending. See BATCH-0.20.0.md.

## Previous release notes

# Current controls — 0.19.1

Y toggles native Sneak in gameplay, with SNEAK badge and state-change vibration; Y retains Back/page switching in menus/radial. Hold R3 + forward for native sprint. The radial utility page has eight actions, removing Sneak and Walk / Run. Launcher Radial page adjusts size live (80% default). For finishing, draw a melee weapon, point at a downed NPC until FINISH appears, then hold right grip + trigger for 0.8 s after releasing buttons. See FIXES-0.19.1.md.

## Previous release notes

# Current controls — 0.19.0

L3 toggles the radial, right stick selects, A confirms, B cancels and Y changes page. Utility page adds Sneak and Walk / Run. Hold R3 with left-stick forward for native stamina-consuming sprint. L3 + R3 camera reset takes priority. Hold the equipped melee hand trigger until the ready vibration, then physically strike for power attack; right grip + trigger held 0.8 s on a focused downed NPC requests the native finishing animation. See BATCH-0.19.0.md for skill, stamina and cancellation rules.

## Earlier release notes

# Current behavior — 0.16.4

Animal form: hold X + Y on the left controller for 0.5 seconds, outside menus/radial, to return human and recenter. Crossbow: right trigger fires; left grip at the foregrip enables stable left-guided aim. One-hand aim has visible sway. See FIXES-0.16.4.md.

## Previous release history

# Quest 3 controllers — 0.16.1

0.16.1 resolves the actual compiled hero Cock alias (2.8 seconds in this installation), fixes mixed quaternion/vector motion tracks, moves and reveals casting items before culling, keeps transformed creatures in native third-person VR, doubles unsupported 2H rotation speed, and preserves native melee progression bonuses instead of clipping them to weapon base damage. Full controller reach was accepted by the user in 0.16.0. See FIXES-0.16.1.md and PROGRESSION-AUDIT.md. New fixes await headset validation.


Physical bow: equip a bow using the radial. Bring the right hand within 20 cm of the left grip, hold the right trigger, pull back at least 12 cm, aim along the two hands, and release to shoot. Full draw defaults to 50 cm. A trigger held while equipping cannot start a draw; release before grabbing. Menus, radial, recovery, tracking loss, weapon switches and camera reset cancel. Use the Bow page to adjust grip orientation, full draw length and grab radius; Save settings applies live and cancels any current draw. Native trigger attacks/zoom are suppressed only for a drawn bow during tracked VR, while menu clicking, knockdown recovery and other ranged weapons retain their normal bindings. 1H Melee Weapon replaces the Sword tab; one-handed sword combat was confirmed by the user in 0.10.2. See PHYSICAL-BOW.md. The following notes describe earlier releases.

Drawn melee weapons automatically use a forward grip, measured from the actual weapon attachment and mesh bounds. Adjust Sword pitch/yaw/roll in the Sword launcher tab, separately from Left hand / Right hand calibration. Both the hand and attached sword follow that pose. Consecutive wrist-only swings rearm on angular reversal; vertical/horizontal strokes have the same thresholds. Physical close contact uses actual blade capsule overlaps plus swept rays. Headset confirmation is pending; recovery and device calibration were confirmed in 0.10.1.

During knockdown/get-up, both trigger commands pass to native recovery and the arm/body pose override is suspended. After recovery, tracked melee resumes. Left hand and Right hand launcher tabs calibrate each device grip's position/orientation live; the default outward offset is 5 cm per hand. See HAND-CALIBRATION.md. Sword damage now scales the final native HP delta, including OnDamage's recalculation.

Draw the sword using the radial and swing the right controller through the target. Damage depends on the stroke, capped at the weapon's damage. Triggers no longer attack with a drawn melee weapon in VR; sheathe it to interact with objects. Menu trigger clicks and radial controls keep working. See PHYSICAL-COMBAT.md. Bow drawing/release remains native pending the physical bow implementation. The notes below document earlier releases.

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

**Proba 0.9.0:** opțiunea **Tracked arms and weapons (experimental)** aplică IK brațelor native ale jucătorului și mută temporar armele la mâinile urmărite în 6DoF. Sabia folosește dreapta, arcul stânga. Necesită proba în cască pentru identificarea oaselor, vizibilitate și aliniere; 33/33 teste automate trec. Damage-ul din balansarea sabiei și întinderea fizică a corzii nu sunt încă implementate; triggerul continuă să comande atacul nativ în această probă. Se poate reveni la brațele native dezactivând opțiunea și salvând în timpul jocului.

Interfața radialului și a launcherului este în engleză în 0.8.3. Utilizatorul a confirmat radialul și selecția în proba 0.8.2; logurile arată comenzi native confirmate și închiderea normală a motorului. Comenzile și setările existente sunt păstrate.

**Radial după încărcarea salvării:** apasă o dată **L3**, înclină joystickul drept spre opțiune și apasă **A**. **B** sau o nouă apăsare L3 anulează. Nu trebuie să ții joystickul apăsat; revenirea lui în centru păstrează opțiunea aleasă. Cele zece sectoare pornesc cu 1 în partea de sus și continuă în sens orar până la 0. **Y** schimbă pagina între sloturile rapide și cele opt comenzi: inventar, personaj, jurnal, hartă, magie, armă, ia tot și înapoi. Sloturile folosesc `SessionKey.QuickUse1` până la `QuickUse9`, apoi `QuickUse0`, inclusiv remapările existente. Apăsarea confirmată este ținută 80 ms și eliberată automat; pierderea focusului o eliberează imediat. În inventar, indică mai întâi obiectul cu cursorul, apoi deschide radialul pentru a trimite o tastă rapidă. Cursorul rămâne în ultima poziție în timpul selecției, iar interfața nativă decide atribuirea/folosirea slotului.

În proba 0.8.1, radialul se anula după redesenări de aproximativ 235 ms: mostra controllerelor expira la 100 ms. În 0.8.3 geometria este calculată o singură dată la conectarea VR; actualizările reutilizează masca. O întârziere grafică suspendă comenzile native, păstrând radialul, selecția și muchiile butoanelor până la următoarea mostră proaspătă. Pierderea reală a focusului, schimbarea meniului, ieșirea din lume și resetarea camerei anulează în continuare radialul. R3 singur nu anulează, iar A fără selecție lasă radialul deschis. Testele includ reluarea exactă a întârzierilor din proba live și compararea tuturor pixelilor geometriei. Radialul a fost confirmat în cască în proba 0.8.2.

L3+R3 anulează radialul și păstrează combinația de resetare. Mersul, rotirea și atacul sunt suspendate în timpul selecției. Schimbarea modului de meniu sau pierderea trackingului anulează alegerea. Înainte de încărcarea unei lumi, radialul este dezactivat. Panoul radial are propria dimensiune și poziție în VIEW, independent de scalarea meniurilor și de rezoluția 4K/5K. Imaginea este actualizată numai la schimbarea sectorului sau paginii. Cele 31 de teste automate trec; radialul a fost confirmat în cască în 0.8.2.

Țintirea și interacțiunile cu controllerul drept au fost confirmate în 0.7.4 pe Quest 3 / Virtual Desktop / VDXR. Utilizatorul a raportat în 0.7.5 înclinare verticală după resetare pe pante, apoi a confirmat pe 10 octombrie 2026 că resetarea este corectă pe pantă în 0.7.6. Logul probei conține trei cereri L3+R3, fiecare aplicată în următorul cadru stereo, la 4–7 ms după cerere; resursele sunt eliberate înaintea închiderii motorului. Cursorul stabilizat și eliminarea laserului nu au încă o confirmare explicită.

**Corecția 0.7.6:** actualizarea mersului nu mai poate consuma recentrarea înaintea cadrului stereo. Acea cale folosea rotația completă a căștii și omitea actualizarea originii poziției. Cadrul stereo aplică acum resetarea completă și, cu Orizont stabil activ, folosește numai yaw-ul ca orientare a originii. Înălțimea relativă la personaj și urmărirea fizică a capului se păstrează. Testul reproduce resetarea cu cap înclinat și poziție deplasată, urmată de actualizarea mersului înaintea randării stereo.

În first-person, selecția obiectelor și personajelor urmărește poziția și orientarea **aim** a controllerului drept. Îndreaptă controllerul spre personaj sau obiect și apasă triggerul drept. Numele și evidențierea native arată ce entitate este selectată. Scanarea păstrează distanțele, filtrele, prioritățile și verificările de vizibilitate ale Gothic 3. **Laserul vizibil a fost eliminat în 0.7.5**, la cererea utilizatorului.

**Resetarea camerei din joc:** privește înainte și apasă simultan joystickul stâng și joystickul drept (click **L3 + R3**) timp de **0,5 secunde**. Ambele butoane trebuie apoi eliberate înainte de următoarea resetare. Comenzile de mers, rotire, atac și click sunt eliberate cât timp se apasă combinația. Trackingul mai vechi de 100 ms sau pierderea focusului anulează combinația. Tastatura păstrează **Ctrl+Shift+F11** pentru aceeași recentrare.

Resetarea memorează din nou poziția completă și yaw-ul căștii, anulând deplasarea trackingului față de camera personajului. Păstrează înălțimea calibrată a personajului, înălțimea/offsetul înainte din launcher și înclinarea reală a capului când Orizont stabil este activ. Nu recalibrează camera dintr-o tranziție de dialog. Conversațiile păstrează camera nativă. Evenimentul `XrEventDataReferenceSpaceChangePending` pentru LOCAL declanșează recentrarea automat la primul cadru valid cu `predictedDisplayTime >= changeTime`, inclusiv când runtime-ul nu furnizează transformarea față de originea anterioară. Referință: [specificația OpenXR](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrEventDataReferenceSpaceChangePending.html). Logurile includ `local_reference_change_pending`, `local_reference_changed`, `vr_camera_reset_requested` și `vr_camera_reset_applied`.

**Cursor stabilizat în meniuri:** filtrul adaptiv netezește tremurul fin și reacționează mai repede la mișcări intenționate. Folosește coordonate normalizate ale panoului, independent de rezoluție. La apăsarea triggerului păstrează punctul deja indicat timp de 100 ms; apoi permite tragerea cu triggerul ținut. Intrarea într-un meniu, schimbarea geometriei panoului, ieșirea pointerului din panou, recentrarea și pierderea trackingului resetează filtrul. Țintirea din lume rămâne directă, fără această filtrare.

Conversia controllerului folosește aceeași bază nativă, recentrare, înălțime și offset înainte/înapoi ca ochii stereo. Poziția controllerului este originea razei de selecție. Hook-ul verifică SHA256 Game.dll și prologurile `FindFocusEntity` / `GetAngles(vector)` înainte de instalare. Doar focusul jucătorului este modificat. Camera temporară a query-ului este restaurată inclusiv la propagarea unei excepții native; NPC-urile și mersul nu primesc direcția controllerului. În meniuri/conversații rămâne pointerul pentru panou. Pierderea focusului, trackingul mai vechi de 100 ms sau dezactivarea controllerelor restabilesc selecția nativă. Logul `controller_world_focus` include ținta, query-uri, calcule de unghiuri și restaurări.

În 0.7.3, dialogurile și recentrarea căștii păstrează calibrarea înălțimii personajului. Revenirea în first-person nu folosește poziția ridicată a camerei native din tranziția de conversație. Testele includ revenirea dintr-o cameră ridicată, deplasarea pe teren după dialog și resetarea la ieșirea din lume/dezactivarea stabilizării.

Fila **Cameră și control** include **Cameră înainte / înapoi** (−100…100 cm, pozitiv = înainte), **Viteză rotire joystick** (10…2000%, implicit 500%) și **Orizont stabil** (implicit activ). Camera păstrează și reglajul separat de înălțime. Setările se salvează și se aplică în timpul jocului. În 0.7.2, intervalul extins permite viteze mai mari decât maximul anterior de 200%; 800% produce de patru ori mișcarea orizontală a mouse-ului de la 200%. Logul `vr_camera_settings` confirmă valoarea aplicată.

Orizontul stabil elimină pitch/roll-ul nativ în first-person și înlocuiește balansul vertical al capului animat cu înălțimea relativă la poziția personajului. Săriturile, pantele și deplasarea reală a căștii rămân prezente. Recentrarea folosește numai yaw-ul căștii, astfel încât o înclinare a capului la recentrare să nu încline lumea. Conversațiile păstrează camera nativă. Offsetul înainte/înapoi se aplică orizontal în direcția camerei native, identic pentru ambii ochi.

Conectează casca prin Virtual Desktop, cu runtime-ul VDXR, și activează ambele controllere. Bifa **Controllere Quest / Touch** din launcher activează comenzile; modificarea se aplică după salvare și în timpul jocului. Configurația este `[Controllers] Enabled=1` în `scripts\Script_Gothic3VRDiagnostics.ini`.

| Comandă | În lumea jocului | În meniuri și conversații |
| --- | --- | --- |
| Joystick stâng | Mers înainte/înapoi și lateral | — |
| Joystick drept, stânga/dreapta | Rotire orizontală cu viteză reglabilă | — |
| Ambele joystickuri apăsate, 0,5 secunde | Reset camera VR | Reset camera VR |
| Trigger drept | Acțiunea principală: interacțiune/atac | Click la poziția indicată de controllerul drept |
| Trigger stâng | Acțiunea secundară | — |
| A | Săritură | — |
| B | Inventar | Inventar |
| X | Scoate/ascunde arma | — |
| Y | Escape | Înapoi/închide |

Pointerul intersectează panoul OpenXR folosind poziția și orientarea controllerului drept. Respectă dimensiunea, distanța și offsetul meniurilor din launcher, inclusiv la 4K/5K. Cursorul jocului indică locul selecției în meniuri. Proba 0.9.0 folosește brațele și mâinile originale Gothic 3 în first-person, nu modele separate de mâini.

Comenzile de joc folosesc tastele din `Documents\Gothic3\UserOptions.ini` (sau copia temporară `__UserOptions.ini` în timpul pornirii). Prima asociere de tastatură/buton de mouse acceptată este preferată. Joystickul stâng produce apăsări digitale cu deadzone 0,25. Rotirea folosește mișcare relativă de mouse numai pe X, cu deadzone 0,4 și intensitate progresivă; 100% înseamnă 600 unități mouse pe secundă la deflecție maximă. Viteza efectivă depinde și de sensibilitatea nativă a mouse-ului. Nu se adaugă mișcare pe Y. Damage-ul rămâne cel nativ, comandat prin buton; proba 0.9.0 adaugă numai poziționarea vizuală a brațelor și armelor din controllere.

Apăsările sunt eliberate la pierderea focusului jocului/runtime-ului, dezactivarea VR/controllerelor și la închiderea sesiunii. Datele mai vechi de 100 ms nu produc comenzi. După schimbarea meniului, butoanele de acțiune și triggerele trebuie eliberate înainte de o nouă apăsare, pentru a evita selectarea accidentală a unei opțiuni de conversație. Tastatura și mouse-ul rămân disponibile.

OpenXR creează un action set, opt acțiuni, 16 bindings pentru `/interaction_profiles/oculus/touch_controller` și spații grip/aim independente pentru fiecare mână. Pozițiile și orientările grip sunt citite în LOCAL, iar aim și în VIEW pentru intersecția cu panoul. Logul stereo include `quest_controller_actions_ready` și mostre `quest_controller_poses`; logul de diagnostic include joystickuri, triggere, butoane și coordonatele pointerului.

Cele 30 de teste automate includ bindings OpenXR și spații pentru ambele mâini, pierderea focusului/trackingului, intersecția pointerului cu marginile panoului, tranziții de meniu, apăsări/eliberări și salvarea opțiunii din launcher. Verifică țintirea din originea mâinii, obiectele pe sol, rotirea corpului, ABI-ul hook-urilor, resetarea după deplasări de câțiva metri, păstrarea înălțimii, ordinea temporală a evenimentelor OpenXR, tremurul cursorului, răspunsul la mișcare intenționată, clicuri, tragere și combinația L3+R3. Proba în cască se validează separat.





Optional item locator: hold left trigger and tap X to toggle markers, after importing/enabling original eCDELocator 1.0.7 in Optional mods. X alone remains inventory; double left trigger remains quick heal. Menus and activities block this shortcut.

Local 0.23.99 declares additional OpenXR controller profiles as unvalidated hardware compatibility candidates. Index A/B and Generic primary/secondary map to the same logical controls as Touch; Steam Frame uses Valve's Touch-compatible face-button mapping. Main menu remains available in the radial on controllers without an application menu button. See [Controller compatibility](docs/CONTROLLER-COMPATIBILITY.md).

Local 0.23.97: right-stick horizontal input can orbit the camera around the hero in third-person conversations, activities and knockdowns. Physical headset crouching lowers the visual body and hides it except for hands during very deep crouches. Launcher 0.23.98 keeps these validated features and controller input enabled without exposing switches; existing tracking calibration is preserved. See [Camera orbit and physical crouching](docs/CAMERA-ORBIT-PHYSICAL-CROUCH.md).
