# Hand calibration —0.22.10

Left hand / Right hand explain palm-centred rotation and explicit controller-local position offsets. All controls/settings remain available. 75/75 tests pass; headset acceptance pending. See FIXES-0.22.10.md.

## Previous release notes

# Weapon handling controls —0.22.9

1H/2H/Staff pages include Weight and Stabilization. Both0 = direct; Save settings applies live. Existing settings/calibration are preserved. 74/74 tests pass; headset acceptance pending. See FIXES-0.22.9.md.

## Previous release notes

# Controller hints — 0.22.4

Motion, Radial and footer hints now show L3 Sprint toggle and double-tap Y Sneak. Existing settings are preserved. 71/71 tests and15 launcher pages pass; live acceptance pending. See FIXES-0.22.4.md.

## Previous release notes

# Velocity limits and revised controller hints — 0.22.3

All melee swing/stab speed sliders extend to 800 cm/s in steps of 10. Existing settings are preserved. Updated Motion/Radial/footer hints show Y toggle, left grip Sprint, Menu Back and R3/left-stick radial. See FIXES-0.22.3.md.

## Previous release notes

# Minimum stab speed — 0.22.2

1H Melee Weapon, 2H Melee and Staff / Halberd each have separate stab speed/distance settings. Default stab speed 50 cm/s, range 40–400, step 10. Staff uses two columns; user calibrations and swing thresholds are retained. See FIXES-0.22.2.md.

## Previous release notes

# Stab distance — 0.22.1

Stab distance minimum appears in1H Melee Weapon,2H Melee andStaff/Halberd, separately from swing damage/stamina distance. Default15cm; range5–100cm in5cm steps. Save settings applies live and preserves previous profile values. See FIXES-0.22.1.md.

## Previous release notes

# Swing speed settings — 0.22.0

Minimum swing speed appears in 1H Melee Weapon, 2H Melee and Staff / Halberd. Default 100 cm/s, range 50–400, step 10. Save settings applies live. Existing independent damage/stamina distance and hand calibration settings remain. See BATCH-0.22.0.md.

## Previous release notes

# Shield page — 0.21.0

15 pages; Shield has pitch/yaw/roll and three palm offsets. Save settings applies live independently of hand calibration. Existing key identities preserved. See BATCH-0.21.0.md.

## Previous release notes

# Current launcher — 0.20.1

Motion contains Instant turn response only; Direct VR locomotion and Movement start response controls are removed. Fourteen pages, 58 numeric settings. Radial and per-category swing keys preserve user values. Saving cleans obsolete direct motor keys; the runtime ignores them regardless. See FIXES-0.20.1.md.

## Previous release notes

# Current launcher — 0.20.0

Fourteen pages and 59 numeric settings. 1H Melee Weapon, 2H Melee and Staff / Halberd each have Damage swing minimum and Stamina swing minimum (10–200 cm in 10 cm steps), saved live. Motion keeps current user settings. Radial size remains configurable independently of HUD/native menus. See BATCH-0.20.0.md.

## Previous release notes

# Current behavior — 0.16.4

Magic now includes three independent grip-contact sliders, alongside shared Aim stabilization and the reticle toggle. Default forward / depth / side are 4 / 2 / 0 cm; Save settings applies live. See FIXES-0.16.4.md.

## Previous release history

0.13.6 adds Staff / Halberd → Grip palm depth (+2 cm default, -10 to +10 cm) and applies depth/finger offsets to both hand contacts. Positive depth moves the shaft toward the palm-facing side, mirrored between hands. Existing Grip toward fingers values remain saved. StaffGripDepthCm is in [Stereo]. Save settings applies live; right-hand tracking and the accepted upward direction remain unchanged. The removed reversal setting remains ignored.

0.16.1 resolves the actual compiled hero Cock alias (2.8 seconds in this installation), fixes mixed quaternion/vector motion tracks, moves and reveals casting items before culling, keeps transformed creatures in native third-person VR, doubles unsupported 2H rotation speed, and preserves native melee progression bonuses instead of clipping them to weapon base damage. Full controller reach was accepted by the user in 0.16.0. See FIXES-0.16.1.md and PROGRESSION-AUDIT.md. New fixes await headset validation.


# Gothic 3 VR 0.16.1 — launcher and settings

