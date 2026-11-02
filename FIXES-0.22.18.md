# 0.22.18 — combat movement recovery and third-person knockdowns

## Captured incident

Session `diagnostic-20261015-232839-2196.jsonl` records nonzero native wished directions with a stationary player root. Combat translation remains enabled after the script returns to `PS_Melee_Loop` / `PS_Normal_Loop`, including after the ordinary movement permission becomes enabled again. At approximately 154 seconds combat translation clears and normal movement resumes, matching the reported orc push. Later episodes again show the same combat flag. Power attacks are a reported trigger; the original log does not record the SPU instruction owner and cannot prove every initiating action.

The installed native `ProcessControledTranslation` takes its combat branch before the normal movement branch while this flag is set. Only `sAICombatMoveStart` and the combat instruction epilogue call its enable/disable setter in this Game.dll. This explains the native movement override; the precise callback left by each incident still needs live confirmation.

## Changes

- Observe the exact player's idle combat state for 300 continuous milliseconds. A null SPU instruction with combat translation still active receives the native epilogue's SDK movement/rotation/combat setters. A still-active attack instruction (actions 1–14) can instead be retired with the native `AIStopCombatMove` when physical VR melee has already returned to its standing idle loop.
- Do not retire live incoming-hit/parry, finisher, knockout/get-up, ranged/magic, other script instructions or special movement modes. Menus, pause, world changes and identity changes invalidate the observation. NPC calls do not advance or reset the hero's observation.
- No position, root velocity, movement mode, input, damage, stamina or learned skills are rewritten by orphan recovery. Native attack cancellation uses the game's complete instruction cleanup.
- Add SPU callback/action/time diagnostics alongside the existing once-per-second movement records so any remaining case can be classified.
- All native knockdown/knockout/down states now use third-person, including ordinary sitting/lying knockdowns and get-up. The head stays visible during this camera exception. Return to calibrated first-person uses the existing activity-return path after standing.

## Verification

Policy tests cover orphan and physical-attack ownership, cooldown/identity/pause boundaries, all reaction/action exclusions, and one-shot mutation. Native binary checks validate the private instruction-action read, native stop entry and combat-translation gate/cleanup. Camera tests cover every fall/KO pose, down-only detection, get-up, visible head, actor changes and first-person return. Full build/test outcome is recorded with the installation receipt.

Headset combat reproduction and native reaction timing remain to be tested. Audio, hands, bows, crossbows and user settings are unchanged.
