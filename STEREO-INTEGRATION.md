# Stereo weapons / native movies — 0.21.0

Prepare 2H/staff/shield once before both eye frusta; handle native cached palettes. Native PlayVideo uses GPU backbuffer capture and opaque both-eye panel. Startup clips before plugin load are not covered. Headset testing pending; see BATCH-0.21.0.md.

## Previous release notes

# Gothic3VR 0.5.6 — stereo geometric și interfață VR

Plugin Win32/x86 pentru Gothic 3 1.75.20108.14, cu D3D9Ex și Quest 3 conectat prin Virtual Desktop/VDXR. Instalează prin `tools/Build-Diagnostics.ps1 -Install`; installerul păstrează backup-ul DLL-ului și configurației modificate. Runtime-ul global nu este schimbat.

## Proba în joc

1. Conectează Quest 3 prin Virtual Desktop și ține casca activă. Închide scena standalone dacă rulează.
2. Pornește Gothic 3, încarcă o salvare și intră în first-person nativ.
3. Privește înainte și apasă **Ctrl+Shift+F8** cu jocul activ. Aceasta activează stereo și recentrează când VDXR furnizează tracking activ.
4. Scena Gothic 3 trebuie să apară direct în VR. Rotește capul și deplasează-l ușor lateral pentru a verifica adâncimea, urmărirea și scara lumii.
5. **Ctrl+Shift+F11** recentrează; **Ctrl+Shift+F8** dezactivează stereo și închide sesiunea grafică. Dezactivează stereo înainte de schimbarea rezoluției.
6. Dacă stereo se dezactivează după o eroare sau standby, reactivarea cu F8 încearcă o sesiune nouă. Logurile explică motivul.

Configurație: `[Stereo] Enabled=1`, `UnitsPerMeter=100`. Scara de 100 unități/metru este punctul inițial de calibrare, nu o măsurătoare confirmată în cască. `Enabled=0`, după repornirea jocului, restabilește traseul 0.4.2 cu tracking headless și imagine pe desktop.

## Implementare

- Hook verificat pe exportul `eCCameraBase::Render(bool,bool)` și pe actualizarea proiecției. Hash-ul complet Engine.dll și prologul exact sunt verificate înainte de patch. Hook-ul refuză alte binare sau un prolog deja schimbat de alt mod.
- O sesiune OpenXR grafică D3D11 pe GPU-ul cerut de VDXR și folosit de joc. Worker-ul headless nu este pornit când stereo este configurat.
- La `xrWaitFrame` / `xrBeginFrame`, pozițiile, orientările și FOV-ul ambilor ochi sunt localizate la predictedDisplayTime. În fiecare ochi, poziția fizică relativă centrării este convertită în unitățile jocului; camera nativă păstrează deplasarea și orientarea de bază.
- Motorul randează lumea de două ori, cu reconstrucția frustumului pentru fiecare ochi. Logica OnProcess nu este apelată suplimentar. Camera, view și projection sunt restaurate după perechea stereo, inclusiv pe traseul de unwind al randării.
- Imaginea fiecărui ochi este capturată după randarea camerei în textura D3D9Ex partajată, apoi copiată pe GPU în imaginea D3D11 OpenXR. Evenimentele GPU D3D9 și D3D11 confirmă ordinea înainte de reutilizarea texturii. Nu există readback CPU în fluxul live.
- Două vederi reale într-un `XrCompositionLayerProjection`. Fiecare vedere folosește aceeași poziție/FOV la randare și compoziție. FOV-ul nativ recalculat de motor este înlocuit doar pe durata ochiului.

## Limite ale primei versiuni

- Activarea inițială se face în first-person nativ. După activare, randarea continuă și când jocul schimbă camera pentru lovituri, dialoguri sau alte interacțiuni. Din 0.5.6, interfața nativă desenată în OnPostRender/OnRenderDraw este capturată într-un panou VR separat. Vizibilitatea tuturor paginilor și a ecranelor de încărcare trebuie confirmată în proba live.
- Rezoluția per ochi este rezoluția curentă a backbuffer-ului jocului; nu este încă optimizată după rezoluția recomandată a căștii. FOV-ul este cel al runtime-ului, independent de raportul ecranului PC.
- Oglinda de pe PC păstrează ultima randare a lumii (ochiul drept). Efectele temporale, umbrele și postprocesarea trebuie verificate în proba live pentru randarea dublă.
- Sincronizarea GPU este conservatoare, cu așteptări explicite; optimizarea latenței și numărul de buffere urmează după validarea vizuală.
- Locomoția existentă și tastatura/mouse-ul rămân disponibile; integrarea controllerelor VR și țintirea necesită etape ulterioare.

