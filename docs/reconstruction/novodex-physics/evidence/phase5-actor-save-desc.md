# Phase 5 actor saveToDesc, dynamic vtable slot 82

The pinned actor dynamic table at RVA `0x104530` points slot 82 at RVA
`0x3690` (`phys_fn_000120`). Ghidra and the Win32 table identify a write-guarded
`saveToDesc(NxActorDescBase&)` implementation. It copies the body pose for a
static actor, or converts the dynamic record quaternion at `+0x5c` and copies
its translation at `+0x50`. It also copies body density at `+0x18`, actor flags
at `+0x14`, actor group at `+0x1c`, and actor user data at `+4`. It does not
touch the descriptor's body pointer or name.

`NpActorVtable::saveToDesc` now transcribes those writes. The new public-DLL
probe exercises a static actor, a normal dynamic actor, a half-turn actor, and
a quarter-turn actor. For each, both the oracle and candidate return the same
12 pose words and descriptor metadata, including preservation of the sentinel
body pointer and name. The dynamic probe initially exposed a separate
construction mismatch: `nxActorLoadFromDescInternal` had left body density at
zero. It now stores the descriptor density at body `+0x18`, as the oracle does.

Eight new output lines are pinned in the Phase 5 gate, bringing its assertion
floor from 667 to 675. The inventory row is `reconstructed`. The family gate
remains red because the shape final tables and actor class family are not yet
closed; the explicit `CANDIDATE-MISSING` marker remains in place.
