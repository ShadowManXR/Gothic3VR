# 0.23.69 — radial Main menu and Quicksave

On the Actions page, Weapon is replaced by Main menu in sector5 and Take all
is replaced by Quicksave in sector6. The remaining positions are unchanged:
Inventory, Character, Journal, Map, Magic, Main menu, Quicksave, Back.
The first page still contains all ten native quick slots1 through0.

Quicksave reads SessionKey.QuickSave from the game's UserOptions using the
existing binding resolver, including secondary bindings. It does not assume
F5 and never invokes TakeAll. The native game receives one bounded press and
release and continues to decide when saving is permitted. No automated test
writes or overwrites an actual game save.

Main menu is a distinct controller command using the existing native Escape
transport; it opens the game's menu after the radial closes. No SteamVR-reserved
controller button is needed. The normal gameplay weapon-draw binding is intact.
Both radial confirmations consume A, suppress jumping and cannot repeat while
the button remains held.

Validation: native-binding fixtures deliberately remap Quicksave away fromF5
and give TakeAll a different binding. Tests verify correct press/release,
actual radial sector selection, Main menu versus Weapon, held-confirm behavior,
labels and existing controller focus/release behavior. Both targeted CTests
pass; an Actions-page bitmap was inspected. Save files are untouched.
Launcher UI and settings are unchanged, so its executable need not be replaced.
Installed plugin version is0.23.69; launcher remains0.23.68.
