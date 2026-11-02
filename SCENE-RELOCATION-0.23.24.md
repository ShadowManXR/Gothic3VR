# Correct batch profiling when Engine.dll is relocated

The0.23.23 actual PID9304 startup reported8/9 deeper scene hooks (mask509).
Read-only inspection proved Engine.dll loaded at0x028C0000. The untouched
ExecuteAllInstances prefix contains MOV EDI,[0x02ECA5B8], where its on-disk
preferred-base operand is0x1060A5B8. The missing measurement was a conservative
prefix refusal caused by the loader's normal HIGHLOW relocation, not PUP.

0.23.24 generates the expected operand from the actual Engine module base plus
the verified0x60A5B8 global RVA. Only those four address bytes change. Whole-file
hash, symbol, RVA and every instruction byte are still validated. No jump is
followed, existing detours are rejected, and the native batch function executes
unchanged. The trampoline copies the actual relocated address.

The real PE test confirms a HIGHLOW fixup at function RVA+5. Regression tests
cover preferred base, the captured runtime base, other unchanged signatures,
overflow, malformed operands and a real executable synthetic x86 batch hook:
the moved global is read through the trampoline and the original call forwards
once. This synthetic test does not execute Gothic3's native code.

Sparse timing remains one of32 world stereo frames with diagnostic temporal
frames excluded. A successful startup mask511 enables the missing instance-batch
measurement. This release fixes diagnostic coverage; it claims no FPS gain.
Launcher, gameplay, bow settings, engine/DXVK/PUP files and DLAA remain unchanged.

The existing ExecuteInstance geometry observer also records its inclusive cost
on the exact same sparse frames. No second detour is installed on that entry.
New phase sampled_instance_item_inclusive is appended at ID39; old IDs stay
stable. Temporal entity/history capture and cleanup remain inside their original
nested scope. This separates the batch dispatcher from its per-item execution.
Comparisons must use matched closed sample periods; window-boundary child/parent
reporting can differ. Never subtract unrelated maxima or add inclusive scopes.
