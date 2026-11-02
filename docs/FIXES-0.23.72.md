# 0.23.72 - Dialogue comfort, clustered selection and single-item trading

## Static third-person conversations

VR retains the first displayed native third-person dialogue position/orientation until dialogue mode ends. Later native orbits, camera interpolation and shot changes do not move that shared render base. Both eyes still receive live OpenXR head tracking, projections and IPD: looking around and leaning remain immediate.

This copies the native framing rather than disabling camera AI or editing dialogue scripts. Native camera state is restored after the existing render scope. Dialogue exit restores the existing camera policy; world unload, runtime disconnect and player replacement clear stale ownership. The first captured framing and return to first person require headset validation.

## Small objects among large interactables

Interaction focus now distinguishes precise ray hits from generous assistance bounds and weighs target size/alignment. A chest between beds and small offerings on a shrine can win over oversized furniture focus volumes. Ground-item aim assistance and configured reach remain available.

The final VR choice is selected from candidates that already passed native eligibility and line-of-sight checks. It no longer loses solely because native `HasLowerPriority` favours another object's root within 300 game units. This does not enumerate the entire world, alter native inventory/ownership restrictions or change ranged weapon targeting. Small selected-item hysteresis is reduced to avoid a prior target trapping focus.

Binary evidence in the existing REA/Ghidra output for the supported Game.dll: `GetCurrentPriority` at 0x1006c910, `IsValidEntity` at 0x1006d3f0, `HasLowerPriority` at 0x1006d6f0, `FocusCheck` at 0x1006d7c0 and `FindFocusEntity` at 0x1006da20. These are analysis results, not recovered original source.

## Buy/sell one item

In the **trade screen only**, aim at an item and press **right grip** to transfer one unit into/out of the pending trade. Release and press again for another unit. Holding grip does not produce an automatic repeating stream. **Right trigger** retains normal left-mouse clicking/dragging for full stacks and the existing trade buttons. The transaction still requires the native Trade/Balance controls; prices, quantities, gold and item ownership are not modified directly.

The controller sends the native right-mouse input, bounded to 80 ms and cancelled on menu/world, focus, pointer or capture transitions. It is separate from world `Use2` despite sharing the same physical mouse binding. The trade page is the SDK enum `gEGamePage_Trade`, confirmed by `gCGUIManager::BeginTrade` / `GetActivePage`.

Native quantity handling is also visible in the local Game.dll analysis: right-button handler 0x100a1a50 calls the transaction-update routine 0x100a1fa0; right-button release 0x100a1af0 clears repeat state. The [Gothic 3 handling guide](https://mondgesaenge.de/G3DB/guide_handling.htm?id=handling_1) documents right-click single-unit trading. The [publisher's manual](https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/39500/manuals/G3_Manual_UK.pdf) describes the pending trade and gold balancing flow.

## Verification and scope

Regression checks cover fixed dialogue framing, preserved stereo/head movement, camera exit/lifecycle, furniture/altar target competition, candidate order, LOS/reach preservation, single-click trade input and context cleanup. Launcher hints remain English. Headset and native merchant acceptance remain to be tested; automated tests do not claim an actual purchase or measured comfort improvement.

The accepted mapping stays: hold left primary trigger for Sprint, release for Run, tap L3 for quick heal; left grip remains weapon support. Native movement direction, runtime timing, graphics, engine INIs, mod payloads and calibration are unchanged. No profiler or continuous input capture is enabled.