The melee tab is named 1H Melee Weapon and its fields Weapon pitch/yaw/roll; saved SwordPitchDeg/SwordYawDeg/SwordRollDeg values are preserved for compatibility. The Bow tab adds Bow pitch/yaw/roll (-90..90 deg), Full draw length (20..80 cm, default 50) and String grab radius (5..30 cm, default 20). Keys BowPitchDeg/BowYawDeg/BowRollDeg/BowFullDrawCm/BowGrabRadiusCm are in [Stereo]. Bow angles adjust the displayed bow and left-controller aim together. The calibrated left grip anchors the arrow origin; the right hand controls draw power. A stereo crosshair marks the same left aim, using native scene raycasts. The animated bow palette is rebuilt per eye at the tracked left grip, with the native palette and frame-cache ID restored afterwards. Native HoldRightStack return values are ignored in favour of validating the actual right slot. The next native held arrow is prepared automatically after a shot. Save settings cancels the current draw and applies live. All seven launcher pages are checked; current headset bow acceptance is pending. Earlier release notes follow.

The Sword tab adds Sword pitch/yaw/roll (-90 to +90 degrees). Zero means the blade's automatic forward grip, on top of saved device calibration. Positive pitch raises the tip, negative pitch lowers it; the sword hand rotates with the blade. Unarmed hands and bows retain the saved device pose. Save settings applies live and discards the current stroke; it cannot generate a hit. Keys SwordPitchDeg/SwordYawDeg/SwordRollDeg are in [Stereo]. All six launcher pages passed layout checks; the Sword page was visually reviewed. Headset sword orientation acceptance is pending.

Left hand and Right hand tabs expose independent outward, up, forward, pitch, yaw and roll calibration. Positions are in cm, angles in degrees. Save settings applies while playing; positive outward is mirrored between hands. Existing configurations default to +5 cm outward for each hand, with zero on other axes. See HAND-CALIBRATION.md. All five pages passed the launcher layout capture; both hand pages were visually reviewed.

0.9.11 adds the Interaction launcher tab with Selection vertical aim: a signed slider and numeric field from -200 to +200 percent. Save settings applies the selection calibration live within approximately one second, including while the game is running. Positive values tilt object/NPC selection upward, negative values downward, and zero restores the original controller direction. For the reported need to bend the wrist down, try -15 or -30 percent. The existing saved calibration is preserved. The expanded range is used consistently by settings loading/saving and the native selection ray; tracked hands, menus and ranged crosshair queries keep their existing transforms.

38/38 tests pass, including signed negative calibration round-trips and the extended range in actual ray mathematics. All three launcher pages passed layout verification. Preferred headset calibration is to be determined by the user.

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

Interfața radialului și a launcherului este în engleză în 0.8.3. Utilizatorul a confirmat radialul și selecția în proba 0.8.2; logurile arată comenzi native confirmate și închiderea normală a motorului. Comenzile și setările existente sunt păstrate.

În 0.9.0, **Tracked arms and weapons (experimental)** din **Camera and controls** activează/dezactivează proba de IK pentru mâinile și armele personajului. Se aplică după salvare și în timpul jocului. Cheia este `[Controllers] TrackedArms=1`. Proba nu implementează încă damage-ul fizic sau întinderea corzii; comenzile de atac native rămân disponibile.

**Radial:** după încărcarea salvării, apasă o dată L3, selectează cu joystickul drept și apasă **A**. **B** sau încă o apăsare L3 anulează. Radialul rămâne deschis fără ținerea joystickului, iar revenirea acestuia în centru păstrează selecția. Prima pagină oferă sloturile rapide 1–9 și 0, iar Y schimbă la pagina de comenzi și meniuri. În inventar, indică obiectul cu cursorul înainte de deschiderea radialului, pentru a trimite tasta rapidă către obiectul respectiv. L3+R3 păstrează prioritatea pentru resetarea camerei. Instrucțiunea apare în launcher.

În 0.7.6, recentrarea este aplicată de cadrul stereo, fără a prelua înclinarea completă a capului prin actualizarea mersului. Cu **Orizont stabil** activ, resetarea pe pantă păstrează verticala lumii și înălțimea personajului. Combinația de butoane și setările existente se păstrează.

**Reset cameră:** apasă simultan ambele joystickuri (L3 + R3) timp de 0,5 secunde, apoi eliberează-le. Indicația apare și în launcher. Recentrarea folosește poziția completă și yaw-ul căștii, păstrând înălțimea personajului și reglajele din launcher. Schimbările originii LOCAL făcute de runtime sunt tratate automat. Cursorul controllerului din meniuri are stabilizare adaptivă și protecție la mișcarea încheieturii când apeși triggerul. Laserul din lumea jocului a fost eliminat; selecția folosește evidențierea nativă a numelui țintei.

Conversațiile păstrează înălțimea first-person memorată. Reglajele de cameră continuă să se aplice la revenirea din dialog, fără recalibrarea după camera cinematică ridicată.