Logurile principale: `scripts/Gothic3VRDiagnostics/logs/diagnostic-*.jsonl`. Loguri grafice: în același director, `stereo-game-*.jsonl`. Verifică `geometric_stereo_hooks_installed`, `game_stereo_ready`, contoarele submissions / left_copies / right_copies și perechile `stereo_eye` din logul principal.

## Corecție 0.5.1: închiderea jocului

Proba 0.5.0 a confirmat lumea în stereo și tracking, dar jocul a căzut la ieșire. Callstack-ul din Lastlog_GE3.log trece prin eCApplication::Destroy, DestroyEngine, DestroyModules și eCGfxContextAdmin::Destroy, apoi în d3d9.dll (RVA 0x12FFE1). La acel moment sesiunea VR și resursele D3D9Ex partajate nu fuseseră eliberate.

Versiunea 0.5.1 interceptează callbackul SDK OnDestroyEngine, verificând slotul 165 și importul simbolic Game::OnDestroyEngine din executabil. Închide sesiunea OpenXR, eliberează referințele D3D11/D3D9, apoi continuă callbackul original. Randarea VR refuză reactivarea după începerea închiderii. Curățarea se execută înaintea închiderii modulelor motorului, în afara DllMain.

Cele 14 teste trec. Testul GPU verifică explicit că dispozitivul nativ nu mai are referințe VR când este distrus; testele SDK și EXE verifică traseul callbackului de închidere. Confirmarea finală necesită o ieșire normală din Gothic 3 după activarea VR, fără dezactivare manuală prealabilă. Logul stereo trebuie să se încheie cu game_stereo_summary.
## Corecție 0.5.2: schimbările camerei

În proba live 0.5.1, ieșirea normală a închis sesiunea VR și resursele GPU înainte de rendererul jocului. Logul conține game_stereo_summary cu 4779 de perechi stereo și stereo_resources_released_before_engine_shutdown.

Aceeași probă a arătat first_person=false la 53–55 s, 58–59 s și de la 61 s încolo. Vechea condiție din PrepareStereoFrame întrerupea cadrele la aceste schimbări de cameră; utilizatorul le-a observat la lovituri și interacțiuni. Dialogurile și temporary first-person erau excluse explicit.

Condiția de first-person se aplică acum numai conectării inițiale. După conectare, orice randare a camerei perspective curente a jocului continuă în stereo, cu tracking și restaurarea camerei native. Schimbările de mod nu recentrează casca și nu întrerup sesiunea. Direcția de mers folosește în continuare condițiile native de gameplay. Logul adaugă stereo_camera_mode pentru tranziții.

Toate cele 14 teste trec, inclusiv regresia de eligibilitate a cadrelor peste schimbări de cameră și geometria ochilor. Verificarea în joc rămâne necesară: activează VR în first-person, lasă un animal să te lovească și interacționează cu un personaj; imaginea trebuie să continue să se actualizeze în ambele cazuri.

Această corecție privește randarea lumii. Textul, opțiunile de dialog și celelalte elemente UI sunt desenate după capturarea camerei și necesită integrarea separată a interfeței VR. Dacă motorul nu randează deloc camera în timpul încărcării, acest hook nu poate produce o scenă nouă.
## Corecție 0.5.3: first-person la knockdown

Game.dll verificat (SHA256 4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00) arată că gCCameraAI_PS::IsInFirstPerson consultă scriptul IsFirstPersonCameraAllowed, deși opțiunea nativă first-person poate rămâne activă. SetMode consultă aceeași verificare înainte să aleagă presetul first-person. SDK-ul expune apelul prin gCScriptAdmin::CallScript, slot virtual 47.

Versiunea 0.5.3 interceptează dispatcherul încărcat și apelează întotdeauna funcția originală cu aceleași argumente. Transformă numai rezultatul 0 în 1 al scriptului exact IsFirstPersonCameraAllowed, pentru entitatea jucătorului, pe firul jocului, când stereo VR este conectat și activ. Rezultatele de eroare, celelalte scripturi și NPC-urile rămân neschimbate. Dezactivarea VR și începerea închiderii restabilesc imediat răspunsul original. Nu schimbă damage-ul, animația knockdown sau starea AI.

