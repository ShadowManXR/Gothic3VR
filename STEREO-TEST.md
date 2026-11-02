# Scena stereo geometrică OpenXR / VDXR

Țintă CMake: `Gothic3VRStereoProbe` (Win32, D3D11). Versiunea probei: 0.1.0. Pluginul de tracking al jocului rămâne 0.4.2.

Surse: `tools/StereoProbe.cpp`, `src/StereoRenderer.h`, `src/StereoMath.h`; teste: `tests/StereoMathTests.cpp` și randare pe GPU prin `--self-test`.

Fiecare cadru folosește `xrWaitFrame` / `xrBeginFrame`, `xrLocateViews` la predictedDisplayTime, două imagini acquire/wait/render/release și un `XrCompositionLayerProjection` cu două vederi. Camera și FOV-ul fiecărui ochi sunt utilizate și la randare, și la compoziție. Nu se activează XR_MND_headless. Datele fără poziție/orientare valide și tracked nu generează un strat stereo. Scena are origine stabilă în LOCAL după recentrare.

VDXR furnizează cerințele adaptorului și feature level; dispozitivul D3D11 este creat pe acel adaptor. Formatele acceptate de probă sunt RGBA8/BGRA8 sRGB, două swapchain-uri 2D, fără MSAA. Oglinda de pe PC folosește copii proprii ale imaginilor înainte de release, fără acces la imaginile runtime-ului după release. Resursele COM ale imaginilor runtime-ului sunt eliberate înainte de distrugerea swapchain-urilor.

Repo-ul `D:\Gothic 3 VR\VirtualDesktop-OpenXR` este clonat ca reper pentru integrare; runtime-ul efectiv rămâne cel instalat de Virtual Desktop Streamer. Nu este necesară recompilarea sau înlocuirea VDXR. Codul runtime-ului nu este copiat în probă.

Proba validează legătura stereo OpenXR/VDXR și tracking-ul. Nu validează încă transferul D3D9Ex al Gothic 3 sau reexecutarea randării motorului. Acestea necesită integrarea în joc.
