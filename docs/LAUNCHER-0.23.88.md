# Launcher 0.23.88 — Gothic VR background and translucent panels

Uses the supplied `ChatGPT Image Oct 26, 2026, 08_40_18 PM.png` as the embedded
launcher background. `tools/assets/launcher-background.png` is a byte-identical
copy (SHA256 `1BDC86FEDF49FA21F175323BEE235BE9FE9113EE33D559F50B95EB7511D6D9EB`).
The original artwork is not recolored or regenerated.

The source has transparent padding below the visible 1254×944 artwork. A
one-time startup alpha scan selects the visible content for aspect-preserving
cover, ignoring alpha values below 4/255. The original PNG stays intact.

Settings cards, navigation background and footer use alpha 230/255 (90.2%
opacity). Unselected navigation buttons reuse the composed background instead
of hiding it with an opaque fill. A lighter global veil leaves the picture
visible; the header retains a local dark gradient for text readability.
Text, input fields and action buttons retain clear contrast. Existing cached
canvas/slider painting, enlarged G3 VR emblem and Donate link are preserved.

All 40 UI reports passed at normal and compact sizes, including 18 pages,
interaction round-trips and idle repaint checks. Both primary screenshots were
visually inspected. Only the launcher changes; game/plugin/settings remain intact.