Viteza de rotire are acum intervalul 10…2000%, implicit 500%. Valori precum 800% permit o rotire mai rapidă decât vechiul maxim de 200%, fără schimbarea sensibilității mouse-ului din setările native ale jocului.

Fila **Cameră și control** reglează înălțimea, deplasarea înainte/înapoi și viteza de rotire pe joystick. **Orizont stabil** elimină înclinarea și balansul camerei native în first-person, păstrând trackingul căștii și deplasarea personajului pe teren. Fila **HUD și meniuri** păstrează reglajele existente pentru panouri. Salvarea aplică toate aceste reglaje în timpul jocului.

În 0.7.0, bifa **Controllere Quest / Touch** activează joystickurile, butoanele și pointerul pentru meniuri. Schimbarea se aplică după salvare și în sesiunea curentă. Comenzile și limitele acestei etape sunt în [CONTROLLERS.md](CONTROLLERS.md).

Pornește `Gothic3VRLauncher.exe` din folderul jocului. Conectează Quest 3 prin Virtual Desktop, cu VDXR selectat, apoi apasă **Salvează și pornește**. Opțiunea **Pornește automat în VR** este activată implicit și deschide inclusiv meniul principal în cască. Încarcă salvarea și folosește camera first-person nativă a jocului. Conversațiile păstrează camera nativă.

Launcherul reglează înălțimea camerei, poziția și dimensiunea HUD-ului, poziția și dimensiunea meniurilor și distanța panourilor. HUD înseamnă întreaga interfață permanentă capturată de joc, inclusiv reticulul; elementele sale nu sunt mutate individual. Când este deschis un meniu sau o pagină a jocului, panoul folosește reglajele meniurilor. Distanța este comună, iar raportul de aspect se păstrează. Poziția este relativă la cască, în centimetri: valorile negative coboară panoul.

**Rezoluție per ochi** alege dimensiunea efectivă a randării native: QHD, 3200×1800, 4K, 5K sau variante pătrate 2560/2880/3200. **Din joc — fără modificare** păstrează opțiunile video existente. Alegerea este salvată în `[Stereo] RenderWidth/RenderHeight`; la **Salvează și pornește**, launcherul aplică dimensiunile în `Documents\Gothic3\UserOptions.ini`, folosind modul fereastră PUP (`WindowMode=0`) și refresh 0. Enum-ul din SDK pentru modul ferestrei are alte valori decât această cheie PUP. Creează un backup `UserOptions.ini.bak` și păstrează celelalte setări, inclusiv audio, gamma și taste. Nu schimbă rezoluția monitorului sau setările de streaming Virtual Desktop. Rezoluția aleasă afectează și oglinda desktop și textura interfeței; este independentă de reglajul dimensiunii fizice a panourilor.

Rezoluția necesită repornirea jocului: **Salvează setările** pregătește alegerea, iar **Salvează și pornește** o aplică înainte de crearea dispozitivului grafic. Motorul creează la noua dimensiune și resursele de postprocesare; mărirea doar a texturilor OpenXR ar interpola aceiași pixeli. Fiecare ochi primește întreaga rezoluție aleasă. 3840×2160 și 2880×2880 au fiecare 2,25 ori numărul de pixeli al rezoluției 2560×1440. Rezoluțiile mai mari cresc costul GPU și pot reduce FPS-ul. Pentru revenire, alege 2560×1440 și relansează din launcher. Logul `game_stereo_ready` confirmă dimensiunea reală a backbuffer-ului preluat de OpenXR.

**Corecții 0.6.3:** motorul respinge la pornire dimensiuni absente din lista monitorului chiar în modul fereastră. PUP încarcă `patches\Patch_Gothic3VRResolution.dll` înainte de crearea motorului; acesta adaptează un context nativ pentru rezoluția cerută și îl păstrează pe durata sesiunii, astfel încât încărcarea unei salvări să nu revină la dimensiunea veche. Intervenția este permisă numai pentru hash-ul Engine.dll verificat, CP 1.75.20108.14 din instalarea cu PUP 1.1.1.

Jocul redenumește temporar `UserOptions.ini` în `__UserOptions.ini` la pornire; inițializarea eșuată poate lăsa originalul lipsă. Launcherul recuperează configurația validă din acest fișier sau din `.bak`. Scrierea video schimbă numai cele patru chei, păstrează celelalte date și encodingul, scrie și golește pe disc un fișier temporar, apoi folosește `ReplaceFileW`. O eroare raportează codul Windows și calea reală.

