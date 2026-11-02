# Main-menu Gothic 3 VR branding

The native menu layout `g3.gui.res` references
`G3_HUD_View_Menu_Main_Back.tga`. The supported `Library.pak/g3.igl` icon map
resolves it to `Data/gui/icons/compiled/g3_hud_backgrounds_02.dds`.
This is the Enhanced Edition banner scene used by the Community Patch menu.
The installed GUI patch/mod archives contain no later replacement for this
particular atlas.

The replacement adds a bronze/gold `VR` wordmark beside the existing title.
Artwork was edited with the built-in imagegen tool, using the decoded native
atlas as the edit target. The generation prompt is saved with the source art
under `assets/menu-branding/imagegen-prompt.txt`.

Native resource payload:

`assets/menu-branding/Data/gui/icons/compiled/g3_hud_backgrounds_02.dds`

Copy the `Data` subtree below the game root. The loose GUI resource overrides
the archived texture through the existing GUI mount; no DLL hook, menu layout
or compiled icon-map modification is needed. Restart Gothic 3 to reload an
already cached menu texture.

`tools/PrepareMenuBranding.py` converts the approved artwork into the original
1024 x 1024 DXT3/BC2 slot with one mip level and exactly 1,048,704 file bytes.
The original DDS header and every native alpha block are retained, including
the unused lower 256 pixels and ornamental border coverage. The encoded file
is decoded again to verify its dimensions and transparent padding.

The source image, native archive extracts and installation receipt are kept
outside the game. Original `gui.pak`, GUI patch/mod archives, `g3.igl`, game
settings and VR runtime binaries remain unchanged. To revert, remove only the
new loose DDS override (or restore the previous loose override from the
installation backup if one existed).

Validation still required in the game: after a fresh launch, confirm the main
menu title reads `Gothic 3 VR` and its buttons and other menus render normally.
