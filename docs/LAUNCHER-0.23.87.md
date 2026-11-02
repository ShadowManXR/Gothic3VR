# Launcher 0.23.87 — Release presentation

The header now uses the existing G3 VR emblem alone, enlarged from 48 to
88 logical pixels and placed at the top right. The separate Gothic 3 / Virtual
Reality wordmark is removed. Page headings and descriptions reserve space for
the emblem. Existing branding artwork, background and application icon are
unchanged.

A centered `Donate` button joins the footer row beside Defaults, Save settings
and Save and launch. Its warm bronze fill, gold border, coral heart and hover /
pressed states match the launcher's dark Gothic palette. It remains an ordinary
keyboard-focusable button. The existing default Enter action still launches.

Clicking Donate opens `https://ko-fi.com/pecos2k5` in the system default browser.
It does not save or collect settings, launch the game, open an embedded browser,
make a payment or contact Ko-fi during launcher startup.

Only the launcher executable changes. The game plugin, VR runtime, user
settings and all gameplay features are preserved.

Validation: all 18 pages at 1320×940 and 1180×810, including text/control bounds,
interaction round-trips, slider repaint checks and idle repaint checks. Final
normal and compact screenshots were visually inspected. The first concurrent
normal-size idle check saw repaint events while the other test window closed;
the isolated rerun passed without changing UI code. The existing launch fixture
self-test also passed. Browser opening was not triggered during automated checks.
