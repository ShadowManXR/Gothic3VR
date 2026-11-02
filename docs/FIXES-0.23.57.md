# 0.23.57 — first-person world label cost

The native GUI calls IsFocus while drawing names for ordinary NPCs, items and
world entities. Our signpost extension previously queried MenuPanelActive
(including repeated VirtualQuery and native lockpick checks) before discovering
that most of these entities were not signposts. This ran only in first-person.

External capture external-20261023-165618 contains200 first-person samples:
111 include native DrawFocusNames/DrawFocusName, and98 include our signpost
extension's context check. The original matching .56 DLL/PDB maps the hot
addresses to SignpostNameFocus -> MenuPanelActive -> Readable/NativeLockpick.
These are overlapping wall-clock samples, not exclusive CPU percentages.
No samples entered MaintainWallDistance/CollectPolysInAABB; their behavior is
unchanged. Actual FPS improvement still requires headset acceptance.

The native focus predicate still runs first and keeps its original result.
For extra labels, the synchronous native entity's name is now checked for the
Sign_ prefix before any VR menu, player, memory or headset queries. Native
ownership and SEH protect that short name access; expensive validation remains
for actual signposts. Names, translations, eight-metre visibility and view
cone are retained. No persistent entity-pointer cache is introduced.

Regression validation processes70000 non-sign labels without invoking the
expensive extension, then checks positive signs and context rejection. Existing
guard/heal/hearth/particle/controller interaction tests also pass. Normal
runtime remains compiled without old profiling/capture experiments. No saved
settings, distances, engine DLLs or separate DX11 trial binary are changed.
