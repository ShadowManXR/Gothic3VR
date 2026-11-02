# 0.23.45 — general magic hand effects

The previous held-item particle correction did not cover native TargetBone effects attached directly to the hero's skeleton. Audited the installed Library.p00/g3.efm: ice/frost, lightning, healing, charm, summon and other magic sequences use left/right Hero hand bones.

- Enumerate only the live hero animation's native attachment array, bounded128 entries, with owner and hand-bone metadata checks. Native Engine accessor/constructor bytes and SDK method addresses verify the layout before enabling it.
- Relocate VFX_Magic attachment roots with the exact native-to-tracked hand transform, preserving each effect's original attachment offset. Restore all local poses after each eye and before world teardown. The two eyes reuse the same frozen hand pose; repeated application does not accumulate a displacement.
- This covers local-space/instanced visuals through their entity transform and private world-space particles through the existing scoped draw correction. No effect duplication, template edits, particle-buffer writes, simulation edits, NPC/world scans, projectile-root overrides or spell progression changes.
- Native charge, release, targeting, impact and aura ownership remain intact. Only actual hero hand attachments move; head/body/victim/world attachments stay native.

Validation:9 relevant CTest cases passed, including bounded isolated magic attachment poses/restoration for both hands and eyes, empty/stale hands, exact CP engine attachment layout, flame isolation and existing weapon/casting regression cases. Headset acceptance pending. Game settings and renderer unchanged; DX11 trial retained.