Configurație: [Stereo] KeepFirstPerson=1 (implicit). KeepFirstPerson=0 păstrează comportamentul 0.5.2 după repornire. Instalarea păstrează o valoare existentă și salvează backup-ul configurației înainte de adăugarea noii opțiuni.

Logul arată first_person_camera_permission_hook_installed la instalarea hookului și vr_first_person_permission_retained cu numărul de refuzuri native ocolite. Dacă profilul sau dispatcherul nu se potrivesc, hookul este refuzat și stereo rămâne disponibil.

Cele 15 teste trec, inclusiv argumentele și numărul de apeluri originale, izolarea la scriptul camerei și jucător, damage/stand-up neschimbate, revenirea cu VR oprit și propagarea excepțiilor native. Proba în joc trebuie să confirme că o lovitură cu knockdown păstrează perspectiva first-person, în timp ce personajul cade și se ridică normal.

Această modificare vizează refuzul camerei la knockdown. Camera de dialog și alte perspective alese explicit rămân gestionate de joc; blocarea lor într-o perspectivă VR proprie este o etapă distinctă. Animația personajului poate modifica poziția camerei native, la care se adaugă trackingul 6DoF al căștii.
## Reglaj 0.5.4: înălțimea ochilor

Utilizatorul a confirmat că 0.5.3 păstrează first-person la knockdown și preferă camera nativă third-person în conversații.

Versiunea 0.5.4 adaugă [Stereo] FirstPersonHeightOffsetCm=-10. La scara configurată în UnitsPerMeter, aceasta coboară ambii ochi cu 10 cm pe axa verticală a lumii. Se aplică numai când camera este în first-person sau temporary first-person și camera de dialog nu este activă. Poziția nativă se restaurează după randarea stereo. Pozițiile relative 6DoF și distanța dintre ochi se păstrează.

Valori utile: -5 pentru o coborâre mai mică, -15 pentru una mai mare, 0 pentru înălțimea originală. Setarea acceptă centimetri întregi între -50 și 50; textul invalid revine la -10. Salvează configurația din scripts/Script_Gothic3VRDiagnostics.ini și apasă Ctrl+Shift+F11 în joc pentru reîncărcare și recentrare, fără repornire. Activarea VR cu F8 reîncarcă și ea valoarea. Logul înregistrează first_person_height_offset_cm.

Cele 15 verificări existente trec. Poziția exactă la nivelul ochilor se calibrează în cască; -10 cm este valoarea inițială, aleasă pentru proba vizuală.

## Corecție 0.5.5: capul în first-person VR

[Stereo] HideFirstPersonHead=1 ascunde capul jucătorului numai în first-person/temporary first-person VR, în afara camerei de dialog. Reglajul poate fi reîncărcat cu Ctrl+Shift+F11; valoarea 0 dezactivează ascunderea. În instalația curentă se păstrează FirstPersonHeightOffsetCm=-25, confirmat de utilizator.

SDK-ul expune eCVisualAnimationFactory::SetBodyPartVisible și lista eSAdditionalBodyPart. Modificarea selectează exact Slot_Head, Slot_Hair, Slot_Beard și Slot_Helmet, păstrând celelalte părți ale corpului. Randarea nativă verificată în Engine.dll, la RVA 0x43F30, folosește aceeași vizibilitate atât pentru LoD-ul normal al piesei, cât și pentru cel facial; ascunderea Slot_Head exclude inclusiv ochii și gura acelui actor. Nu se oprește animația facială.

Părul, barba și casca pot fi entități separate. Sunt identificate prin sloturile native ale inventarului jucătorului, cu toate entitățile copil. Se memorează fiecare flag de randare înainte de modificare. EnableRendering se propagă recursiv în Genome, deci restaurarea se face în ordinea părinte–copil, păstrând inclusiv copiii care erau deja invizibili. Nu se ascund player-ul, corpul, armura sau armele prin această modificare.

