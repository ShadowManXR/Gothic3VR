# Optional item locator

Install **Technical Mods → ItemLocator** through **Optional mods → Open protected Union+**. Union+ manages installation, removal, version, search radius and categories. Gothic 3 VR includes no separate Locator importer or mod payload. The author is **fyryNy**: https://www.nexusmods.com/gothic3/mods/28.

The VR adapter supports the original 1.0.7 and the 1.0.8 binary included in the local Union+ pack. Hold **left trigger + tap X** to toggle markers. The plugin's configured keyboard shortcut is read from its native object; Union+'s default Y and the older F13 configuration both work. Keyboard input remains native. No changes to the original DLL or INI are needed.

X alone opens inventory. Double-press Y to heal with the default layout. The Locator chord is unavailable in menus, radial selection, activities, animal form and recovery states.

Manage filters in Union+. Search radius remains subject to Entity.ROI; VR does not raise it. Quest-item and quest-target filters can override regular categories. Larger searches can add CPU cost. The original plugin requires Community Patch 1.75.14 and Parallel Universe Patch.

New protected Union+ sessions do not lock Locator files, allowing normal updates and removal. Historical imported caches and recovery receipts are retained outside the game; they are no longer used to install or re-enable the plugin. Existing installed 1.0.7 files continue to work until Union+ replaces or removes them.

Projection adapts only the known plugin's camera-matrix import in memory to the rendered eye-center view and existing OpenXR HUD quad. Stale or invalid stereo data retains the native matrix. Unknown binaries are skipped. Current exact binary profiles verify the native KeyPressed and GetWorldToScreenMatrix imports before patching; keyboard, viewport and toggle object fields were checked in disassembly. The existing accepted 1.0.7 alignment is preserved. The Union+ 1.0.8 path still needs a headset check after installation.

Credits: fyryNy (mod), Georgeto (SDK), game-icons.net (original mod icons). No new installer package was built for local update 0.23.105.