La 4K, cache-ul viewport-ului interfeței putea rămâne 2560×1440. Sincronizarea dimensiunilor native rulează înainte de `OnProcess`, când jocul poate crea inventarul/dialogurile, și înainte de desenare, inclusiv la schimbarea paginii. Coordonatele mouse-ului sunt remapate de la partea accesibilă a ferestrei desktop la întregul viewport de randare, înainte de conversia nativă în coordonatele virtuale ale interfeței. Mișcarea și clickurile folosesc aceeași transformare, numai în meniuri cu VR activ. În proba live 4K, Bogdan a confirmat centrarea inventarului și conversațiilor, precum și accesul și selecția tuturor opțiunilor cu mouse-ul. Capturile GPU confirmă interfața întreagă în textura de 3840×2160.

După revenirea la **Din joc**, o rezoluție personalizată deja salvată în modul fereastră rămâne acceptată de patch-ul timpuriu, fără rescrierea configurației video. Un mod fullscreen existent este păstrat și nu activează această intervenție.

Pentru diagnostic, variabila de mediu opțională `G3VR_DUMP_UI_FILE` salvează textura UI după stabilizarea paginii, maximum 16 capturi pe sesiune. Fără această variabilă nu există readback GPU pentru capturi. Cele 25 de teste automate trec, inclusiv transferurile GPU 4K și pătrate, marginile mouse-ului și păstrarea unei rezoluții 4K la revenirea la **Din joc**. Proba live cu selecția opțiunilor a fost efectuată separat.

Implicit, camera are offset **−25 cm**, HUD-ul **−30 cm**, meniurile **0 cm**, dimensiunea **100%**, distanța **200 cm**. Camera acceptă −50…50 cm. Poziția panourilor acceptă −150…100 cm, dimensiunea 40…180%, distanța 100…500 cm.

**Salvează setările** aplică reglajele de cameră, panouri și vizibilitate în cel mult o secundă și în timpul jocului. Pornirea automată se aplică la următoarea lansare; schimbarea bifei nu comută sesiunea curentă. **Valori implicite** pregătește valorile inițiale fără a le salva automat. Launcherul refuză lansarea unui al doilea Gothic 3.

Configurația rămâne `scripts\Script_Gothic3VRDiagnostics.ini`. Salvarea păstrează celelalte chei/secțiuni, creează o copie `.bak` și înlocuiește configurația numai după scrierea completă. Installerul păstrează valorile deja existente, inclusiv offsetul camerei. Calea jocului se poate alege din launcher și este salvată în `Gothic3VRLauncher.ini`, lângă aplicație.

**Ctrl+Shift+F8** rămâne comutatorul manual VR. **Ctrl+Shift+F11** recentrează. Dacă headsetul intră în standby și runtime-ul întrerupe sesiunea, reconectează casca și folosește comutatorul/recentrarea.

Implementarea pornește conexiunea grafică din randarea camerei sau din faza UI, fără să ceară o salvare încărcată în first-person. Hook-ul pentru păstrarea first-person se instalează după intrarea în lumea jocului. Citirea setărilor și alegerea panoului se fac pe thread-ul aplicației; OpenXR folosește un quad VIEW cu alpha premultiplicat, după proiecțiile stereo. Nu se schimbă runtime-ul OpenXR global.

Build: `tools\Build-Diagnostics.ps1`; instalare: `tools\Install-Diagnostics.ps1`. Testele includ transferul GPU și transparența UI, trimiterea parametrilor panoului către compositor, salvarea configurației cu păstrarea celorlalte setări și pornirea unui proces de probă cu calea și directorul de lucru corecte. Testele nu pornesc jocul real și nu validează confortul în cască.





Bow → **Bow aiming reticle** toggles the visible bow aiming indicator live after Save settings. Off hides the VR indicator and native fixed crosshair without changing aim direction or physical draw. The default is On.

**Bow grip** contains three bow-only contact offsets: **Grip toward fingers**, **Grip palm depth**, **Grip across palm**. They do not move the hand tracking pose. Default values 5 / 2 / 0 cm move contact away from the wrist. Each supports -15 to +15 cm and applies live through Save settings. The arrow stays nocked above the lower grip.

2H / Staff: separate weapon angles, Support grip radius and Two-hand grip checkbox. Bring the free left hand near the handle, hold left grip to steer with both controllers, and release to return to right-hand steering. Save settings applies live; the 1H and Bow profiles remain independent.


Motion (0.18.0): **Direct VR locomotion — separate movement from animation** replaces normal human ground translation with controller velocity, retaining surrounding collision/terrain/physics handling. **Movement start response** (0–150 ms, default 60) controls acceleration only; release stops that contribution immediately. Existing LocomotionResponseMs values are retained. **Legacy camera smoothing** is used only with direct locomotion disabled. **Instant turn response** remains independently configurable. Save settings applies live. See FIXES-0.18.0.md; headset comfort is pending validation.