Snapshot-ul este local cadrului: ascundere înaintea celor două randări stereo, restaurare în __finally înainte de restaurarea camerei. Nu persistă o stare ascunsă între cadre, în salvare, la dezactivarea VR sau la revenirea în third-person/dialog. Colectarea este limitată și refuză structurile nevalide înainte de schimbarea vizibilității. Logul include player_visual, player_head_part și first_person_head_visibility, pentru confirmarea pieselor reale din salvare.

Validare: build Win32 Release și 16/16 teste trecute. Testul nou acoperă selecția capului, camerele excluse, ambele randări, restaurarea flagurilor recursive diferite, referințele duplicate, refuzul înainte de modificare și restaurarea după excepție SEH atât în randare, cât și într-un setter. Proba vizuală în Quest 3 urmează după instalare: first-person cu mișcarea capului, conversație third-person și revenire la first-person.

## Integrare 0.5.6: meniuri și HUD

Problema confirmată de utilizator: lumea se vede în VR, dar meniurile nu apar. Capturarea și xrEndFrame se făceau înaintea interfeței native. Logul live 0.5.5 arată OnPreRender, OnPostRender.enter, OnRenderDraw.enter/exit și OnPostRender.exit; OnRenderDraw este inclus în OnPostRender.

0.5.6 păstrează captura celor două scene geometrice, dar amână xrEndFrame până la terminarea întregului OnPostRender nativ. În această fază, patru metode D3D9 (DrawPrimitive, DrawIndexedPrimitive și variantele UP, sloturi 81–84) reproduc comenzile GPU pe o textură UI transparentă, numai pentru dispozitivul jocului, firul capturii și ținta backbuffer. Callback-urile jocului, logica UI și input-ul rulează o singură dată. Originalul fiecărei comenzi continuă să deseneze imaginea de pe monitor. Previzualizările cu ținte intermediare sunt excluse; sprite-ul lor final poate fi capturat normal.

În fiecare reproducere sunt păstrate și restaurate render target-ul, depth surface, viewport-ul, scissor-ul și stările schimbate. Alpha se acumulează separat (ONE / INVSRCALPHA), iar culorile păstrează blend-ul nativ. Textura se golește complet înaintea fiecărui cadru, indiferent de viewport-ul nativ anterior. Nu se elimină fundalurile negre printr-o culoare-cheie și nu se fac diferențe între imaginile lumii.

Interfața folosește o imagine BGRA8 sRGB și un XrCompositionLayerQuad cu transparență premultiplicată, după XrCompositionLayerProjection. Panoul este în spațiul VIEW, pentru ambii ochi, la 2 m în fața capului, cu lățime de 2,6 m și raportul original al ecranului. Lumea rămâne geometrică; interfața nativă este un panou plat. Mouse-ul și tastatura păstrează maparea nativă. Controlerele VR și pointerul lor nu sunt implementate în această etapă.

După activarea inițială în first-person, dacă un cadru de meniu nu mai randează camera perspectivă, OnPostRender începe un cadru OpenXR doar cu interfața. Un cadru incomplet sau întrerupt este anulat, iar sesiunea și capturarea sunt oprite înaintea distrugerii motorului. Reglajul -25 cm și ascunderea capului rămân păstrate.

Validare: build Win32 Release fără avertismente și 18/18 teste trecute. Testul GPU nou verifică toate cele patru metode de desen, transparența, alpha parțial premultiplicat, panouri opace inclusiv negre, egalitatea pixelilor desktop cu randarea nativă fără captură, restaurarea țintei/depth/stărilor/viewport/scissor, golirea completă, excluderea țintelor intermediare, lipsa capturii în afara fazei UI, restaurarea hook-urilor și eliberarea referințelor D3D9. Testul compositorului execută codul real de trimitere contra unui xrEndFrame simulat: ordine lume/interfață, două vederi geometrice, panou în VIEW pentru ambii ochi, dimensiuni/alpha/time, cadre numai UI, anulare fără straturi și erori de trimitere fără dublu EndFrame. Aceste teste nu confirmă vizual toate paginile Gothic 3 în cască.

Proba live: pornește jocul, încarcă salvarea, intră first-person și activează VR cu Ctrl+Shift+F8. Deschide inventarul (I), meniul Esc și o conversație. Verifică textele, cursorul și selecția cu mouse/tastatură, apoi închide fiecare pagină și verifică dispariția panourilor. Logul stereo include ui_frames, ui_draws și ui_only_frames; ui_draws trebuie să crească atunci când jocul desenează interfața.
