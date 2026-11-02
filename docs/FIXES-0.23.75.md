# 0.23.75 - Landscape fixed foveation

Hardware FFR now uses a landscape oval: its horizontal axis is 1.5 times its
vertical axis in each eye's normalized view. Both the native-detail boundary
and Performance's outer 4x4 boundary use the same orientation.

Reciprocal axis scaling retains the previous nominal full-detail area before
viewport clipping and conservative hardware-tile coverage. The saved
`FfrDiameterPercent` value and its 40-95% range still scale both axes together;
no settings migration or extra shape option is needed. Per-eye projection
centres remain unchanged. Render-target pixel aspect does not determine the
orientation, since XR maps that texture into the headset eye field of view.

Only the rate-map calculation and launcher guidance change. Native geometry,
depth, HUD/menu exclusion and backend ABI are preserved. The Meta x64 host and
the DXVK backend do not need rebuilding or replacement. Pattern generation
still happens only when FFR configuration changes.

Validation covers shape direction, non-square eye textures, equivalent area,
asymmetric eye centres, conservative tile coverage, increasing size in every
preset and settings round-trips. The actual DXVK GPU check also measures the
oval's horizontal/vertical detail difference and live size changes. Headset
acceptance remains pending.
